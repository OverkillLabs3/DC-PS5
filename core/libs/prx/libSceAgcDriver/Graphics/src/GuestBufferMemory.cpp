#include "prx/libSceAgcDriver/Graphics/include/GuestBufferMemory.hpp"
#include "prx/libc/include/VerboseLog.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/PerformanceTimer.hpp"
#include <atomic>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <limits>

namespace AgcDriver::Graphics {

GuestBufferMemory::GuestBufferMemory(const Context& context) : context(context) {}

bool GuestBufferMemory::WritesOverlap(std::uint64_t address, std::size_t bytes) const {
    return std::any_of(writes.begin(), writes.end(), [&](const auto& range) { return address < range.second && range.first < address + bytes; });
}

void GuestBufferMemory::validate(std::uint64_t address, std::size_t bytes) const {
    Require(!uploaded, "guest memory ownership is frozen for GPU execution");
    Require(address != 0 && bytes != 0, "empty guest memory range");
    Require(bytes <= std::numeric_limits<std::uint64_t>::max() - address, "guest memory range overflow");
}

void GuestBufferMemory::AddWritable(std::uint64_t address, std::size_t bytes) {
    validate(address, bytes);
    GuestMemory::CheckRange(reinterpret_cast<const void*>(address), bytes, 1, true);
    regions.push_back({address, address + bytes, true, {}, nullptr, nullptr});
    writes.emplace_back(address, address + bytes);
}

void GuestBufferMemory::AddReadOnly(std::uint64_t address, std::size_t bytes) {
    validate(address, bytes);
    GuestMemory::CheckRange(reinterpret_cast<const void*>(address), bytes, 1);
    regions.push_back({address, address + bytes, false, {}, nullptr, nullptr, true});
}

void GuestBufferMemory::AddSnapshot(const GuestMemorySnapshot& snapshot) {
    validate(snapshot.address, snapshot.bytes.size());
    for (const auto& region : regions) {
        if (region.begin <= snapshot.address && snapshot.address + snapshot.bytes.size() <= region.end) {
            const auto offset = static_cast<std::size_t>(snapshot.address - region.begin);
            const auto* source = region.writable || region.guest ? reinterpret_cast<const std::byte*>(snapshot.address) : region.snapshot.data() + offset;
            Require(std::memcmp(source, snapshot.bytes.data(), snapshot.bytes.size()) == 0, "guest snapshot differs from registered memory");
            return;
        }
    }
    regions.push_back({snapshot.address, snapshot.address + snapshot.bytes.size(), false, {snapshot.bytes.begin(), snapshot.bytes.end()}, nullptr, nullptr});
}

void GuestBufferMemory::Upload(bool addressable) {
    Require(!uploaded, "guest memory was already uploaded");
    uploaded = true;
    std::sort(regions.begin(), regions.end(), [](const Region& left, const Region& right) { return left.begin < right.begin; });
    std::vector<Region> merged;
    for (auto& region : regions) {
        if (!merged.empty() && region.begin < merged.back().end) {
            auto& previous = merged.back();
            if (previous.writable != region.writable) {
                // The same memory bound both writable and read-only: the writable region absorbs the read-only guest one. An immutable snapshot cannot be absorbed.
                const auto& readOnly = previous.writable ? region : previous;
                Require(readOnly.guest, "writable guest memory overlaps an immutable snapshot");
                previous.writable = true;
                previous.guest = false;
                previous.snapshot.clear();
            } else if (!region.writable && (previous.guest || region.guest)) {
                previous.guest = true; // read from guest memory; any snapshot bytes are superseded
                previous.snapshot.clear();
            } else if (!region.writable) {
                const auto offset = static_cast<std::size_t>(region.begin - previous.begin);
                const auto overlap = static_cast<std::size_t>(std::min(previous.end, region.end) - region.begin);
                Require(std::memcmp(previous.snapshot.data() + offset, region.snapshot.data(), overlap) == 0, "inconsistent overlapping guest snapshots");
                if (region.end > previous.end) previous.snapshot.insert(previous.snapshot.end(), region.snapshot.begin() + overlap, region.snapshot.end());
            }
            previous.end = std::max(previous.end, region.end);
        } else {
            merged.push_back(std::move(region));
        }
    }
    regions = std::move(merged);
    for (auto& region : regions) {
        const auto bytes = region.end - region.begin;
        Require(bytes <= std::numeric_limits<std::size_t>::max(), "guest GPU allocation size overflow");
        const auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | (addressable ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0u);
        region.buffer = std::make_unique<Buffer>(context, static_cast<std::size_t>(bytes), usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        region.staging = std::make_unique<Buffer>(context, static_cast<std::size_t>(bytes), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        if (region.writable || region.guest) {
            // Every source range was CheckRange'd (readable or writable) when it was registered a moment ago on this thread, and merging only unions
            // overlapping ranges, so the second range check GuestMemory::Read performs is redundant work (~1.8 ms/frame at ~200 draws).
            const auto staged = region.staging->Bytes();
            std::memcpy(staged.data(), reinterpret_cast<const void*>(region.begin), staged.size());
            CountEvent(FrameTiming::ReadBytes, staged.size());
        }
        else std::memcpy(region.staging->Bytes().data(), region.snapshot.data(), region.snapshot.size());
        region.snapshot.clear();
    }
}

void GuestBufferMemory::RecordUpload(VkCommandBuffer commands) {
    Require(uploaded && !committed, "guest memory is not available for recording");
    for (const auto& region : regions) {
        Require(region.buffer != nullptr && region.staging != nullptr, "guest GPU upload is incomplete");
        const VkBufferCopy copy{0, 0, region.end - region.begin};
        context.Function<PFN_vkCmdCopyBuffer>("vkCmdCopyBuffer")(commands, region.staging->Handle(), region.buffer->Handle(), 1, &copy);
    }
    if (regions.empty()) return;
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
    context.Function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void GuestBufferMemory::RecordDownload(VkCommandBuffer commands) {
    Require(uploaded && !committed, "guest memory is not available for recording");
    downloadRecorded = true;
    downloaded.clear();
    auto ranges = writes;
    std::sort(ranges.begin(), ranges.end());
    for (const auto& range : ranges) {
        if (!downloaded.empty() && range.first < downloaded.back().second) downloaded.back().second = std::max(downloaded.back().second, range.second);
        else downloaded.push_back(range);
    }
    if (downloaded.empty()) return;
    VkMemoryBarrier before{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    before.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    before.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    context.Function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &before, 0, nullptr, 0, nullptr);
    for (const auto& [begin, end] : downloaded) {
        const auto found = std::upper_bound(regions.begin(), regions.end(), begin, [](auto address, const auto& region) { return address < region.begin; });
        Require(found != regions.begin(), "guest download range has no GPU owner");
        const auto& region = *std::prev(found);
        Require(region.buffer != nullptr && region.staging != nullptr && region.writable && end <= region.end, "guest download range exceeds its GPU owner");
        const VkBufferCopy copy{begin - region.begin, begin - region.begin, end - begin};
        context.Function<PFN_vkCmdCopyBuffer>("vkCmdCopyBuffer")(commands, region.buffer->Handle(), region.staging->Handle(), 1, &copy);
    }
    VkMemoryBarrier after{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.Function<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &after, 0, nullptr, 0, nullptr);
}

VkDescriptorBufferInfo GuestBufferMemory::Descriptor(std::uint64_t address, std::size_t bytes) const {
    Require(uploaded && !committed, "guest GPU memory is not available");
    Require(bytes != 0 && bytes <= std::numeric_limits<std::uint64_t>::max() - address, "invalid guest buffer view");
    const auto found = std::upper_bound(regions.begin(), regions.end(), address, [](std::uint64_t value, const Region& region) { return value < region.begin; });
    Require(found != regions.begin(), "guest buffer has no GPU owner");
    const auto& region = *std::prev(found);
    Require(address >= region.begin && address + bytes <= region.end && region.buffer != nullptr, "guest buffer view exceeds its GPU owner");
    const auto offset = address - region.begin;
    Require(context.limits.minStorageBufferOffsetAlignment != 0 && offset % context.limits.minStorageBufferOffsetAlignment == 0, "guest buffer view violates storage buffer offset alignment");
    Require(bytes <= context.limits.maxStorageBufferRange, "guest buffer view exceeds descriptor range limit");
    return {region.buffer->Handle(), offset, bytes};
}

std::vector<ShaderRecompiler::BdaAbi::Range> GuestBufferMemory::AddressRanges() const {
    Require(uploaded && !committed, "guest GPU address ranges are not available");
    std::vector<ShaderRecompiler::BdaAbi::Range> result;
    for (const auto& region : regions) {
        Require(region.buffer != nullptr, "incomplete guest GPU upload");
        const auto address = region.buffer->DeviceAddress();
        Require(region.end - region.begin <= std::numeric_limits<std::uint64_t>::max() - address, "GPU address range overflow");
        result.push_back({region.begin, region.end, address, ShaderRecompiler::BdaAbi::Read, 0});
    }
    return result;
}

void GuestBufferMemory::WriteBack() {
    Require(uploaded && !committed, "guest memory cannot be committed twice or before upload");
    Require(downloadRecorded || writes.empty(), "guest memory writes were never staged for readback");
    for (const auto& [begin, end] : downloaded) {
        GuestMemory::CheckRange(reinterpret_cast<const void*>(begin), static_cast<std::size_t>(end - begin), 1, true);
        const auto found = std::upper_bound(regions.begin(), regions.end(), begin, [](auto address, const auto& region) { return address < region.begin; });
        Require(found != regions.begin(), "write-back range has no GPU owner");
        const auto& region = *std::prev(found);
        Require(region.staging != nullptr && region.writable && end <= region.end, "write-back range exceeds its GPU owner");
        GuestMemory::Write(begin, region.staging->Bytes().subspan(static_cast<std::size_t>(begin - region.begin), static_cast<std::size_t>(end - begin)));
    }
    committed = true;
    lease.clear();
}

}
