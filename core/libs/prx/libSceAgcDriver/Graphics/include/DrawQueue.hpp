#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_DRAWQUEUE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_DRAWQUEUE_HPP

#include "prx/libSceAgcDriver/Graphics/include/ShaderResources.hpp"
#include <functional>
#include <map>

namespace AgcDriver::Graphics {

class DrawQueue {
public:
    ~DrawQueue();
    VkCommandBuffer Begin(const Context& context);
    void Enqueue(std::shared_ptr<ShaderResources> resources, std::shared_ptr<void> storage);
    void Flush();
    // `writable` is false for pure guest reads: a small read that overlaps in-flight GPU writes only waits until it has been observed to matter.
    void Resolve(std::uint64_t address, std::size_t bytes, bool writable = true);
    void Wait(const char* reason = "other");
    void WaitGpu();
    void Collect();
    // Runs `action` once everything recorded or submitted so far has completed on the GPU (immediately when nothing is in flight).
    // Batches retire in submission order, so deferred actions run in order. Used for end-of-pipe label writes so the CPU need not drain the GPU.
    void Defer(std::function<void()> action);
    bool HasDeferred() const { return deferred != 0; }
    void RecordMemoryBarrier(const Context& context);
    // Appends commands to the batch being recorded, in order with the draws already recorded (no CPU wait).
    void RecordCommands(const Context& context, const std::function<void(VkCommandBuffer)>& record);

private:
    struct Entry {
        std::shared_ptr<void> storage;
        std::shared_ptr<ShaderResources> resources;
    };
    struct Batch {
        std::vector<Entry> entries;
        std::unique_ptr<CommandBatch> commands;
        bool hasBarrier = false;
        std::vector<std::function<void()>> completions;
    };
    void retire(Batch batch);
    // Runs `action` when everything submitted so far has retired, without forcing a submit.
    void AfterCompletion(std::function<void()> action);
    struct HazardHistory {
        std::uint32_t agreeing = 0;   // consecutive waits after which the bytes were unchanged
        bool trusted = false;         // reads may proceed without waiting; verified after completion
    };
    std::map<std::pair<std::uint64_t, std::size_t>, HazardHistory> hazardHistory;
    Batch recording;
    std::vector<Batch> pending;
    std::vector<std::unique_ptr<CommandBatch>> available;
    std::size_t drawCount = 0;
    std::size_t deferred = 0;
};

}

#endif
