#include "prx/libSceAgcDriver/Graphics/include/State.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libc/include/General.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace AgcDriver::Graphics {
namespace {

// While DecodeState runs, unsupported-state checks are recorded instead of thrown so a single failure
// report lists every unsupported setting of a draw (the decoder is a strict allow-list).
thread_local std::vector<std::string>* violations = nullptr;

void collectOrThrow(bool ok, const std::string& reason) {
    if (ok) return;
    if (violations != nullptr) {
        violations->push_back(reason);
        return;
    }
    throw std::runtime_error("AGC graphics: " + reason);
}

std::uint32_t read(const Registers& registers, std::uint32_t offset, const char* bank = "context") {
    const auto it = registers.find(offset);
    if (it == registers.end()) {
        std::ostringstream message;
        message << "missing register in " << bank << " bank at DWORD 0x" << std::hex << offset << " (" << std::dec << offset << ')';
        throw std::runtime_error("AGC graphics: " + message.str());
    }
    return it->second;
}

float readFloat(const Registers& registers, std::uint32_t offset) {
    const auto value = std::bit_cast<float>(read(registers, offset));
    Require(std::isfinite(value), "non-finite register at DWORD " + std::to_string(offset));
    return value;
}

void zero(const Registers& registers, std::uint32_t offset, std::uint32_t mask, const char* name, const char* bank = "context") {
    const auto value = read(registers, offset, bank);
    if ((value & mask) != 0) {
        std::ostringstream detail;
        detail << name << " is unsupported (" << bank << " reg 0x" << std::hex << offset << " = 0x" << value << ", unsupported bits 0x" << (value & mask) << ')';
        collectOrThrow(false, detail.str());
    }
}

VkBlendFactor blendFactor(std::uint32_t value) {
    switch (value) {
        case 0: return VK_BLEND_FACTOR_ZERO;
        case 1: return VK_BLEND_FACTOR_ONE;
        case 2: return VK_BLEND_FACTOR_SRC_COLOR;
        case 3: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case 4: return VK_BLEND_FACTOR_SRC_ALPHA;
        case 5: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case 6: return VK_BLEND_FACTOR_DST_ALPHA;
        case 7: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case 8: return VK_BLEND_FACTOR_DST_COLOR;
        case 9: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case 10: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case 13: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case 14: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case 19: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case 20: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        default: throw std::runtime_error("AGC graphics: unsupported blend factor " + std::to_string(value));
    }
}

VkBlendOp blendOp(std::uint32_t value) {
    switch (value) {
        case 0: return VK_BLEND_OP_ADD;
        case 1: return VK_BLEND_OP_SUBTRACT;
        case 2: return VK_BLEND_OP_MIN;
        case 3: return VK_BLEND_OP_MAX;
        case 4: return VK_BLEND_OP_REVERSE_SUBTRACT;
        default: throw std::runtime_error("AGC graphics: unsupported blend operation " + std::to_string(value));
    }
}

void intersect(VkRect2D& result, const Registers& registers, std::uint32_t offset, bool screen) {
    const auto tl = read(registers, offset);
    const auto br = read(registers, offset + 1);
    if (!screen) Require((tl & 0x80008000u) == 0x80000000u && (br & 0x80008000u) == 0, "scissor window offsets or reserved bits are unsupported");
    const auto x = tl & 0xffffu;
    const auto y = (tl >> 16u) & (screen ? 0xffffu : 0x7fffu);
    const auto right = br & 0xffffu;
    const auto bottom = br >> 16u;
    Require(x <= right && y <= bottom, "inverted scissor rectangle");
    const auto oldRight = static_cast<std::uint32_t>(result.offset.x) + result.extent.width;
    const auto oldBottom = static_cast<std::uint32_t>(result.offset.y) + result.extent.height;
    const auto left = std::max(static_cast<std::uint32_t>(result.offset.x), x);
    const auto top = std::max(static_cast<std::uint32_t>(result.offset.y), y);
    result.offset = {static_cast<std::int32_t>(left), static_cast<std::int32_t>(top)};
    result.extent = {std::min(oldRight, right) > left ? std::min(oldRight, right) - left : 0, std::min(oldBottom, bottom) > top ? std::min(oldBottom, bottom) - top : 0};
}

}

ShaderStages DecodeShaderStages(const QueueState& queue) {
    const auto value = read(queue.context, 0x2d5);
    std::ostringstream prefix;
    prefix << "VGT_SHADER_STAGES_EN=0x" << std::hex << value << ": ";
    const auto validate = [&](bool condition, const char* reason) { Require(condition, prefix.str() + reason); };
    validate((value & 0xfc000000u) == 0, "reserved stage bits are set");
    validate((value & 3u) != 3u && ((value >> 3u) & 3u) != 3u && ((value >> 6u) & 3u) != 3u, "reserved LS_EN, ES_EN or VS_EN encoding");
    const auto primitive = read(queue.userConfig, 0x242, "user-config");
    const bool tessellation = primitive == 9;
    const bool geometry = (value & 0x20u) != 0;
    validate(tessellation == ((value & 4u) != 0), "Patch topology and HS_EN disagree");
    validate(!tessellation || !geometry, "combined tessellation and geometry is unsupported by the reference path");
    const auto path = tessellation ? ShaderPath::Tessellation : geometry ? ShaderPath::Geometry : ShaderPath::Vertex;
    ShaderStages result{path, value, (value & 0x00400000u) != 0 ? 32u : 64u, (read(queue.context, 0x1b6) & 0x8000u) != 0 ? 32u : 64u, {}, {}};
    if (path == ShaderPath::Vertex) {
        validate((value & 0x2000u) != 0, "legacy vertex routing without PRIMGEN_EN is unsupported");
        validate((value & ~0x02402010u) == 0, "unsupported vertex routing, scheduling or wave-ID state");
    } else if (path == ShaderPath::Tessellation) {
        validate((value & 0x00600020u) == 0, "wave32 tessellation or geometry amplification is unsupported");
        validate((value & ~0x0007ed0du) == 0 && (value & 3u) == 1u && ((value >> 3u) & 3u) == 1u, "unsupported tessellation routing");
        const auto config = read(queue.context, 0x2d6);
        const auto parameters = read(queue.context, 0x2db);
        ShaderRecompiler::TessellationConfiguration tess{(config >> 8u) & 0x3fu, (config >> 14u) & 0x3fu, parameters & 3u, (parameters >> 2u) & 3u, (parameters >> 5u) & 3u};
        validate(tess.inputControlPoints != 0 && tess.inputControlPoints <= 32 && tess.outputControlPoints != 0 && tess.outputControlPoints <= 32, "invalid tessellation control-point counts");
        validate(tess.domain == 1 && tess.partitioning == 2 && tess.outputTopology == 2, "only triangular, fractional-odd, clockwise tessellation is supported by the reference path");
        result.tessellation = tess;
    } else {
        validate((value & ~0x0047ec30u) == 0, "unsupported geometry routing, fast launch or wave-ID state");
        const auto group = read(queue.userConfig, 0x25b, "user-config");
        const auto vertices = (group >> 9u) & 0x1ffu;
        const auto primitives = group & 0x1ffu;
        const auto maxVertices = read(queue.context, 0x1ff);
        const auto verticesPerPrimitive = read(queue.context, 0x2ce);
        validate((primitive == 1 || primitive == 2 || primitive == 4 || primitive == 6) && read(queue.context, 0x29b) == 2 && verticesPerPrimitive >= 3, "unsupported geometry input or output assembly");
        const auto inputSize = primitive == 1 ? 1u : primitive == 2 ? 2u : 3u;
        validate(vertices >= inputSize && maxVertices != 0 && maxVertices <= 256 && verticesPerPrimitive <= 256, "invalid geometry subgroup output");
        const auto inputStep = primitive == 6 ? 1u : inputSize;
        const auto groupPrimitives = std::min({primitives, (vertices - inputSize) / inputStep + 1u, maxVertices / verticesPerPrimitive});
        validate(groupPrimitives != 0, "geometry subgroup contains no primitives");
        const auto resources = read(queue.shader, 0x8b, "shader");
        validate(((read(queue.shader, 0x8a, "shader") >> 29u) & 3u) == 3 && ((resources >> 16u) & 3u) == 3, "unsupported geometry VGPR allocation");
        result.mesh = ShaderRecompiler::MeshConfiguration{primitive, groupPrimitives, (groupPrimitives - 1u) * inputStep + inputSize, maxVertices, primitives * (verticesPerPrimitive - 2u), ((maxVertices + result.vertexWaveSize - 1u) / result.vertexWaveSize) * result.vertexWaveSize, ((resources >> 19u) & 0xffu) * 128u, 0};
    }
    return result;
}

#define Require(condition, reason) collectOrThrow((condition), std::string(reason))

static State DecodeStateImpl(const QueueState& queue) {
    const auto& cx = queue.context;
    State result{};
    result.stages = DecodeShaderStages(queue);
    const auto primitive = read(queue.userConfig, 0x242, "user-config");
    switch (primitive) {
        case 1: Require(result.stages.mesh.has_value(), "point-list vertex rendering requires point-size output support"); result.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
        case 2: result.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case 7:
        case 17:
            Require(result.stages.path == ShaderPath::Vertex, "rect-list requires vertex routing");
            result.rectList = true;
            result.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            break;
        case 9: result.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; break;
        case 4: result.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
        case 5: result.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
        case 6: result.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
        default: throw std::runtime_error("AGC graphics: unsupported primitive type " + std::to_string(primitive));
    }
    zero(queue.userConfig, 0x24b, ~0u, "primitive restart (GE_MULTI_PRIM_IB_RESET_EN)", "user-config");
    zero(cx, 0x207, ~0u, "clip distances, layer, viewport or auxiliary vertex exports");
    // Z_WRITE_ENABLE (bit 2) alone is unobservable: this decoder never binds a depth target, and Z_ENABLE, STENCIL_ENABLE and
    // DEPTH_BOUNDS_ENABLE (bits 1, 0, 3) stay rejected, so nothing can ever test against what would be written.
    zero(cx, 0x200, ~0x007007f6u, "stencil, depth bounds or conditional color writes");
    zero(cx, 0x203, ~0x00009870u, "depth export, shader coverage or ordered fragment execution");
    zero(cx, 0x2dc, ~0x0001ff00u, "alpha-to-coverage");
    zero(cx, 0x2f8, ~0u, "multisampling or coverage conversion");
    zero(cx, 0x292, ~2u, "scan conversion mode");
    zero(cx, 0x293, ~0x06003fffu, "sample iteration, primitive discard or out-of-order rasterization");
    zero(cx, 0x80, ~0u, "window offset");
    zero(cx, 0x8d, ~0x01ff01ffu, "reserved PA_SU_HARDWARE_SCREEN_OFFSET bits");
    Require(read(cx, 0x83) == 0xffffu, "clip rectangles are unsupported");
    Require((read(cx, 0x8c) & 0xfu) == 0xau, "nonstandard triangle edge rules are unsupported");
    Require(read(cx, 0x2f9) == 0x2du, "nonstandard pixel center or vertex quantization is unsupported");
    Require(read(cx, 0x313) == 0x6000u, "conservative rasterization is unsupported");
    Require(read(cx, 0x30e) == 0xffffffffu && read(cx, 0x30f) == 0xffffffffu, "sample masks are unsupported");
    const auto viewportControl = read(cx, 0x206);
    if (viewportControl != 0x43fu) {
        std::ostringstream message;
        message << "AGC graphics: PA_CL_VTE_CNTL=0x" << std::hex << viewportControl << ": expected 0x43f for homogeneous positions and all viewport transforms; pre-divided coordinates, reciprocal W or disabled transforms are unsupported";
        throw std::runtime_error(message.str());
    }
    zero(cx, 0x204, ~0x80000u, "unsupported PA_CL_CLIP_CNTL flags");
    result.negativeOneToOne = (read(cx, 0x204) & 0x80000u) == 0;
    const auto raster = read(cx, 0x205);
    Require((raster & ~0x7u) == 0 || (raster & ~0x7u) == 0x240u, "polygon mode, depth bias, provoking vertex or nonstandard rasterization is unsupported");
    result.cullMode = ((raster & 1u) != 0 ? VK_CULL_MODE_FRONT_BIT : 0u) | ((raster & 2u) != 0 ? VK_CULL_MODE_BACK_BIT : 0u);
    if (result.rectList) result.cullMode = VK_CULL_MODE_NONE;
    result.frontFace = (raster & 4u) != 0 ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    const auto targetMask = read(cx, 0x8e);
    const auto shaderMask = read(cx, 0x8f);
    // Each CB target owns four mask bits. Only a contiguous range of enabled targets starting at target zero is supported.
    std::uint32_t colorCount = 0;
    while (colorCount < MaxColorTargets && ((targetMask >> (4u * colorCount)) & 0xfu) != 0) ++colorCount;
    if ((targetMask >> (4u * colorCount)) != 0 || (shaderMask >> (4u * colorCount)) != 0) {
        std::ostringstream message;
        message << "only a contiguous range of up to " << MaxColorTargets << " color targets starting at zero is supported: CB_TARGET_MASK=0x" << std::hex << targetMask << ", CB_SHADER_MASK=0x" << shaderMask;
        Require(false, message.str());
    }
    for (std::uint32_t i = 0; i < colorCount; ++i) Require(((shaderMask >> (4u * i)) & 0xfu) == 0xfu, "partial shader color exports are unsupported");
    result.colorCount = colorCount;
    result.hasColorTarget = colorCount != 0;
    Require(read(cx, 0x202) == 0xcc0010u, "only normal color rendering with copy ROP is supported");
    zero(cx, 0x1c4, ~0u, "depth or sample-mask export");
    const auto exportFormat = read(cx, 0x1c5);
    if (colorCount <= 1) {
        Require(exportFormat == 4 || exportFormat == 9, "only FP16_ABGR or 32_ABGR color export is supported");
    } else {
        for (std::uint32_t i = 0; i < colorCount; ++i) {
            const auto nibble = (exportFormat >> (4u * i)) & 0xfu;
            Require(nibble == 4 || nibble == 9, "only FP16_ABGR or 32_ABGR color export is supported");
        }
        Require((exportFormat >> (4u * colorCount)) == 0, "color export formats beyond the enabled targets are unsupported");
    }
    Require(read(cx, 0x1c3) == 4, "additional position exports are unsupported");
    for (std::uint32_t i = 0; i < colorCount; ++i) {
        const auto stride = 15u * i; // CB_COLORn_* registers repeat every 15 dwords
        auto& target = result.colors[i];
        const auto info = read(cx, 0x31c + stride);
        const auto number = (info >> 8u) & 7u;
        const auto swap = (info >> 11u) & 3u;
        const bool half4 = ((info >> 2u) & 0x1fu) == 12 && number == 7 && swap == 0; // COLOR_16_16_16_16, NUMBER_FLOAT
        Require(half4 || (((info >> 2u) & 0x1fu) == 10 && (number == 0 || number == 6) && swap <= 1), "unsupported color format or component order");
        Require((info & ~(half4 ? 0x00041f7cu : 0x00029f7cu)) == 0, "color compression, DCC, endian conversion, nonstandard rounding or color optimization is unsupported");
        Require(half4 || (info & 0x8000u) != 0, "unclamped normalized color is unsupported");
        zero(cx, 0x31b + stride, ~0u, "color mip or array view");
        zero(cx, 0x31d + stride, ~0x20000u, "color samples or fragments"); // FORCE_DST_ALPHA_1 (bit 17) is applied to the blend factors below
        const auto attrib2 = read(cx, 0x3b0 + i);
        Require((attrib2 >> 28u) == 0, "mipmapped render targets are unsupported");
        const auto attrib3 = read(cx, 0x3b8 + i);
        target.tileMode = DecodeColorTileMode(attrib3);
        target.extent = {((attrib2 >> 14u) & 0x3fffu) + 1u, (attrib2 & 0x3fffu) + 1u};
        const ColorTargetLayout colorLayout(target.extent.width, target.extent.height, target.tileMode);
        const auto high = read(cx, 0x390 + i);
        Require((high & ~0xffu) == 0, "invalid color address extension");
        target.address = (static_cast<std::uint64_t>(high) << 40u) | (static_cast<std::uint64_t>(read(cx, 0x318 + stride)) << 8u);
        target.gpuOnly = half4;
        // A GPU-only surface has no guest layout here; its size is the linear lower bound, enough to track the memory it overlays.
        target.bytes = half4 ? static_cast<std::size_t>(target.extent.width) * target.extent.height * 8u : colorLayout.Bytes();
        GuestMemory::CheckGpuRange(reinterpret_cast<const void*>(target.address), target.bytes, colorLayout.Alignment(), true);
        target.format = half4 ? VK_FORMAT_R16G16B16A16_SFLOAT : swap == 0 ? (number == 0 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB) : (number == 0 ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_B8G8R8A8_SRGB);
        target.componentMapping = 0xe4u;
        Require(i == 0 || (target.extent.width == result.colors[0].extent.width && target.extent.height == result.colors[0].extent.height), "color targets of one draw must share an extent");
    }
    if (colorCount != 0) {
        result.color = result.colors[0];
        result.renderExtent = result.colors[0].extent;
    } else {
        const auto screenBottomRight = read(cx, 0xd);
        result.renderExtent = {screenBottomRight & 0xffffu, screenBottomRight >> 16u};
        Require(result.renderExtent.width != 0 && result.renderExtent.height != 0, "empty framebuffer extent for a draw without color writes");
    }
    // Depth: the buffer is GPU-resident. Vulkan ties depth writes to the depth test, so a write-only setup uses an always-pass test.
    {
        const auto depthControl = read(cx, 0x200);
        const bool zEnable = (depthControl & 2u) != 0;
        const bool zWrite = (depthControl & 4u) != 0;
        const auto zFunc = (depthControl >> 4u) & 7u;
        const auto findOr = [&](std::uint32_t offset) {
            const auto it = cx.find(offset);
            return it == cx.end() ? 0u : it->second;
        };
        const auto zFormat = findOr(0x10) & 3u;
        const bool clearEnable = (findOr(0x0) & 1u) != 0; // DB_RENDER_CONTROL.DEPTH_CLEAR_ENABLE: a clear draw needs the target even with test and write off
        if ((zEnable || zWrite || clearEnable) && zFormat != 0) {
            Require(zFormat == 1 || zFormat == 3, "unsupported depth format");
            result.hasDepth = true;
            result.depth.format = zFormat == 3 ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_D16_UNORM;
            result.depth.extent = result.renderExtent;
            result.depth.address = (static_cast<std::uint64_t>(findOr(0x1c)) << 40u) | (static_cast<std::uint64_t>(findOr(0x14)) << 8u);
            static constexpr VkCompareOp compare[8] = {VK_COMPARE_OP_NEVER, VK_COMPARE_OP_LESS, VK_COMPARE_OP_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL, VK_COMPARE_OP_GREATER, VK_COMPARE_OP_NOT_EQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_COMPARE_OP_ALWAYS};
            result.depthTestEnable = true;
            // A clear draw's interpolated Z is not the clear value: it must neither test against nor overwrite the cleared buffer.
            result.depthWriteEnable = zWrite && !clearEnable;
            result.depthCompare = zEnable && !clearEnable ? compare[zFunc] : VK_COMPARE_OP_ALWAYS;
            result.depthClear = clearEnable;
            std::uint32_t clearBits = findOr(0xb);
            std::memcpy(&result.depthClearValue, &clearBits, sizeof(clearBits));
        }
    }
    const auto xs = readFloat(cx, 0x10f);
    const auto xo = readFloat(cx, 0x110);
    const auto ys = readFloat(cx, 0x111);
    const auto yo = readFloat(cx, 0x112);
    const auto zs = readFloat(cx, 0x113);
    const auto zo = readFloat(cx, 0x114);
    const auto minDepth = result.negativeOneToOne ? zo - zs : zo;
    const auto maxDepth = zo + zs;
    if (!(xs > 0 && ys != 0 && std::isfinite(minDepth) && std::isfinite(maxDepth))) {
        std::ostringstream message;
        message << "AGC graphics: unsupported viewport transform: scale=(" << xs << ", " << ys << ", " << zs << "), offset=(" << xo << ", " << yo << ", " << zo << "), depth=(" << minDepth << ", " << maxDepth << "), negativeOneToOne=" << result.negativeOneToOne;
        throw std::runtime_error(message.str());
    }
    Require(readFloat(cx, 0xb4) <= readFloat(cx, 0xb5), "inverted viewport depth clamp bounds");
    result.viewport = {xo - xs, yo - ys, 2 * xs, 2 * ys, minDepth, maxDepth};
    result.scissor = {{0, 0}, result.renderExtent};
    intersect(result.scissor, cx, 0xc, true);
    intersect(result.scissor, cx, 0x81, false);
    intersect(result.scissor, cx, 0x90, false);
    if ((read(cx, 0x292) & 2u) != 0) intersect(result.scissor, cx, 0x94, false);
    for (std::uint32_t i = 0; i < result.colorCount; ++i) {
        const auto stride = 15u * i;
        auto& attachment = result.blends[i];
        const auto blend = read(cx, 0x1e0 + i);
        Require((blend & 0x0000e000u) == 0, "reserved blend control bits");
        attachment.colorWriteMask = (targetMask >> (4u * i)) & 0xfu;
        attachment.blendEnable = (blend >> 30u) & 1u;
        if (attachment.blendEnable) {
            Require((read(cx, 0x31c + stride) & 0x10000u) == 0, "blend bypass conflicts with enabled blending");
            attachment.srcColorBlendFactor = blendFactor(blend & 0x1fu);
            attachment.dstColorBlendFactor = blendFactor((blend >> 8u) & 0x1fu);
            attachment.colorBlendOp = blendOp((blend >> 5u) & 7u);
            const auto alpha = (blend & 0x20000000u) != 0 ? blend >> 16u : blend;
            attachment.srcAlphaBlendFactor = blendFactor(alpha & 0x1fu);
            attachment.dstAlphaBlendFactor = blendFactor((alpha >> 8u) & 0x1fu);
            attachment.alphaBlendOp = blendOp((alpha >> 5u) & 7u);
            if ((read(cx, 0x31d + stride) & 0x20000u) != 0) {
                // CB_COLORn_ATTRIB.FORCE_DST_ALPHA_1: blending reads destination alpha as 1.0.
                const auto forced = [](VkBlendFactor& factor) {
                    if (factor == VK_BLEND_FACTOR_DST_ALPHA) factor = VK_BLEND_FACTOR_ONE;
                    else if (factor == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA) factor = VK_BLEND_FACTOR_ZERO;
                };
                forced(attachment.srcColorBlendFactor);
                forced(attachment.dstColorBlendFactor);
                forced(attachment.srcAlphaBlendFactor);
                forced(attachment.dstAlphaBlendFactor);
            }
        }
    }
    if (result.colorCount != 0) {
        result.blend = result.blends[0];
        for (std::uint32_t i = 0; i < 4; ++i) result.blendConstants[i] = readFloat(cx, 0x105 + i);
    }
    return result;
}

#undef Require

State DecodeState(const QueueState& queue) {
    std::vector<std::string> found;
    violations = &found;
    struct Reset { ~Reset() { violations = nullptr; } } reset;
    State result{};
    try {
        result = DecodeStateImpl(queue);
    } catch (const std::exception& error) {
        std::string message = error.what();
        constexpr std::string_view prefix = "AGC graphics: ";
        if (message.rfind(prefix, 0) == 0) message.erase(0, prefix.size());
        found.push_back(message);
    }
    if (!found.empty()) {
        // Diagnostic: the first few rejected draws dump their whole context register file, so decoding of new state
        // (depth, multiple render targets) can be built on real values.
        static int registerDumps = 0;
        if (registerDumps < 4) {
            ++registerDumps;
            std::string dump = "[STATE] rejected draw context registers:";
            for (const auto& [offset, value] : queue.context) {
                char item[40];
                std::snprintf(item, sizeof(item), " %x=%x", offset, value);
                dump += item;
            }
            std::fprintf(stderr, "%s\n", dump.c_str());
            std::string shader = "[STATE] rejected draw shader registers:";
            for (const auto& [offset, value] : queue.shader) {
                char item[40];
                std::snprintf(item, sizeof(item), " %x=%x", offset, value);
                shader += item;
            }
            std::fprintf(stderr, "%s\n", shader.c_str());
            std::fflush(stderr);
        }
        std::string joined;
        for (const auto& entry : found) joined += (joined.empty() ? "" : " | ") + entry;
        throw std::runtime_error("AGC graphics: " + joined);
    }
    return result;
}

}
