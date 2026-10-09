#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_PERFORMANCETIMER_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_PERFORMANCETIMER_HPP

#include <atomic>
#include "prx/libc/include/VerboseLog.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace AgcDriver {

class FrameTiming {
public:
    using Clock = std::chrono::steady_clock;

    struct Metric {
        Clock::duration total{};
        Clock::duration maximum{};
        std::uint64_t count = 0;
        std::uint64_t bytes = 0;
    };

    // Cheap per-frame event counters (one relaxed atomic add each): they give every frame a compact record ([F] line) at almost no cost, while the
    // expensive per-scope timers below run on a duty cycle (1 frame in 16, plus every frame slower than 40 ms).
    enum Counter : int {
        Draws, Dispatches, CpuFills, CpuFillBytes, GpuClears, Waits, WaitUs, HazardWaits, RtUploads, RtDownloads, TexCreates, PipeCreates,
        ShaderCompiles, ReadBytes, WriteBytes, BufMisses, LabelsDeferred, WorkerBusyUs, WorkerIdleUs, CounterCount
    };
    void Count(Counter counter, std::uint64_t amount = 1) { counters[counter].fetch_add(amount, std::memory_order_relaxed); }
    bool Detailed() const { return id % 16 == 0; }

    explicit FrameTiming(std::uint64_t id) : id(id) {}

    Metric* Get(const char* scope, const char* stage) {
        std::lock_guard lock(mutex);
        return &metrics[{scope, stage}];
    }

    void Add(Metric* metric, Clock::duration elapsed, std::uint64_t bytes = 0) {
        std::lock_guard lock(mutex);
        metric->total += elapsed;
        metric->maximum = std::max(metric->maximum, elapsed);
        ++metric->count;
        metric->bytes += bytes;
    }

    void IncludeSubmission(std::uint64_t serial, Clock::time_point received, Clock::time_point enqueued, Clock::time_point dequeued, bool firstSegment) {
        if (serial == 0 || received > enqueued || enqueued > dequeued) throw std::runtime_error("Frame timing: invalid submission timestamps");
        if (firstSerial == 0) {
            firstSerial = serial;
            start = received;
            executionStart = dequeued;
        }
        start = std::min(start, received);
        lastSerial = serial;
        if (firstSegment) {
            Add(Get("Submission", "accept"), enqueued - received);
            Add(Get("Submission", "queue"), dequeued - enqueued);
        }
    }

    void SetFlip(std::uint64_t serial, std::size_t offset, Clock::time_point received, Clock::time_point reached) {
        if (serial != lastSerial || received > reached || executionStart > reached) throw std::runtime_error("Frame timing: invalid flip lineage");
        flipSerial = serial;
        flipOffset = offset;
        flipReceived = received;
        flipReached = reached;
    }

    void Print(std::uint32_t outputHandle, std::int32_t buffer, std::int64_t argument, Clock::time_point finished, Clock::duration interval) {
        std::lock_guard lock(mutex);
        if (firstSerial == 0 || flipSerial == 0) throw std::runtime_error("Frame timing: incomplete submission lineage");
        {
            static const auto processStart = Clock::now();
            const auto get = [&](Counter counter) { return static_cast<unsigned long long>(counters[counter].load(std::memory_order_relaxed)); };
            char line[640];
            std::snprintf(line, sizeof(line),
                "[F] id=%llu t=%.3f iv=%.2f s2f=%.2f e2f=%.2f busy=%.2f idle=%.2f draws=%llu disp=%llu cfill=%llu cfillMB=%.1f gclr=%llu waits=%llu waitms=%.2f haz=%llu rtup=%llu rtdn=%llu tex=%llu pipe=%llu shd=%llu rdMB=%.1f wrMB=%.1f buf=%llu lbl=%llu\n",
                static_cast<unsigned long long>(id), std::chrono::duration<double>(finished - processStart).count(), milliseconds(interval),
                milliseconds(finished - start), milliseconds(flipReached - executionStart), get(WorkerBusyUs) / 1000.0, get(WorkerIdleUs) / 1000.0,
                get(Draws), get(Dispatches), get(CpuFills), get(CpuFillBytes) / 1048576.0, get(GpuClears), get(Waits), get(WaitUs) / 1000.0, get(HazardWaits),
                get(RtUploads), get(RtDownloads), get(TexCreates), get(PipeCreates), get(ShaderCompiles), get(ReadBytes) / 1048576.0, get(WriteBytes) / 1048576.0,
                get(BufMisses), get(LabelsDeferred));
            if (VerboseLog()) std::fputs(line, stdout);
        }
        if (!(Detailed() || milliseconds(interval) > 40.0)) {
            if (std::fflush(stdout) != 0) throw std::runtime_error("Frame timing: report write failed");
            return;
        }
        if (!VerboseLog()) return;
        std::ostringstream output;
        output.imbue(std::locale::classic());
        output << std::fixed << std::setprecision(3);
        output << "[FrameTiming] frame=" << id << " submissions=" << firstSerial << ':' << lastSerial;
        output << " output=" << outputHandle;
        output << " flip=" << flipSerial << ':' << flipOffset << " buffer=" << buffer << " argument=" << argument;
        output << " endpoint=flip_complete display_confirmed=0";
        output << " first_submit_to_flip_ms=" << milliseconds(finished - start);
        output << " flip_submit_to_complete_ms=" << milliseconds(finished - flipReceived);
        output << " first_submit_to_execute_ms=" << milliseconds(executionStart - start);
        output << " execute_to_flip_packet_ms=" << milliseconds(flipReached - executionStart);
        output << " flip_packet_to_complete_ms=" << milliseconds(finished - flipReached);
        auto accounted = Clock::duration::zero();
        for (const auto scope : {"Driver.Packet", "Driver.Suspend", "Driver.Worker", "Driver.Completion"}) {
            const auto it = metrics.find({scope, "total"});
            if (it != metrics.end()) accounted += it->second.total;
        }
        output << " worker_unattributed_ms=" << milliseconds(flipReached - executionStart - accounted);
        if (interval != Clock::duration::zero()) output << " flip_interval_ms=" << milliseconds(interval);
        output << " metrics=inclusive(count,sum_ms,max_ms[,bytes])";
        for (const auto& [key, metric] : metrics) {
            if (metric.count == 0) continue;
            output << ' ' << key.first << '.' << key.second << "=(" << metric.count << ',' << milliseconds(metric.total) << ',' << milliseconds(metric.maximum);
            if (metric.bytes != 0) output << ',' << metric.bytes;
            output << ')';
        }
        output << '\n';
        const auto text = output.str();
        if (std::fwrite(text.data(), 1, text.size(), stdout) != text.size() || std::fflush(stdout) != 0) throw std::runtime_error("Frame timing: report write failed");
    }

private:
    static double milliseconds(Clock::duration elapsed) {
        return std::chrono::duration<double, std::milli>(elapsed).count();
    }

    std::atomic<std::uint64_t> counters[CounterCount]{};
    std::mutex mutex;
    std::map<std::pair<std::string_view, std::string_view>, Metric> metrics;
    std::uint64_t id;
    std::uint64_t firstSerial = 0;
    std::uint64_t lastSerial = 0;
    std::uint64_t flipSerial = 0;
    std::size_t flipOffset = 0;
    Clock::time_point start{};
    Clock::time_point executionStart{};
    Clock::time_point flipReceived{};
    Clock::time_point flipReached{};
};

class PerformanceContext {
public:
    explicit PerformanceContext(FrameTiming* frame) : previous(current) {
        current = frame;
    }

    PerformanceContext(const PerformanceContext&) = delete;
    PerformanceContext& operator=(const PerformanceContext&) = delete;

    ~PerformanceContext() {
        current = previous;
    }

    static FrameTiming* Current() {
        return current;
    }

private:
    inline static thread_local FrameTiming* current = nullptr;
    FrameTiming* previous;
};

// Reports any single operation that blocks for a long time (a hitch you can feel), independent of the opt-in detailed timers.
class StallGuard {
public:
    StallGuard(const char* what, const char* detail = "", std::uint64_t a = 0, std::uint64_t b = 0, long long thresholdMs = 200)
        : what(what), detail(detail), a(a), b(b), thresholdMs(thresholdMs), enabled(VerboseLog()), start(enabled ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{}) {}
    StallGuard(const StallGuard&) = delete;
    StallGuard& operator=(const StallGuard&) = delete;
    ~StallGuard() {
        if (!enabled) return;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (ms < thresholdMs) return;
        static std::atomic<int> reported{0};
        if (reported.fetch_add(1) >= 300) return;
        std::fprintf(stderr, "[STALL] %s %s %lldms a=0x%llx b=%llu\n", what, detail, static_cast<long long>(ms), static_cast<unsigned long long>(a), static_cast<unsigned long long>(b));
        std::fflush(stderr);
    }

private:
    const char* what;
    const char* detail;
    std::uint64_t a, b;
    long long thresholdMs;
    bool enabled;
    std::chrono::steady_clock::time_point start;
};

// Counts an event into the frame being executed by this thread (no-op outside a frame context).
inline void CountEvent(FrameTiming::Counter counter, std::uint64_t amount = 1) {
    if (auto* frame = PerformanceContext::Current()) frame->Count(counter, amount);
}

class PerformanceTimer {
public:
    // Detailed per-scope timing costs a mutex and a string-keyed map lookup per event (tens of thousands of events per frame), which is a
    // measurable share of a frame. It is therefore opt-in: create timing_detail.txt in the working directory to get the full metric breakdown.
    // Frame-level timing (flip interval, submission latencies) is always available.
    static FrameTiming* Gate(FrameTiming* frame) {
        return frame != nullptr && (frame->Detailed() || DetailEnabled()) ? frame : nullptr;
    }

    static bool DetailEnabled() {
        static const bool enabled = std::fopen("timing_detail.txt", "rb") != nullptr;
        return enabled;
    }

    explicit PerformanceTimer(const char* scope) : frame(Gate(PerformanceContext::Current())), scope(scope) {
        if (frame != nullptr) {
            total = frame->Get(scope, "total");
            start = Clock::now();
            previous = start;
        }
    }

    PerformanceTimer(const PerformanceTimer&) = delete;
    PerformanceTimer& operator=(const PerformanceTimer&) = delete;

    ~PerformanceTimer() {
        if (frame == nullptr) return;
        const auto end = Clock::now();
        if (tail != nullptr) frame->Add(tail, end - previous);
        frame->Add(total, end - start);
    }

    void Mark(const char* stage, std::uint64_t bytes = 0) {
        if (frame == nullptr) return;
        const auto now = Clock::now();
        if (tail == nullptr) tail = frame->Get(scope, "tail");
        frame->Add(frame->Get(scope, stage), now - previous, bytes);
        previous = now;
    }

private:
    using Clock = FrameTiming::Clock;

    FrameTiming* frame;
    const char* scope;
    FrameTiming::Metric* total = nullptr;
    FrameTiming::Metric* tail = nullptr;
    Clock::time_point start;
    Clock::time_point previous;
};

}

#endif
