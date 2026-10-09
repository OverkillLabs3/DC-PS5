#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_CONTEXT_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_CONTEXT_HPP

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace AgcDriver::Graphics {

// vkGetDeviceProcAddr hashes the name on every call and the draw path resolves dozens of entry points per draw; resolved pointers are
// memoised per thread by (device, name address). The stored copy of the name guards against a reused temporary string address.
inline void* CachedDeviceProc(PFN_vkGetDeviceProcAddr proc, VkDevice device, const char* name) {
    struct Entry {
        std::string name;
        void* function;
    };
    struct Key {
        VkDevice device;
        const char* name;
        bool operator==(const Key& other) const { return device == other.device && name == other.name; }
    };
    struct Hash {
        std::size_t operator()(const Key& key) const { return std::hash<const void*>()(key.device) * 31u + std::hash<const void*>()(key.name); }
    };
    thread_local std::unordered_map<Key, Entry, Hash> cache;
    const Key key{device, name};
    const auto found = cache.find(key);
    if (found != cache.end() && found->second.name == name) return found->second.function;
    const auto function = reinterpret_cast<void*>(proc(device, name));
    if (function != nullptr) cache[key] = Entry{name, function};
    return function;
}

class TextureDetiler;
class GpuColorTransfer;
class BufferPool;
class TextureCache;
class RenderCache;
class DrawQueue;
class GraphicsPipelineCache;
class DescriptorCache;
class SamplerCache;

inline void Require(bool condition, const std::string& reason) {
    if (!condition) throw std::runtime_error("AGC graphics: " + reason);
}

inline void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) throw std::runtime_error(std::string("AGC graphics: ") + operation + ": Vulkan result " + std::to_string(result));
}

inline void Require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(std::string("AGC graphics: ") + reason);
}

struct Context {
    VkDevice device;
    VkPhysicalDevice physical;
    VkQueue queue;
    VkCommandPool pool;
    PFN_vkGetDeviceProcAddr deviceProc;
    PFN_vkGetPhysicalDeviceFormatProperties formatProperties;
    PFN_vkGetPhysicalDeviceImageFormatProperties imageFormatProperties;
    VkPhysicalDeviceMemoryProperties memory;
    VkPhysicalDeviceLimits limits;
    bool tessellationShader = false;
    bool meshShader = false;
    VkPhysicalDeviceMeshShaderPropertiesEXT meshLimits{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT};
    bool depthClipControl = false;
    bool depthRangeUnrestricted = false;
    bool bufferDeviceAddress = false;
    VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    bool fragmentShaderBarycentric = false;
    bool samplerAnisotropy = false;
    bool textureCompressionBC = false;
    TextureDetiler* detiler = nullptr;
    GpuColorTransfer* colorTransfer = nullptr;
    mutable std::shared_ptr<BufferPool> bufferPool;
    TextureCache* textureCache = nullptr;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    RenderCache* renderCache = nullptr;
    DrawQueue* drawQueue = nullptr;
    GraphicsPipelineCache* graphicsPipelines = nullptr;
    mutable std::shared_ptr<DescriptorCache> descriptorCache;
    mutable std::shared_ptr<SamplerCache> samplerCache;

    template<typename TFunction>
    TFunction Function(const char* name) const {
        Require(deviceProc != nullptr, "missing Vulkan device function resolver");
        const auto function = reinterpret_cast<TFunction>(CachedDeviceProc(deviceProc, device, name));
        if (function == nullptr) throw std::runtime_error(std::string("AGC graphics: missing Vulkan function: ") + name);
        return function;
    }

    std::uint32_t MemoryType(std::uint32_t mask, VkMemoryPropertyFlags flags) const {
        // Host-visible system memory without HOST_CACHED is write-combined: CPU reads from it (guest write-back of GPU results)
        // are orders of magnitude slower. Prefer a cached type when one satisfies the request.
        if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 && (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0) {
            const VkMemoryPropertyFlags preferred = flags | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
                if ((mask & (1u << i)) != 0 && (memory.memoryTypes[i].propertyFlags & preferred) == preferred) return i;
            }
        }
        for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
            if ((mask & (1u << i)) != 0 && (memory.memoryTypes[i].propertyFlags & flags) == flags) return i;
        }
        throw std::runtime_error("AGC graphics: required Vulkan memory type is unavailable");
    }
};

}

#endif
