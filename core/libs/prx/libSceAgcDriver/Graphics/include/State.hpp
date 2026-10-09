#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_STATE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_STATE_HPP

#include "prx/libSceAgcDriver/Graphics/include/Context.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ColorTargetLayout.hpp"
#include "prx/libSceAgcDriver/Execution/include/QueueState.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include "Recompiler.hpp"

namespace AgcDriver::Graphics {

enum class ShaderPath {
    Vertex,
    Geometry,
    Tessellation,
    TessellationGeometry
};

struct ShaderStages {
    ShaderPath path;
    std::uint32_t registerValue;
    std::uint32_t vertexWaveSize;
    std::uint32_t fragmentWaveSize;
    std::optional<ShaderRecompiler::MeshConfiguration> mesh;
    std::optional<ShaderRecompiler::TessellationConfiguration> tessellation;
};

struct ColorTarget {
    std::uint64_t address;
    VkExtent2D extent;
    VkFormat format;
    std::size_t bytes;
    std::uint8_t componentMapping;
    ColorTileMode tileMode = ColorTileMode::Linear;
    // Transient surface with no guest layout implementation (for example RGBA16F accumulation buffers): it is never uploaded from
    // or written back to guest memory. It starts cleared, and any guest write to its memory resets it to cleared.
    bool gpuOnly = false;
};

// Render targets a single draw can write: the enabled CB targets are always the contiguous range 0..colorCount-1.
constexpr std::uint32_t MaxColorTargets = 4;

// A depth buffer is GPU-resident (it is never written back to guest memory); it only lives as long as the guest keeps
// rendering with the same base address, extent and format.
struct DepthTarget {
    std::uint64_t address = 0;
    VkExtent2D extent{};
    VkFormat format = VK_FORMAT_UNDEFINED;
};

struct State {
    ShaderStages stages;
    ColorTarget color;
    bool hasColorTarget;
    std::uint32_t colorCount = 0;
    std::array<ColorTarget, MaxColorTargets> colors{};
    std::array<VkPipelineColorBlendAttachmentState, MaxColorTargets> blends{};
    bool hasDepth = false;
    DepthTarget depth;
    bool depthTestEnable = false;
    bool depthWriteEnable = false;
    VkCompareOp depthCompare = VK_COMPARE_OP_ALWAYS;
    bool depthClear = false;
    float depthClearValue = 1.0f;
    bool rectList = false;
    VkExtent2D renderExtent;
    VkPrimitiveTopology topology;
    VkViewport viewport;
    bool negativeOneToOne;
    VkRect2D scissor;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    VkPipelineColorBlendAttachmentState blend;
    std::array<float, 4> blendConstants;
};

ShaderStages DecodeShaderStages(const QueueState& queue);
State DecodeState(const QueueState& queue);

}

#endif
