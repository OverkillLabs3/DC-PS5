#include "prx/libSceAgcDriver/Graphics/include/RenderCache.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DrawQueue.hpp"
#include "prx/libSceAgcDriver/Execution/include/MemoryAccessScope.hpp"
#include "prx/libSceAgcDriver/Execution/include/PerformanceTimer.hpp"
#include <cmath>
#include <limits>

namespace AgcDriver::Graphics {

void ResidentColor::Transition(VkCommandBuffer commands, VkImageLayout next) {
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = layout == VK_IMAGE_LAYOUT_UNDEFINED ? 0u : VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.oldLayout = layout;
    barrier.newLayout = next;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = Target().Image();
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    context.Function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    layout = next;
}

void ResidentColor::Begin(VkCommandBuffer commands) {
    PerformanceTimer timing("Graphics.ResidentColor.Begin");
    Require(generation != std::numeric_limits<std::uint64_t>::max(), "resident color generation overflow");
    ++generation;
    Require(memoryWatch != nullptr, "render target memory ownership was released");
    if (!valid && color.gpuOnly) {
        memoryWatch->Protect(GuestMemoryTracking::Protection::Read);
        Transition(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        const VkClearColorValue zero{};
        const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        context.Function<PFN_vkCmdClearColorImage>("vkCmdClearColorImage")(commands, Target().Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &zero, 1, &range);
        timing.Mark("clear");
    } else if (!valid) {
        memoryWatch->Protect(GuestMemoryTracking::Protection::Read);
        timing.Mark("upload_protect");
        const GuestMemory::MemoryAccessScope suspended(nullptr, nullptr);
        transfer.Upload(color.address, color.extent.width, color.extent.height, color.tileMode);
        timing.Mark("upload_read");
        transfer.Detile(commands);
        timing.Mark("upload_detile_record");
        Transition(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {color.extent.width, color.extent.height, 1};
        context.Function<PFN_vkCmdCopyBufferToImage>("vkCmdCopyBufferToImage")(commands, transfer.LinearBuffer(), Target().Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        timing.Mark("upload", color.bytes);
        CountEvent(FrameTiming::RtUploads);
    } else {
        timing.Mark("reuse");
    }
    Transition(commands, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    valid = true;
    dirty = true;
    memoryWatch->Protect(GuestMemoryTracking::Protection::None);
}

void ResidentColor::ClearColor(VkCommandBuffer commands, const VkClearColorValue& value) {
    PerformanceTimer timing("Graphics.ResidentColor.Clear");
    Require(generation != std::numeric_limits<std::uint64_t>::max(), "resident color generation overflow");
    Require(memoryWatch != nullptr, "render target memory ownership was released");
    ++generation;
    Transition(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    context.Function<PFN_vkCmdClearColorImage>("vkCmdClearColorImage")(commands, Target().Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);
    Transition(commands, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    valid = true;
    dirty = true;
    memoryWatch->Protect(GuestMemoryTracking::Protection::None);
}

bool DecodeFillPattern(VkFormat format, std::uint32_t dword, VkClearColorValue& out) {
    const auto b0 = dword & 0xffu, b1 = (dword >> 8u) & 0xffu, b2 = (dword >> 16u) & 0xffu, b3 = dword >> 24u;
    const auto unorm = [](std::uint32_t byte) { return static_cast<float>(byte) / 255.0f; };
    // sRGB encoding is only exact at the ends of the range; alpha is always linear.
    const auto exactSrgb = [](std::uint32_t byte) { return byte == 0u || byte == 255u; };
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            out.float32[0] = unorm(b0); out.float32[1] = unorm(b1); out.float32[2] = unorm(b2); out.float32[3] = unorm(b3);
            return true;
        case VK_FORMAT_B8G8R8A8_UNORM:
            out.float32[0] = unorm(b2); out.float32[1] = unorm(b1); out.float32[2] = unorm(b0); out.float32[3] = unorm(b3);
            return true;
        case VK_FORMAT_R8G8B8A8_SRGB:
            if (!exactSrgb(b0) || !exactSrgb(b1) || !exactSrgb(b2)) return false;
            out.float32[0] = unorm(b0); out.float32[1] = unorm(b1); out.float32[2] = unorm(b2); out.float32[3] = unorm(b3);
            return true;
        case VK_FORMAT_B8G8R8A8_SRGB:
            if (!exactSrgb(b0) || !exactSrgb(b1) || !exactSrgb(b2)) return false;
            out.float32[0] = unorm(b2); out.float32[1] = unorm(b1); out.float32[2] = unorm(b0); out.float32[3] = unorm(b3);
            return true;
        case VK_FORMAT_R16G16B16A16_SFLOAT: {
            // A 64-bit texel is two identical dwords: halves h0,h1,h0,h1.
            const auto half = [](std::uint32_t bits, float& result) {
                const auto sign = (bits >> 15u) & 1u;
                const auto exponent = (bits >> 10u) & 0x1fu;
                const auto mantissa = bits & 0x3ffu;
                if (exponent == 0x1fu) return false; // inf / NaN: leave to the exact memory path
                float magnitude = exponent == 0 ? std::ldexp(static_cast<float>(mantissa), -24) : std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, static_cast<int>(exponent) - 15);
                result = sign != 0 ? -magnitude : magnitude;
                return true;
            };
            return half(dword & 0xffffu, out.float32[0]) && half(dword >> 16u, out.float32[1]) && half(dword & 0xffffu, out.float32[2]) && half(dword >> 16u, out.float32[3]);
        }
        default: return false;
    }
}

void ResidentColor::Download(VkCommandBuffer commands) {
    if (!dirty || color.gpuOnly) return; // a GPU-only surface has nothing to write back
    CountEvent(FrameTiming::RtDownloads);
    Transition(commands, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkMemoryBarrier reuse{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    reuse.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    reuse.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    context.Function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &reuse, 0, nullptr, 0, nullptr);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {color.extent.width, color.extent.height, 1};
    context.Function<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer")(commands, Target().Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, transfer.LinearBuffer(), 1, &copy);
    transfer.Tile(commands);
}

void ResidentColor::Commit() {
    if (!dirty) return;
    const GuestMemory::MemoryAccessScope suspended(nullptr, nullptr);
    if (!color.gpuOnly) transfer.WriteBackTracked(color.address);
    dirty = false;
    memoryWatch->Protect(GuestMemoryTracking::Protection::Read);
}

std::shared_ptr<ResidentColor> RenderCache::Get(const ColorTarget& color, bool blending) {
    Require(color.address != 0 && color.bytes != 0 && color.bytes <= std::numeric_limits<std::uint64_t>::max() - color.address, "invalid resident color range");
    if (context.drawQueue) context.drawQueue->Resolve(color.address, color.bytes);
    if (blending) {
        VkFormatProperties properties{};
        context.formatProperties(context.physical, color.format, &properties);
        Require((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) != 0, "render target format does not support blending");
    }
    for (auto it = entries.begin(); it != entries.end();) {
        const auto& previous = it->second->Description();
        if (!it->second->SharesPages(color)) {
            ++it;
            continue;
        }
        if (previous.address == color.address && previous.bytes == color.bytes && previous.extent.width == color.extent.width && previous.extent.height == color.extent.height && previous.format == color.format && previous.tileMode == color.tileMode) return it->second;
        Resolve(previous.address, previous.bytes, true);
        it->second->ReleaseMemory();
        it = entries.erase(it);
    }
    if (entries.size() >= 64) {
        Flush();
        for (const auto& [address, resident] : entries) resident->ReleaseMemory();
        entries.clear();
    }
    static int createDiag = 0;
    if (createDiag++ < 80) std::fprintf(stderr, "[RCREATE] addr=%llx bytes=%llu %ux%u fmt=%u tile=%u gpuOnly=%d\n", static_cast<unsigned long long>(color.address), static_cast<unsigned long long>(color.bytes), color.extent.width, color.extent.height, static_cast<unsigned>(color.format), static_cast<unsigned>(color.tileMode), static_cast<int>(color.gpuOnly));
    auto entry = std::make_shared<ResidentColor>(context, color);
    entries.emplace(color.address, entry);
    return entry;
}

std::shared_ptr<DepthImage> RenderCache::GetDepth(const DepthTarget& depth) {
    Require(depth.address != 0, "invalid depth buffer address");
    const auto found = depths.find(depth.address);
    if (found != depths.end()) {
        const auto& existing = found->second->Description();
        if (existing.extent.width == depth.extent.width && existing.extent.height == depth.extent.height && existing.format == depth.format) return found->second;
        depths.erase(found); // the guest reused the address for a different surface; in-flight draws keep the old image alive
    }
    if (depths.size() >= 16) depths.clear();
    auto image = std::make_shared<DepthImage>(context, depth);
    depths.emplace(depth.address, image);
    return image;
}

std::shared_ptr<ResidentColor> RenderCache::Find(std::uint64_t address) const {
    const auto it = entries.find(address);
    if (it != entries.end() && context.drawQueue) context.drawQueue->Resolve(address, it->second->Description().bytes);
    return it != entries.end() && it->second->Valid() ? it->second : nullptr;
}

void RenderCache::Resolve(std::uint64_t address, std::size_t bytes, bool writable) {
    std::vector<std::shared_ptr<ResidentColor>> affected;
    for (const auto& [base, entry] : entries) {
        const auto& color = entry->Description();
        if (address >= base + color.bytes || base >= address + bytes) continue;
        if (entry->Dirty()) affected.push_back(entry);
        else if (writable) entry->Invalidate();
    }
    if (affected.empty()) return;
    PerformanceTimer timing("Graphics.RenderCache.Resolve");
    if (context.drawQueue) context.drawQueue->Wait("rendercache_resolve");
    CommandBatch batch(context);
    for (const auto& entry : affected) entry->Download(batch.Handle());
    batch.SubmitAndWait();
    timing.Mark("download_wait");
    for (const auto& entry : affected) {
        entry->Commit();
        if (writable) entry->Invalidate();
    }
    timing.Mark("guest_writeback");
}

void RenderCache::Flush() {
    Resolve(0, std::numeric_limits<std::size_t>::max(), true);
}

}

namespace AgcDriver::Graphics {
std::string RenderCache::DebugDump(std::uint64_t address) const {
    std::string out;
    char line[200];
    for (const auto& [base, entry] : entries) {
        const auto& d = entry->Description();
        const bool overlap = address >= d.address && address < d.address + d.bytes;
        std::snprintf(line, sizeof(line), " [%llx+%llx %ux%u v=%d d=%d%s]", static_cast<unsigned long long>(d.address), static_cast<unsigned long long>(d.bytes), d.extent.width, d.extent.height, static_cast<int>(entry->Valid()), static_cast<int>(entry->Dirty()), overlap ? " OVERLAP" : "");
        out += line;
    }
    return out;
}
}
