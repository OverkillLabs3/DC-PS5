#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_RENDERCACHE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_RENDERCACHE_HPP

#include <string>
#include "prx/libSceAgcDriver/Graphics/include/GpuColorTransfer.hpp"
#include "prx/libc/include/GuestMemoryTracking.hpp"
#include <map>

namespace AgcDriver::Graphics {

class ResidentColor {
public:
    ResidentColor(const Context& context, const ColorTarget& color);
    ~ResidentColor();
    void Begin(VkCommandBuffer commands);
    // Replaces the whole surface with one colour on the GPU (a guest fill of the entire target), leaving guest memory stale until it is read.
    void ClearColor(VkCommandBuffer commands, const VkClearColorValue& value);
    void Download(VkCommandBuffer commands);
    void Commit();
    void Transition(VkCommandBuffer commands, VkImageLayout layout);
    RenderTarget& Target() { return transfer.Target(color, false); }
    const ColorTarget& Description() const { return color; }
    bool Valid() const { return valid; }
    bool Dirty() const { return dirty; }
    std::uint64_t Generation() const { return generation; }
    void Invalidate();
    void ReleaseMemory();
    bool SharesPages(const ColorTarget& other) const;

private:
    void resolveCpuAccess(GuestMemoryTracking::Access access);
    Context context;
    ColorTarget color;
    GpuColorTransfer transfer;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool valid = false;
    bool dirty = false;
    std::uint64_t generation = 0;
    std::unique_ptr<GuestMemoryTracking::Watch> memoryWatch;
};

// Decodes a buffer-fill dword into the clear colour of a render-target format; false when the format or value cannot be reproduced exactly.
bool DecodeFillPattern(VkFormat format, std::uint32_t dword, VkClearColorValue& out);

class RenderCache {
public:
    explicit RenderCache(const Context& context) : context(context) {}
    ~RenderCache();
    std::shared_ptr<ResidentColor> Get(const ColorTarget& color, bool blending);
    std::shared_ptr<ResidentColor> Find(std::uint64_t address) const;
    // The entry at this base address regardless of validity (a target about to be drawn into by the current draw).
    std::shared_ptr<ResidentColor> FindEntry(std::uint64_t address) const { const auto it = entries.find(address); return it != entries.end() ? it->second : nullptr; }
    std::shared_ptr<DepthImage> GetDepth(const DepthTarget& depth);
    void Resolve(std::uint64_t address, std::size_t bytes, bool writable);
    void Flush();
    // TEMPORARY MEASUREMENT
    std::string DebugDump(std::uint64_t address) const;

private:
    Context context;
    std::map<std::uint64_t, std::shared_ptr<ResidentColor>> entries;
    std::map<std::uint64_t, std::shared_ptr<DepthImage>> depths;
};

}

#endif
