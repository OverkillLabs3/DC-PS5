#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_GUESTBUFFERMEMORY_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_GUESTBUFFERMEMORY_HPP

#include "prx/libSceAgcDriver/Graphics/include/Resources.hpp"
#include "BdaAbi.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include <memory>
#include <vector>

namespace AgcDriver::Graphics {

struct GuestMemorySnapshot {
    std::uint64_t address;
    std::span<const std::byte> bytes;
};

class GuestBufferMemory {
public:
    explicit GuestBufferMemory(const Context& context);
    void AcquireRegistered();
    void AddWritable(std::uint64_t address, std::size_t bytes);
    // Guest memory the shader can only read: staged from guest memory at upload but never downloaded or written back.
    void AddReadOnly(std::uint64_t address, std::size_t bytes);
    void AddSnapshot(const GuestMemorySnapshot& snapshot);
    void Upload(bool addressable);
    // Guest shader memory lives in device-local memory; these record the staged copies that
    // move it across the PCIe boundary once per submission instead of per shader access.
    void RecordUpload(VkCommandBuffer commands);
    void RecordDownload(VkCommandBuffer commands);
    VkDescriptorBufferInfo Descriptor(std::uint64_t address, std::size_t bytes) const;
    std::vector<ShaderRecompiler::BdaAbi::Range> AddressRanges() const;
    void WriteBack();
    bool WritesOverlap(std::uint64_t address, std::size_t bytes) const;

private:
    struct Region {
        std::uint64_t begin;
        std::uint64_t end;
        bool writable;
        std::vector<std::byte> snapshot;
        std::unique_ptr<Buffer> buffer;
        std::unique_ptr<Buffer> staging;
        bool guest = false; // not writable, but read from guest memory at upload instead of from `snapshot`
    };

    void validate(std::uint64_t address, std::size_t bytes) const;
    Context context;
    GuestAllocations::Lease lease;
    std::vector<Region> regions;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> writes;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> downloaded;
    bool uploaded = false;
    bool downloadRecorded = false;
    bool committed = false;
};

}

#endif
