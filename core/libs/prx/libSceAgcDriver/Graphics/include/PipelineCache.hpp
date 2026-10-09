#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_PIPELINECACHE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_PIPELINECACHE_HPP

#include "prx/libSceAgcDriver/Graphics/include/Context.hpp"
#include <cstdio>
#include <vector>

namespace AgcDriver::Graphics {

class PipelineCache {
public:
    // The cache is persisted in pipeline_cache.bin: pipelines seen in an earlier run are not compiled again (Vulkan itself rejects data written by a
    // different driver or device, so a stale file is harmless).
    explicit PipelineCache(const Context& context) : context(context) {
        std::vector<char> initial;
        if (std::FILE* file = std::fopen("pipeline_cache.bin", "rb")) {
            std::fseek(file, 0, SEEK_END);
            const long size = std::ftell(file);
            std::fseek(file, 0, SEEK_SET);
            if (size > 0 && size < (1l << 30)) {
                initial.resize(static_cast<std::size_t>(size));
                if (std::fread(initial.data(), 1, initial.size(), file) != initial.size()) initial.clear();
            }
            std::fclose(file);
        }
        VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        info.initialDataSize = initial.size();
        info.pInitialData = initial.empty() ? nullptr : initial.data();
        Check(context.Function<PFN_vkCreatePipelineCache>("vkCreatePipelineCache")(context.device, &info, nullptr, &cache), "vkCreatePipelineCache");
        std::fprintf(stderr, "[PIPECACHE] loaded %zu bytes from pipeline_cache.bin\n", initial.size());
        lastSavedSize = initial.size();
    }

    // Writes the cache to disk when it has grown. Must run on the thread that creates pipelines (the cache is externally synchronised).
    void Save() {
        std::size_t size = 0;
        if (context.Function<PFN_vkGetPipelineCacheData>("vkGetPipelineCacheData")(context.device, cache, &size, nullptr) != VK_SUCCESS || size == 0 || size == lastSavedSize) return;
        std::vector<char> data(size);
        if (context.Function<PFN_vkGetPipelineCacheData>("vkGetPipelineCacheData")(context.device, cache, &size, data.data()) != VK_SUCCESS) return;
        if (std::FILE* file = std::fopen("pipeline_cache.tmp", "wb")) {
            const bool written = std::fwrite(data.data(), 1, size, file) == size;
            std::fclose(file);
            if (written) {
                std::remove("pipeline_cache.bin");
                std::rename("pipeline_cache.tmp", "pipeline_cache.bin");
                lastSavedSize = size;
                std::fprintf(stderr, "[PIPECACHE] saved %zu bytes\n", size);
            }
        }
    }

    ~PipelineCache() {
        context.Function<PFN_vkDestroyPipelineCache>("vkDestroyPipelineCache")(context.device, cache, nullptr);
    }

    PipelineCache(const PipelineCache&) = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;
    VkPipelineCache Handle() const { return cache; }

private:
    Context context;
    VkPipelineCache cache = VK_NULL_HANDLE;
    std::size_t lastSavedSize = 0;
};

}

#endif
