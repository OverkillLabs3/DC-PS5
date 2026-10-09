#include "prx/libSceAgcDriver/Graphics/include/DrawQueue.hpp"
#include "prx/libc/include/VerboseLog.hpp"
#include "prx/libSceAgcDriver/Execution/include/MemoryAccessScope.hpp"
#include "prx/libSceAgcDriver/Execution/include/PerformanceTimer.hpp"
#include <algorithm>
#include <chrono>
#include <map>
#include <tuple>
#include <cstdio>
#include <cstring>
#include <vector>

namespace AgcDriver::Graphics {

DrawQueue::~DrawQueue() {
    recording.commands.reset();
    for (auto& batch : pending) batch.commands.reset();
}

VkCommandBuffer DrawQueue::Begin(const Context& context) {
    Collect();
    if (drawCount >= 64) Wait("cap64");
    if (!recording.commands) {
        if (available.empty()) recording.commands = std::make_unique<CommandBatch>(context);
        else {
            recording.commands = std::move(available.back());
            available.pop_back();
            recording.commands->Reset();
        }
    }
    return recording.commands->Handle();
}

void DrawQueue::Enqueue(std::shared_ptr<ShaderResources> resources, std::shared_ptr<void> storage) {
    Require(recording.commands != nullptr && resources != nullptr && storage != nullptr, "draw batch is incomplete");
    recording.entries.push_back({std::move(storage), std::move(resources)});
    ++drawCount;
    if (recording.entries.size() >= 8) Flush();
}

void DrawQueue::Flush() {
    if (!recording.commands) return;
    Require(!recording.entries.empty() || recording.hasBarrier, "cannot submit an incomplete draw batch");
    pending.push_back(std::move(recording));
    recording = Batch{};
    pending.back().commands->Submit();
}

void DrawQueue::Defer(std::function<void()> action) {
    Collect();
    const bool recordingHasWork = recording.commands != nullptr && (!recording.entries.empty() || recording.hasBarrier);
    if (!recordingHasWork && pending.empty()) {
        action();
        return;
    }
    if (recordingHasWork) {
        recording.completions.push_back(std::move(action));
        ++deferred;
        Flush(); // submit now so the GPU starts and the label lands without further CPU activity
    } else {
        pending.back().completions.push_back(std::move(action));
        ++deferred;
    }
}

void DrawQueue::AfterCompletion(std::function<void()> action) {
    const bool recordingHasWork = recording.commands != nullptr && (!recording.entries.empty() || recording.hasBarrier);
    if (recordingHasWork) {
        recording.completions.push_back(std::move(action));
        ++deferred;
    } else if (!pending.empty()) {
        pending.back().completions.push_back(std::move(action));
        ++deferred;
    } else {
        action();
    }
}

void DrawQueue::Resolve(std::uint64_t address, std::size_t bytes, bool writable) {
    const auto overlaps = [&](const auto& entry) { return entry.resources->WritesOverlap(address, bytes); };
    if (std::any_of(recording.entries.begin(), recording.entries.end(), overlaps) || std::any_of(pending.begin(), pending.end(), [&](const auto& batch) { return std::any_of(batch.entries.begin(), batch.entries.end(), overlaps); })) {
        static int hazardLogs = 0;
        if (hazardLogs < 40) {
            ++hazardLogs;
            std::fprintf(stderr, "[HAZARD] guest range 0x%llx+%zu overlaps GPU-pending writes (draws in flight: %zu)\n", static_cast<unsigned long long>(address), bytes, drawCount);
        }
        if (!writable && bytes <= 256 && hazardHistory.size() < 4096) {
            // The GPU's writes are conservative (the whole bound range). If the bytes a read needs turn out unchanged by the
            // completion of the writers every time, later reads skip the wait and are verified once the writers retire.
            auto& history = hazardHistory[{address, bytes}];
            std::vector<std::uint8_t> before(bytes);
            std::memcpy(before.data(), reinterpret_cast<const void*>(address), bytes);
            if (history.trusted) {
                static unsigned long long skipped = 0;
                if (++skipped % 20000 == 1) std::fprintf(stderr, "[HAZARD] trusted read 0x%llx+%zu proceeds without a GPU wait (#%llu)\n", static_cast<unsigned long long>(address), bytes, skipped);
                const auto key = std::make_pair(address, bytes);
                AfterCompletion([this, key, before = std::move(before)]() {
                    std::vector<std::uint8_t> after(key.second);
                    std::memcpy(after.data(), reinterpret_cast<const void*>(key.first), key.second);
                    if (std::memcmp(after.data(), before.data(), key.second) != 0) {
                        auto& entry = hazardHistory[key];
                        entry.trusted = false;
                        entry.agreeing = 0;
                        std::fprintf(stderr, "[HAZARD] trusted read 0x%llx+%zu was WRONG (GPU changed it); waiting again from now on\n", static_cast<unsigned long long>(key.first), key.second);
                    }
                });
                return;
            }
            Wait("hazard");
            if (std::memcmp(before.data(), reinterpret_cast<const void*>(address), bytes) == 0) {
                if (++history.agreeing >= 4) history.trusted = true;
            } else {
                history.agreeing = 0;
            }
            return;
        }
        Wait("hazard");
    }
}


}
