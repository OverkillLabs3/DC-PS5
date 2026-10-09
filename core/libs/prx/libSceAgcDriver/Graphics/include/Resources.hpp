#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_RESOURCES_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_RESOURCES_HPP

#include <atomic>
#include "prx/libSceAgcDriver/Graphics/include/State.hpp"
#include <span>

namespace AgcDriver::Graphics {

class Buffer {
public:
    Buffer(const Context& context, std::size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ~Buffer();
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    VkBuffer Handle() const;
    VkDeviceAddress DeviceAddress() const;
    std::span<std::byte> Bytes();
    void Invalidate();

private:
    void initializeAddress(VkBufferUsageFlags usage);
    void release() noexcept;
    Context context;
    VkDeviceAddress deviceAddress = 0;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapping = nullptr;
    std::size_t size;
    std::size_t capacity; // pooled VkBuffer size: `size` rounded up into a reuse bucket
    VkDeviceSize allocationBytes = 0;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
    bool reusable = false;
    std::shared_ptr<BufferPool> cache;
};

class RenderTarget {
public:
    RenderTarget(const Context& context, const ColorTarget& target, bool blending);
    ~RenderTarget();
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    VkImage Image() const;
    VkImageView View() const;
    // True when the image can also be sampled directly, so later passes can read it without a copy.
    bool Sampleable() const { return sampleable; }

private:
    void release() noexcept;
    Context context;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    bool sampleable = false;
};

// GPU-resident depth buffer. Guest memory never sees it, so it is created on first use and lives while the guest keeps
// rendering with the same base address, extent and format.
// Advanced by the driver at every presented flip; a depth buffer the guest never clears explicitly is cleared on its first use in each frame.
extern std::atomic<std::uint64_t> DepthFrameEpoch;

class DepthImage {
public:
    DepthImage(const Context& context, const DepthTarget& target);
    ~DepthImage();
    DepthImage(const DepthImage&) = delete;
    DepthImage& operator=(const DepthImage&) = delete;
    VkImage Image() const { return image; }
    VkImageView View() const { return view; }
    const DepthTarget& Description() const { return target; }
    // Records the transition (and the clear, on first use or when the guest requested one) needed before a pass that loads depth.
    void Prepare(VkCommandBuffer commands, bool clear, float clearValue);

private:
    void release() noexcept;
    Context context;
    DepthTarget target;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    bool initialized = false;
    std::uint64_t clearedEpoch = ~0ull;
};

class CommandBatch {
public:
    explicit CommandBatch(const Context& context);
    ~CommandBatch();
    CommandBatch(const CommandBatch&) = delete;
    CommandBatch& operator=(const CommandBatch&) = delete;
    VkCommandBuffer Handle() const;
    void SubmitAndWait();
    void Submit();
    void Wait();
    bool IsComplete();
    void Reset();

private:
    void release() noexcept;
    Context context;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool pending = false;
    bool submitted = false;
};

}

#endif
