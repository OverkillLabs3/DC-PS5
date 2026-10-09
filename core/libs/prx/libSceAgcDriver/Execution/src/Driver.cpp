#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libc/include/VerboseLog.hpp"
#include "prx/libSceAgcDriver/Execution/include/PerformanceTimer.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/ShaderMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/Pm4.hpp"
#include "prx/libSceAgcDriver/Execution/include/QueueState.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ShaderInputState.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "prx/libSceAgcDriver/Execution/include/MemoryAccessScope.hpp"
#include "prx/libc/include/GuestMemoryTracking.hpp"
#include "ControlFlow/RequestSerializer.hpp"
#include <chrono>
#include <bit>
#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <exception>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace AgcDriver {
namespace {

// PS5 titles use PM4 fields the PS4-era validators reject; an escaping throw
// reaches std::terminate and kills the game process. Submission and per-packet
// failures are logged (bounded) and execution continues instead.
int toleratedLogs = 0;
void LogTolerated(const char* where, const char* what) {
    if (toleratedLogs < 60) {
        ++toleratedLogs;
        std::fprintf(stderr, "[DRV] %s tolerated (#%d): %s\n", where, toleratedLogs, what);
        std::fflush(stderr);
    }
}

// Set by dispatch/draw before compiling so a recompiler throw can be traced
// back to the offending guest program.
thread_local std::uint64_t failingProgram = 0;

int dispatchLogs = 0;

// Texture validation epoch (see TextureCache::SetValidationEpoch). Only touched by the driver worker thread.
std::uint64_t textureEpoch = 1;
std::uint64_t textureEpochSerial = 0;

// TEMPORARY MEASUREMENT: splits wall time between the guest producing command streams and the worker executing
// them, which decides whether the next optimisation belongs on the guest side or in the driver.
std::atomic<long long> pipeGuestNs{0}, pipeWorkerNs{0}, pipeSubmissions{0};
// TEMPORARY MEASUREMENT: how long guest threads block inside the driver's synchronisation entry points. If these
// are small, the guest-side frame time is real guest execution rather than back-pressure.
std::atomic<long long> pipeSuspendNs{0}, pipeSuspendCalls{0}, pipeWaitIdleNs{0}, pipeWaitIdleCalls{0};

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

// strict_sync.txt in the working directory restores the fully synchronous GPU model (drain on every transfer, label and submission).
bool StrictSyncEnabled() {
    static const bool strict = std::fopen("strict_sync.txt", "rb") != nullptr;
    return strict;
}


// Opt-in sampling profiler and freeze watchdog (sampler.txt in the working directory).
//  * Every 10 ms it samples the instruction pointer and the innermost return addresses (inside our modules) of the three busiest threads and every
//    10 s reports, per thread, CPU share, hottest 64-byte code buckets and call-site signatures, plus a [MEM] line.
//  * When no frame has been flipped for 400 ms it switches to freeze mode: every 60 ms it samples EVERY thread, and when frames resume it reports
//    what each thread was doing during the freeze ([STALLREPORT]).
// Nothing may allocate while a target thread is suspended (it may hold the heap lock): only fixed buffers are touched between Suspend and Resume.
std::atomic<long long> g_lastFlipNs{0};
std::atomic<long long> g_busyNs{0}, g_idleNs{0};

void samplerMain() {
    struct Info {
        HANDLE handle = nullptr;
        std::uint64_t lastCpu = 0;
        std::uint64_t delta = 0;
    };
    std::map<DWORD, Info> threads;
    std::map<std::pair<DWORD, std::uintptr_t>, unsigned> hits;
    std::map<DWORD, unsigned> samples;
    std::map<std::pair<DWORD, std::string>, unsigned> callers;
    std::map<std::pair<DWORD, std::string>, unsigned> stallSignatures;
    const auto self = GetCurrentThreadId();
    const auto process = GetCurrentProcessId();
    const auto begin = std::chrono::steady_clock::now();
    auto lastRefresh = begin;
    auto lastReport = begin;
    auto lastStallRound = begin;
    bool inStall = false;
    long long stallStartNs = 0;
    int stallRounds = 0;
    std::vector<DWORD> busiest;
    const auto seconds = [&](std::chrono::steady_clock::time_point point) { return std::chrono::duration<double>(point - begin).count(); };
    const auto cpuOf = [](HANDLE handle) {
        FILETIME created{}, exited{}, kernel{}, user{};
        if (GetThreadTimes(handle, &created, &exited, &kernel, &user) == FALSE) return std::uint64_t{0};
        return (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32 | kernel.dwLowDateTime) + (static_cast<std::uint64_t>(user.dwHighDateTime) << 32 | user.dwLowDateTime);
    };
    const auto moduleName = [](std::uintptr_t address, char* out, std::size_t size, std::uintptr_t* base) {
        HMODULE module = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(address), &module) == FALSE || module == nullptr) return false;
        char path[260] = {};
        if (GetModuleFileNameA(module, path, sizeof(path)) == 0) return false;
        const char* slash = std::strrchr(path, '\\');
        std::snprintf(out, size, "%s", slash != nullptr ? slash + 1 : path);
        if (base != nullptr) *base = reinterpret_cast<std::uintptr_t>(module);
        return true;
    };
    // Suspend, copy registers and a stack slice into fixed buffers, resume; then build "[in module 0xoffset] frame <- frame" outside the suspension.
    const auto capture = [&](HANDLE handle, std::uintptr_t& rip, std::string& signature, std::string& lead) {
        std::uintptr_t stackWords[128];
        SIZE_T stackBytes = 0;
        bool captured = false;
        if (SuspendThread(handle) == static_cast<DWORD>(-1)) return false;
        {
            CONTEXT context{};
            context.ContextFlags = CONTEXT_CONTROL;
            captured = GetThreadContext(handle, &context) != FALSE;
            rip = static_cast<std::uintptr_t>(context.Rip);
            if (captured) ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(context.Rsp), stackWords, sizeof(stackWords), &stackBytes);
        }
        ResumeThread(handle);
        if (!captured) return false;
        signature.clear();
        int frames = 0;
        for (SIZE_T i = 0; i < stackBytes / sizeof(std::uintptr_t) && frames < 3; ++i) {
            char name[96];
            std::uintptr_t base = 0;
            if (!moduleName(stackWords[i], name, sizeof(name), &base)) continue;
            const auto length = std::strlen(name);
            const bool ours = (length > 4 && std::strcmp(name + length - 4, ".prx") == 0) || std::strcmp(name, "game.exe") == 0;
            if (!ours) continue;
            char frame[160];
            std::snprintf(frame, sizeof(frame), "%s%s+0x%llx", frames == 0 ? "" : " <- ", name, static_cast<unsigned long long>(stackWords[i] - base));
            signature += frame;
            ++frames;
        }
        if (signature.empty()) signature = "(no app frame)";
        char module[96] = "?";
        moduleName(rip, module, sizeof(module), nullptr);
        char text[160];
        std::snprintf(text, sizeof(text), "[in %s 0x%llx] ", module, static_cast<unsigned long long>(rip & 0xfff));
        lead = text;
        return true;
    };
    for (;;) {
        Sleep(10);
        const auto now = std::chrono::steady_clock::now();
        if (now - lastRefresh >= std::chrono::seconds(1)) {
            lastRefresh = now;
            const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 entry{};
                entry.dwSize = sizeof(entry);
                if (Thread32First(snapshot, &entry) != FALSE) {
                    do {
                        if (entry.th32OwnerProcessID != process || entry.th32ThreadID == self) continue;
                        auto& info = threads[entry.th32ThreadID];
                        if (info.handle == nullptr) info.handle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
                        if (info.handle == nullptr) continue;
                        const auto cpu = cpuOf(info.handle);
                        info.delta = cpu > info.lastCpu ? cpu - info.lastCpu : 0;
                        info.lastCpu = cpu;
                    } while (Thread32Next(snapshot, &entry) != FALSE);
                }
                CloseHandle(snapshot);
            }
            std::vector<std::pair<std::uint64_t, DWORD>> ranked;
            for (const auto& [id, info] : threads) ranked.emplace_back(info.delta, id);
            std::sort(ranked.rbegin(), ranked.rend());
            busiest.clear();
            for (std::size_t i = 0; i < ranked.size() && i < 3; ++i) if (ranked[i].first > 200000ull) busiest.push_back(ranked[i].second); // > 20 ms per second
        }

        // ---- freeze watchdog
        const long long nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        const long long lastFlip = g_lastFlipNs.load();
        const bool stalled = lastFlip != 0 && nowNs - lastFlip > 400000000ll;
        if (stalled) {
            if (!inStall) {
                inStall = true;
                stallStartNs = lastFlip;
                stallRounds = 0;
                stallSignatures.clear();
            }
            if (now - lastStallRound >= std::chrono::milliseconds(60)) {
                lastStallRound = now;
                ++stallRounds;
                for (const auto& [id, info] : threads) {
                    if (info.handle == nullptr) continue;
                    std::uintptr_t rip = 0;
                    std::string signature, lead;
                    if (capture(info.handle, rip, signature, lead)) ++stallSignatures[{id, lead + signature}];
                }
            }
        } else if (inStall) {
            inStall = false;
            std::string report = "[STALLREPORT] t=" + std::to_string(seconds(now)) + "s frozen_for=" + std::to_string((nowNs - stallStartNs) / 1000000) + "ms rounds=" + std::to_string(stallRounds) + "\n";
            std::map<DWORD, std::vector<std::pair<unsigned, std::string>>> perThread;
            for (const auto& [key, count] : stallSignatures) perThread[key.first].emplace_back(count, key.second);
            for (auto& [id, list] : perThread) {
                std::sort(list.rbegin(), list.rend());
                char header[64];
                std::snprintf(header, sizeof(header), "  tid=%lu\n", static_cast<unsigned long>(id));
                report += header;
                for (std::size_t i = 0; i < list.size() && i < 2; ++i) {
                    char line[400];
                    std::snprintf(line, sizeof(line), "    %3u/%d %s\n", list[i].first, stallRounds, list[i].second.c_str());
                    report += line;
                }
            }
            std::fputs(report.c_str(), stderr);
            std::fflush(stderr);
        }

        // ---- normal profiling of the busiest threads
        if (!stalled) {
            for (const auto id : busiest) {
                const auto found = threads.find(id);
                if (found == threads.end() || found->second.handle == nullptr) continue;
                std::uintptr_t rip = 0;
                std::string signature, lead;
                if (!capture(found->second.handle, rip, signature, lead)) continue;
                ++hits[{id, rip & ~static_cast<std::uintptr_t>(0x3f)}];
                ++samples[id];
                ++callers[{id, lead + signature}];
            }
        }
        if (now - lastReport < std::chrono::seconds(10)) continue;
        lastReport = now;
        std::string report = "[SAMPLER] t=" + std::to_string(seconds(now)) + "s\n";
        {
            PROCESS_MEMORY_COUNTERS_EX memory{};
            memory.cb = sizeof(memory);
            MEMORYSTATUSEX system{};
            system.dwLength = sizeof(system);
            GlobalMemoryStatusEx(&system);
            char line[256] = "";
            if (K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)) != FALSE) {
                std::snprintf(line, sizeof(line), "  [MEM] workingset=%.0fMB private=%.0fMB peak_ws=%.0fMB system_free=%.0fMB of %.0fMB threads=%zu\n", memory.WorkingSetSize / 1048576.0, memory.PrivateUsage / 1048576.0,
                    memory.PeakWorkingSetSize / 1048576.0, system.ullAvailPhys / 1048576.0, system.ullTotalPhys / 1048576.0, threads.size());
                report += line;
            }
        }
        for (const auto& [id, total] : samples) {
            if (total < 20) continue;
            char header[128];
            std::snprintf(header, sizeof(header), "  tid=%lu samples=%u cpu_last_s=%.0f%%\n", static_cast<unsigned long>(id), total, threads[id].delta / 100000.0);
            report += header;
            std::vector<std::pair<unsigned, std::uintptr_t>> ranked;
            for (const auto& [key, count] : hits) if (key.first == id) ranked.emplace_back(count, key.second);
            std::sort(ranked.rbegin(), ranked.rend());
            for (std::size_t i = 0; i < ranked.size() && i < 10; ++i) {
                char name[96] = "<guest>";
                std::uintptr_t base = 0;
                moduleName(ranked[i].second, name, sizeof(name), &base);
                char line[200];
                std::snprintf(line, sizeof(line), "    %5.1f%% %s+0x%llx (0x%llx)\n", 100.0 * ranked[i].first / total, name, static_cast<unsigned long long>(ranked[i].second - base), static_cast<unsigned long long>(ranked[i].second));
                report += line;
            }
        }
        for (const auto& [id, total] : samples) {
            if (total < 20) continue;
            char header[64];
            std::snprintf(header, sizeof(header), "  tid=%lu callers:\n", static_cast<unsigned long>(id));
            report += header;
            std::vector<std::pair<unsigned, std::string>> ranked;
            for (const auto& [key, count] : callers) if (key.first == id) ranked.emplace_back(count, key.second);
            std::sort(ranked.rbegin(), ranked.rend());
            for (std::size_t i = 0; i < ranked.size() && i < 8; ++i) {
                char line[400];
                std::snprintf(line, sizeof(line), "    %5.1f%% %s\n", 100.0 * ranked[i].first / total, ranked[i].second.c_str());
                report += line;
            }
        }
        std::fputs(report.c_str(), stderr);
        std::fflush(stderr);
        hits.clear();
        samples.clear();
        callers.clear();
    }
}

#endif

void require(bool condition, const char* reason) {
    if (!condition) {
        throw std::runtime_error(std::string("AGC driver: ") + reason);
    }
}

struct ShaderSnapshot {
    std::uint64_t codeAddress;
    std::uint64_t headerAddress;
    std::uint8_t type;
    std::vector<std::uint32_t> code;
    std::vector<std::byte> header;
};

struct Submission {
    std::uint64_t serial;
    std::uint32_t queue;
    std::vector<std::uint32_t> commands;
    std::map<std::uint64_t, std::shared_ptr<const ShaderSnapshot>> shaders;
    std::map<std::size_t, std::shared_ptr<IFlipRequest>> flips;
    std::set<std::size_t> skipped;
    std::map<std::size_t, std::shared_ptr<IRenderingWait>> renderingWaits;
    bool suspend = false;
    FrameTiming::Clock::time_point received;
    FrameTiming::Clock::time_point copied;
    FrameTiming::Clock::time_point validated;
    FrameTiming::Clock::time_point enqueued;
    FrameTiming::Clock::time_point dequeued;
};

std::uint32_t readRegister(const Registers& registers, std::uint32_t offset) {
    const auto it = registers.find(offset);
    require(it != registers.end(), "required shader register has not been written");
    return it->second;
}

// Registers the guest only writes when they differ from their reset value (e.g. COMPUTE_START_*).
std::uint32_t readRegisterOr(const Registers& registers, std::uint32_t offset, std::uint32_t fallback) {
    const auto it = registers.find(offset);
    return it == registers.end() ? fallback : it->second;
}

class Driver {
public:
    static Driver& Get() {
        static Driver driver;
        return driver;
    }

    ~Driver() {
        stop();
    }

    void Shutdown() {
        stop();
        CheckFailure();
    }

private:
    void stop() {
        require(std::this_thread::get_id() != worker.get_id(), "worker cannot stop itself");
        std::lock_guard shutdownLock(shutdownMutex);
        {
            std::lock_guard lock(mutex);
            stopping = true;
        }
        changed.notify_all();
        if (worker.joinable()) worker.join();
    }

public:

    void Submit(const Packet* packet, std::uint32_t queue) {
        try {
            submitImpl(packet, queue);
        } catch (const std::exception& error) {
            LogTolerated("Submit", error.what());
        }
    }

    void submitImpl(const Packet* packet, std::uint32_t queue) {
        const auto received = FrameTiming::Clock::now();
#ifdef _WIN32
        {
            static std::once_flag samplerOnce;
            std::call_once(samplerOnce, [] {
                if (std::FILE* toggle = std::fopen("sampler.txt", "rb")) {
                    std::fclose(toggle);
                    std::thread([] { samplerMain(); }).detach();
                }
            });
        }
#endif
        CheckFailure();
        require(queue == 0 || (queue >= 0x20 && queue < 0x58), "unsupported compute queue");
        GuestMemory::CheckRange(packet, sizeof(Packet), alignof(Packet));
        const auto descriptor = *packet;
        require(descriptor.flags == 0, "nonzero submission flags are not implemented");
        Submission submission{};
        submission.queue = queue;
        submission.received = received;
        if (descriptor.dw_num != 0) {
            require(descriptor.dw_num <= std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t), "command size overflow");
            GuestMemory::CheckRange(descriptor.addr, static_cast<std::size_t>(descriptor.dw_num) * sizeof(std::uint32_t), alignof(std::uint32_t));
            submission.commands.assign(descriptor.addr, descriptor.addr + descriptor.dw_num);
        }
        submission.copied = FrameTiming::Clock::now();
        validate(submission.commands, queue, submission.skipped);
        submission.validated = FrameTiming::Clock::now();
        {
            std::lock_guard lock(mutex);
            rethrowFailure();
            require(!stopping, "submission during shutdown");
            require(accepted != std::numeric_limits<std::uint64_t>::max(), "submission serial overflow");
            for (std::size_t cursor = 0; cursor < submission.commands.size();) {
                const auto* words = submission.commands.data() + cursor;
                if (words[0] == RenderingWaitPacketHeader) {
                    const auto output = outputs.find(words[1]);
                    require(output != outputs.end(), "rendering wait references an unregistered video output");
                    auto wait = output->second->CaptureRenderingWait(words[2]);
                    require(wait != nullptr, "video output returned a null rendering wait");
                    submission.renderingWaits.emplace(cursor, std::move(wait));
                }
                if (words[0] == FlipPacketHeader) {
                    const auto output = outputs.find(words[1]);
                    require(output != outputs.end(), "flip references an unregistered video output");
                    const FlipInfo info{words[1], std::bit_cast<std::int32_t>(words[2]), words[3], std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(words[4]) | (static_cast<std::uint64_t>(words[5]) << 32u))};
                    auto request = output->second->Reserve(info);
                    require(request != nullptr, "video output returned a null flip reservation");
                    submission.flips.emplace(cursor, std::move(request));
                }
                cursor += static_cast<std::size_t>((words[0] >> 16u) & 0x3fffu) + 2;
            }
            submission.shaders = shaders;
            submission.serial = accepted + 1;
            submission.enqueued = FrameTiming::Clock::now();
            static FrameTiming::Clock::time_point lastEnqueue{}; // only touched under `mutex`
            if (lastEnqueue.time_since_epoch().count() != 0) pipeGuestNs += std::chrono::duration_cast<std::chrono::nanoseconds>(submission.enqueued - lastEnqueue).count();
            lastEnqueue = submission.enqueued;
            pending.push_back(std::move(submission));
            ++accepted;
        }
        changed.notify_all();
    }

    void WaitIdle() {
        require(std::this_thread::get_id() != worker.get_id(), "worker cannot wait for itself");
        const auto entered = FrameTiming::Clock::now();
        std::unique_lock lock(mutex);
        const auto target = accepted;
        changed.wait(lock, [&] { return failure != nullptr || completed >= target; });
        pipeWaitIdleNs += std::chrono::duration_cast<std::chrono::nanoseconds>(FrameTiming::Clock::now() - entered).count();
        pipeWaitIdleCalls.fetch_add(1);
        if (failure != nullptr) {
            LogTolerated("WaitIdle", "waiting despite recorded driver failure");
            return;
        }
    }

    void SuspendPoint() {
        const auto received = FrameTiming::Clock::now();
        require(std::this_thread::get_id() != worker.get_id(), "worker cannot suspend itself");
        std::unique_lock lock(mutex);
        rethrowFailure();
        require(!stopping, "suspend during shutdown");
        require(accepted != std::numeric_limits<std::uint64_t>::max(), "submission serial overflow");
        Submission boundary{};
        boundary.serial = accepted + 1;
        boundary.suspend = true;
        boundary.received = received;
        boundary.copied = received;
        boundary.validated = received;
        boundary.enqueued = FrameTiming::Clock::now();
        pending.push_back(std::move(boundary));
        ++accepted;
        changed.notify_all();
        // The suspend point is an ordered marker for the worker (graphics state reset); the guest does not wait for the GPU to drain,
        // exactly as on hardware, where the game paces itself with the flip and end-of-pipe labels the worker signals in order.
        pipeSuspendNs += std::chrono::duration_cast<std::chrono::nanoseconds>(FrameTiming::Clock::now() - received).count();
        pipeSuspendCalls.fetch_add(1);
        if (failure != nullptr) {
            LogTolerated("SuspendPoint", "returning despite recorded driver failure");
            return;
        }
    }

    void RegisterVideoOutput(std::uint32_t handle, const std::shared_ptr<IVideoOutput>& output) {
        require(output != nullptr, "null video output");
        std::lock_guard lock(mutex);
        rethrowFailure();
        require(!stopping, "video output registration during shutdown");
        require(outputs.emplace(handle, output).second, "video output already registered");
    }

    void UnregisterVideoOutput(std::uint32_t handle, const std::shared_ptr<IVideoOutput>& output) {
        std::lock_guard lock(mutex);
        const auto it = outputs.find(handle);
        require(it != outputs.end() && it->second == output, "video output registration mismatch");
        outputs.erase(it);
    }

    void CheckFailure() {
        std::lock_guard lock(mutex);
        rethrowFailure();
    }

    void ReportFailure(std::exception_ptr error) {
        require(error != nullptr, "null asynchronous failure");
        {
            std::lock_guard lock(mutex);
            if (!failure) failure = error;
            for (const auto& [handle, output] : outputs) output->Fail(failure);
            for (const auto& item : pending) {
                for (const auto& [offset, flip] : item.flips) flip->Fail(failure);
            }
            pending.clear();
        }
        changed.notify_all();
    }

    void Present(const PresentationWindow& window, const DisplayBuffer* buffer, bool opaque, void (*gpuReady)(void*), void* context) {
        PerformanceContext timingContext(window.timing.get());
        PerformanceTimer timing("Driver.Present");
        CheckFailure();
        require(gpuReady != nullptr && context != nullptr, "missing GPU completion callback");
        require(window.getDrawableSize != nullptr, "missing window drawable size query");
        std::shared_ptr<VulkanDevice> presenting;
        timing.Mark("validate");
        try {
            {
                std::lock_guard lock(gpuMutex);
                timing.Mark("gpu_mutex_wait");
                if (device == nullptr || device->Window() == nullptr) {
                    if (device) device->WaitIdle();
                    device = std::make_shared<VulkanDevice>(&window);
                }
                require(device->Window() == window.context, "presentation window does not match device surface");
                presenting = device;
                timing.Mark("device_setup");
                std::uint32_t drawableWidth = 0;
                std::uint32_t drawableHeight = 0;
                window.getDrawableSize(window.context, &drawableWidth, &drawableHeight);
                presenting->Resize(drawableWidth, drawableHeight);
                timing.Mark("resize");
                if (presenting->Presentable()) {
                    if (buffer != nullptr) {
                        require(buffer->width == window.width && buffer->height == window.height, "display buffer extent differs from output");
                        {
                            static int presentLogs = 0;
                            if (presentLogs < 6 || presentLogs % 300 == 0) std::fprintf(stderr, "[DRV] PRESENT #%d display=0x%llx %ux%u fmt=0x%llx\n", presentLogs, static_cast<unsigned long long>(buffer->address), buffer->width, buffer->height, static_cast<unsigned long long>(buffer->pixelFormat));
                            ++presentLogs;
                        }
                        presenting->WaitDraws();
                        timing.Mark("draw_wait");
                        presenting->PresentDisplayBuffer(*buffer);
                        timing.Mark("present_display_buffer");
                    } else {
                        presenting->PresentClear(window.width, window.height, opaque);
                        timing.Mark("present_clear");
                    }
                }
            }
            gpuReady(context);
            timing.Mark("release_and_callback");
            CheckFailure();
        } catch (...) {
            ReportFailure(std::current_exception());
            throw;
        }
    }

    void ReleaseWindow(void* window) {
        std::lock_guard lock(gpuMutex);
        if (device && device->Window() == window) device.reset();
    }

    void RegisterShader(const Shader* shader) {
        CheckFailure();
        GuestMemory::CheckRange(shader, sizeof(Shader), alignof(Shader));
        require(shader->file_header == 0x34333231u && shader->version == 0x18u, "invalid shader header");
        require(shader->header_size >= sizeof(Shader), "shader header is smaller than its fixed fields");
        require(shader->shader_size != 0 && (shader->shader_size & 3u) == 0, "invalid shader size");
        GuestMemory::CheckRange(shader, shader->header_size, alignof(Shader));
        const auto* code = const_cast<const void*>(shader->code);
        GuestMemory::CheckRange(code, shader->shader_size, 256);
        ShaderSnapshot snapshot{reinterpret_cast<std::uintptr_t>(code), reinterpret_cast<std::uintptr_t>(shader), shader->type, {}, {}};
        snapshot.code.resize(shader->shader_size / sizeof(std::uint32_t));
        std::memcpy(snapshot.code.data(), code, shader->shader_size);
        snapshot.header.resize(shader->header_size);
        std::memcpy(snapshot.header.data(), shader, shader->header_size);
        std::lock_guard lock(mutex);
        rethrowFailure();
        const auto address = snapshot.codeAddress;
        shaders.insert_or_assign(address, std::make_shared<const ShaderSnapshot>(std::move(snapshot)));
    }

private:
    std::mutex mutex;
    std::mutex shutdownMutex;
    std::condition_variable changed;
    std::deque<Submission> pending;
    std::map<std::uint64_t, std::shared_ptr<const ShaderSnapshot>> shaders;
    std::map<std::uint32_t, QueueState> queues;
    std::map<std::uint32_t, std::shared_ptr<IVideoOutput>> outputs;
    std::recursive_mutex& gpuMutex = GuestMemoryTracking::GuestMemoryTrackingMutex_nid_postfix();
    std::shared_ptr<VulkanDevice> device;
    std::uint64_t accepted = 0;
    std::uint64_t completed = 0;
    std::exception_ptr failure;
    bool stopping = false;
    bool resetGraphics = false;
    std::shared_ptr<FrameTiming> frameTiming;
    std::uint64_t frameSerial = 0;
    std::thread worker;

    Driver() : worker([this] { run(); }) {
        try {
            LibcRegisterShutdown_nid_postfix([] { Driver::Get().Shutdown(); });
        } catch (...) {
            stop();
            throw;
        }
    }

    void rethrowFailure() const {
        if (failure != nullptr) {
            std::rethrow_exception(failure);
        }
    }

    static void validate(std::span<const std::uint32_t> commands, std::uint32_t queue, std::set<std::size_t>& skipped) {
        for (std::size_t cursor = 0; cursor < commands.size();) {
            const auto header = commands[cursor];
            require((header & 0xc0000000u) == 0xc0000000u, "unsupported PM4 packet type");
            const auto count = static_cast<std::size_t>((header >> 16u) & 0x3fffu) + 2;
            require(count <= commands.size() - cursor, "truncated PM4 packet");
            try {
                Pm4::Validate(commands.subspan(cursor, count), queue);
            } catch (const std::exception& error) {
                // Reject only this packet; dropping the whole submission also drops the
                // flip it carries, which stalls presentation forever.
                skipped.insert(cursor);
                LogTolerated("Validate", ("AGC driver: " + Pm4::Name(header) + " at DWORD " + std::to_string(cursor) + ": " + error.what()).c_str());
            }
            cursor += count;
        }
    }

    void dispatch(QueueState& queue, std::span<const std::uint32_t> packet, const Submission& submission) {
        PerformanceTimer timing("Driver.Dispatch");
        std::lock_guard gpuLock(gpuMutex);
        timing.Mark("gpu_mutex_wait");
        const auto address = (static_cast<std::uint64_t>(readRegister(queue.shader, 0x20c)) << 8u) | (static_cast<std::uint64_t>(readRegister(queue.shader, 0x20d) & 0xffu) << 40u);
        failingProgram = address;
        auto it = submission.shaders.upper_bound(address);
        require(it != submission.shaders.begin(), "compute program does not belong to a registered shader");
        --it;
        const auto& snapshot = *it->second;
        require(address - snapshot.codeAddress < snapshot.code.size() * sizeof(std::uint32_t), "compute program is outside registered shader code");
        require(snapshot.type == 0, "compute program refers to a non-compute shader");
        const auto userCount = (readRegister(queue.shader, 0x213) >> 1u) & 0x1fu;
        std::vector<std::uint32_t> userData;
        for (std::uint32_t i = 0; i < userCount; ++i) {
            userData.push_back(readRegister(queue.shader, 0x240 + i));
        }
        const auto compute = Graphics::DecodeComputeStageInfo(queue.shader);
        // PM4 DISPATCH_DIRECT payload is {endX, endY, endZ, initiator}. Initiator bit 5 says the
        // values are thread counts rather than workgroup counts; bit 2 (FORCE_START_AT_000) says
        // whether they are absolute counts or END coordinates measured from COMPUTE_START_X/Y/Z
        // (0x204..0x206, in workgroup units). Reading them as plain group counts over-dispatched
        // Dead Cells' 64-thread passes by 64x.
        const auto initiator = packet[4];
        std::uint32_t base[3] = {0, 0, 0};
        if ((initiator & 0x4u) == 0) {
            base[0] = readRegisterOr(queue.shader, 0x204, 0);
            base[1] = readRegisterOr(queue.shader, 0x205, 0);
            base[2] = readRegisterOr(queue.shader, 0x206, 0);
        }
        std::uint32_t groups[3] = {packet[1], packet[2], packet[3]};
        bool partialGroup = false;
        for (int axis = 0; axis < 3; ++axis) {
            const auto localSize = compute.numThreads[axis];
            if ((initiator & 0x20u) != 0) {
                const auto startThread = base[axis] * localSize;
                const auto span = groups[axis] > startThread ? groups[axis] - startThread : 0u;
                partialGroup = partialGroup || (span % localSize) != 0;
                groups[axis] = (span + localSize - 1u) / localSize;
            } else {
                groups[axis] = groups[axis] > base[axis] ? groups[axis] - base[axis] : 0u;
            }
        }
        if (groups[0] == 0 || groups[1] == 0 || groups[2] == 0) {
            static std::uint32_t emptyDispatchLogs = 0;
            if (emptyDispatchLogs < 8) {
                ++emptyDispatchLogs;
                std::fprintf(stderr, "[DRV] DISPATCH empty (end=%ux%ux%u start=%ux%ux%u init=0x%x), skipped\n",
                    packet[1], packet[2], packet[3], base[0], base[1], base[2], initiator);
                std::fflush(stderr);
            }
            return;
        }
        if (dispatchLogs < 8) {
            ++dispatchLogs;
            std::fprintf(stderr, "[DRV] DISPATCH #%u end=%ux%ux%u start=%ux%ux%u local=%ux%ux%u init=0x%x wave=%u userCount=%u -> base=%ux%ux%u groups=%ux%ux%u%s program=0x%llx\n",
                dispatchLogs, packet[1], packet[2], packet[3], base[0], base[1], base[2],
                compute.numThreads[0], compute.numThreads[1], compute.numThreads[2],
                initiator, (initiator & 0x8000u) != 0 ? 32u : 64u, userCount,
                base[0], base[1], base[2], groups[0], groups[1], groups[2], partialGroup ? " PARTIAL" : "",
                static_cast<unsigned long long>(address));
            for (std::uint32_t i = 0; i < userCount; ++i) {
                std::fprintf(stderr, "[DRV]   userData[%u]=0x%08x\n", i, userData[i]);
            }
            std::fflush(stderr);
        }
        const std::array<ShaderRecompiler::MemoryRegion, 2> memory{{{snapshot.codeAddress, std::as_bytes(std::span(snapshot.code))}, {snapshot.headerAddress, snapshot.header}}};
        if (device == nullptr) {
            device = std::make_shared<VulkanDevice>();
        }
        const GuestMemory::MemoryAccessScope memoryScope(device.get(), [](void* context, std::uint64_t address, std::size_t bytes, bool writable) {
            static_cast<VulkanDevice*>(context)->ResolveMemory(address, bytes, writable);
        });
        const auto codeOffset = static_cast<std::size_t>((address - snapshot.codeAddress) / sizeof(std::uint32_t));
        {
            // TEMPORARY MEASUREMENT: identify every distinct compute program once, so further fixed-function kernels can be recognised.
            static std::set<std::uint64_t> seenPrograms;
            if (seenPrograms.size() < 24 && seenPrograms.insert(address).second) {
                std::fprintf(stderr, "[KERNEL] program=0x%llx userCount=%u groups=%ux%ux%u local=%ux%ux%u words:", static_cast<unsigned long long>(address), userCount, groups[0], groups[1], groups[2], compute.numThreads[0], compute.numThreads[1], compute.numThreads[2]);
                for (std::size_t i = 0; i < 12 && codeOffset + i < snapshot.code.size(); ++i) std::fprintf(stderr, " %08x", snapshot.code[codeOffset + i]);
                std::fprintf(stderr, "\n");
            }
            // Fast path for the SDK's buffer fill kernel (clears of render targets and other memory), recognised by its exact ISA:
            //   v_lshl_add_u32 v4,s8,6,v0 ; v_mov_b32 v0..v3,s4..s7 ; buffer_store_dwordx4 v0..v3, v4(idxen), srd s0..s3 ; s_endpgm
            // Running it through the GPU uploaded up to 33 MB of contents that are about to be overwritten, ran the fill, downloaded the same
            // amount and wrote it back. Writing the pattern straight into guest memory does the same job with no GPU round trip.
            // fill_kernel_off.txt in the working directory restores the generic path.
            static constexpr std::array<std::uint32_t, 9> fillKernel{0xd7460004u, 0x04010c08u, 0x7e000204u, 0x7e020205u, 0x7e040206u, 0x7e060207u, 0xe01c2000u, 0x80000004u, 0xbf810000u};
            static const bool fastFillEnabled = std::fopen("fill_kernel_off.txt", "rb") == nullptr;
            if (fastFillEnabled && userCount == 8 && codeOffset + fillKernel.size() <= snapshot.code.size() && std::equal(fillKernel.begin(), fillKernel.end(), snapshot.code.begin() + static_cast<std::ptrdiff_t>(codeOffset)) &&
                compute.numThreads[0] == 64 && compute.numThreads[1] == 1 && compute.numThreads[2] == 1 && groups[1] == 1 && groups[2] == 1 && base[0] == 0 && !partialGroup) {
                const auto stride = (userData[1] >> 16u) & 0x3fffu;
                const bool plainDescriptor = (userData[1] >> 30u) == 0 && (userData[3] & 0x00800000u) == 0; // no swizzle, no ADD_TID
                if (stride == 16 && plainDescriptor) {
                    const auto records = std::min<std::uint64_t>(static_cast<std::uint64_t>(groups[0]) * 64u, userData[2]);
                    const auto destination = static_cast<std::uint64_t>(userData[0]) | (static_cast<std::uint64_t>(userData[1] & 0xffffu) << 32u);
                    const auto bytes = records * 16u;
                    if (bytes != 0) {
                        // A fill of exactly one resident render target is a GPU clear: no guest-memory traffic and no readback of the old contents.
                        static const bool gpuClearEnabled = std::fopen("fill_gpu_clear_off.txt", "rb") == nullptr;
                        if (gpuClearEnabled && device->TryGpuFillClear(destination, bytes, &userData[4])) {
                            static std::atomic<unsigned long long> gpuClears{0};
                            if (++gpuClears % 600 == 0) if (VerboseLog()) std::fprintf(stderr, "[FILL] gpu clears=%llu\n", gpuClears.load());
                            CountEvent(FrameTiming::GpuClears);
                            timing.Mark("fill_gpu_clear");
                            return;
                        }
                        // Resolves render-target ownership and pending GPU work for the range, exactly like a shader store would.
                        GuestMemory::CheckRange(reinterpret_cast<const void*>(destination), static_cast<std::size_t>(bytes), 1, true);
                        auto* out = reinterpret_cast<std::byte*>(destination);
                        std::memcpy(out, &userData[4], 16);
                        for (std::uint64_t done = 16; done < bytes;) {
                            const auto chunk = std::min<std::uint64_t>(done, bytes - done);
                            std::memcpy(out + done, out, static_cast<std::size_t>(chunk));
                            done += chunk;
                        }
                    }
                    {
                        static int detailed = 0;
                        if (detailed < 60) {
                            ++detailed;
                            std::fprintf(stderr, "[FILLDETAIL] dst=0x%llx bytes=%llu pattern=%08x %08x %08x %08x targets:%s\n", static_cast<unsigned long long>(destination), static_cast<unsigned long long>(bytes), userData[4], userData[5], userData[6], userData[7], device->DescribeRenderTargets(destination).c_str());
                        }
                    }
                    static std::atomic<unsigned long long> fills{0}, fillBytes{0};
                    fillBytes += bytes;
                    CountEvent(FrameTiming::CpuFills);
                    CountEvent(FrameTiming::CpuFillBytes, bytes);
                    if (++fills % 400 == 0) if (VerboseLog()) std::fprintf(stderr, "[FILL] fast-path fills=%llu bytes=%.1f MB\n", fills.load(), fillBytes.load() / 1048576.0);
                    timing.Mark("fill_fast_path");
                    return;
                }
            }
        }
        {
            // Fast path for the SDK's 16-byte-record buffer copy kernel:
            //   v_lshl_add_u32 v4,s8,6,v0 ; buffer_load_dwordx4 v0..v3, v4(idxen), srd s0..s3 ; s_waitcnt ; buffer_store_dwordx4 v0..v3, v4(idxen), srd s4..s7 ; s_endpgm
            // A plain memmove replaces the upload/dispatch/download round trip. DISABLED by default (it produced visual corruption); copy_kernel_on.txt enables it.
            static constexpr std::array<std::uint32_t, 8> copyKernel{0xd7460004u, 0x04010c08u, 0xe00c2000u, 0x80000004u, 0xbf8c3f70u, 0xe01c2000u, 0x80010004u, 0xbf810000u};
            static const bool fastCopyEnabled = std::fopen("copy_kernel_on.txt", "rb") != nullptr; // opt-in: the CPU copy corrupts frames (verified A/B), the generic dispatch is correct
            if (fastCopyEnabled && userCount == 8 && codeOffset + copyKernel.size() <= snapshot.code.size() && std::equal(copyKernel.begin(), copyKernel.end(), snapshot.code.begin() + static_cast<std::ptrdiff_t>(codeOffset)) &&
                compute.numThreads[0] == 64 && compute.numThreads[1] == 1 && compute.numThreads[2] == 1 && groups[1] == 1 && groups[2] == 1 && base[0] == 0 && !partialGroup) {
                static int matched = 0;
                if (matched++ < 2) std::fprintf(stderr, "[COPY] kernel matched, groups=%u\n", groups[0]);
                const auto copyStride = (userData[1] >> 16u) & 0x3fffu;
                // Records are 16 bytes wide but spaced by the stride; a stride below 16 makes neighbours overlap with identical data, so the union is one contiguous block.
                const bool plain = copyStride != 0 && copyStride <= 16 && ((userData[5] >> 16u) & 0x3fffu) == copyStride &&
                    (userData[1] >> 30u) == 0 && (userData[5] >> 30u) == 0 && (userData[3] & 0x00800000u) == 0 && (userData[7] & 0x00800000u) == 0;
                if (!plain) {
                    static int shown = 0;
                    if (shown++ < 3) std::fprintf(stderr, "[COPY] not plain: ud=%08x %08x %08x %08x %08x %08x %08x %08x\n", userData[0], userData[1], userData[2], userData[3], userData[4], userData[5], userData[6], userData[7]);
                }
                if (plain) {
                    const auto records = std::min<std::uint64_t>(static_cast<std::uint64_t>(groups[0]) * 64u, std::min<std::uint64_t>(userData[2], userData[6]));
                    const auto source = static_cast<std::uint64_t>(userData[0]) | (static_cast<std::uint64_t>(userData[1] & 0xffffu) << 32u);
                    const auto destination = static_cast<std::uint64_t>(userData[4]) | (static_cast<std::uint64_t>(userData[5] & 0xffffu) << 32u);
                    const auto bytes = records == 0 ? 0u : (records - 1u) * copyStride + 16u;
                    if (bytes != 0) {
                        // The generic path drains all queued draws before a dispatch; overlap tests alone miss render-target and write-back ordering.
                        device->DrainDraws();
                        GuestMemory::CheckRange(reinterpret_cast<const void*>(source), static_cast<std::size_t>(bytes), 1, false);
                        GuestMemory::CheckRange(reinterpret_cast<const void*>(destination), static_cast<std::size_t>(bytes), 1, true);
                        std::memmove(reinterpret_cast<void*>(destination), reinterpret_cast<const void*>(source), static_cast<std::size_t>(bytes));
                    }
                    static std::atomic<unsigned long long> copies{0};
                    if (++copies % 600 == 1) std::fprintf(stderr, "[COPY] fast-path copies=%llu last=%llu bytes\n", copies.load(), static_cast<unsigned long long>(bytes));
                    timing.Mark("copy_fast_path");
                    return;
                }
            }
        }
        ShaderRecompiler::RecompileRequest request{
            {ShaderRecompiler::ShaderStage::Compute, address, std::span(snapshot.code).subspan(codeOffset), snapshot.headerAddress, snapshot.header},
            {(packet[4] & 0x8000u) != 0 ? 32u : 64u, 0, userData, compute, std::nullopt, std::nullopt, memory},
            device->Target(),
            {0, 0, 0, 128}
        };
        ShaderMemory shaderMemory(memory);
        timing.Mark("prepare");
        ShaderRecompiler::RecompileResult compiled;
        std::vector<ShaderRecompiler::MemoryRegion> captured;
        try {
            shaderMemory.Capture(request);
            timing.Mark("shader_memory_capture");
            captured = shaderMemory.Regions();
            request.context.memory = captured;
            timing.Mark("request_memory");
            {
                StallGuard compileGuard("ShaderRecompiler::Recompile", "compute", 0, 0, 15);
                compiled = ShaderRecompiler::Recompile(request);
            }
        } catch (...) {
            static bool requestDumped = false;
            if (!requestDumped) {
                requestDumped = true;
                try {
                    ShaderRecompiler::RequestSerializer serializer;
                    std::FILE* out = std::fopen("shaderfail_req.b64", "wb");
                    if (out) {
                        const auto text = serializer.Serialize(request);
                        std::fwrite(text.data(), 1, text.size(), out);
                        std::fclose(out);
                        std::fprintf(stderr, "[DRV] dumped failing request (%zu chars) to shaderfail_req.b64\n", text.size());
                    }
                } catch (const std::exception& error) {
                    std::fprintf(stderr, "[DRV] request dump failed: %s\n", error.what());
                } catch (...) {
                    std::fprintf(stderr, "[DRV] request dump failed\n");
                }
            }
            throw;
        }
        if (!compiled.cacheHit) CountEvent(FrameTiming::ShaderCompiles);
        timing.Mark(compiled.cacheHit ? "shader_cache_hit" : "shader_compile");
        std::vector<Graphics::GuestMemorySnapshot> snapshots;
        for (const auto& region : captured) snapshots.push_back({region.guestAddress, region.bytes});
        timing.Mark("snapshots");
        CountEvent(FrameTiming::Dispatches);
        device->Dispatch(compiled, groups[0], groups[1], groups[2], snapshots, base[0], base[1], base[2]);
        timing.Mark("dispatch_and_resource_release");
    }

    // WAIT_REG_MEM / WAIT_REG_MEM_64 on memory: the command processor polls a guest memory word until the comparison holds.
    // Runs outside the GPU mutex (the value is normally published by guest CPU code or by an earlier fence write), and is bounded so
    // a semantic mismatch cannot hang the whole submission worker.
    void waitRegMem(std::span<const std::uint32_t> packet) {
        PerformanceTimer timing("Driver.WaitRegMem");
        const bool wide = ((packet[0] >> 8u) & 0xffu) == 0x93;
        const auto function = packet[1] & 7u;
        const std::uint64_t address = (static_cast<std::uint64_t>(packet[3]) << 32u) | packet[2];
        const std::uint64_t reference = wide ? (static_cast<std::uint64_t>(packet[5]) << 32u) | packet[4] : packet[4];
        const std::uint64_t mask = wide ? (static_cast<std::uint64_t>(packet[7]) << 32u) | packet[6] : packet[5];
        const auto bytes = wide ? 8u : 4u;
        // Every GPU job this driver runs completes in milliseconds, so a wait still unsatisfied after 250 ms is a flag nothing will set (the old 3 s bound
        // froze the game for the whole time); give up quickly and log it.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
        auto delay = std::chrono::microseconds(20);
        for (;;) {
            if (device != nullptr && device->HasDeferred()) {
                std::lock_guard gpuLock(gpuMutex);
                device->PollCompletions();
                if (device->HasDeferred()) device->WaitDraws(); // the awaited value may be a deferred label: complete the outstanding batches
            }
            std::uint64_t value = 0;
            GuestMemory::Read(address, std::as_writable_bytes(std::span(&value, 1)).first(bytes), bytes);
            value &= mask;
            bool satisfied = true;
            switch (function) {
                case 1: satisfied = value < reference; break;
                case 2: satisfied = value <= reference; break;
                case 3: satisfied = value == reference; break;
                case 4: satisfied = value != reference; break;
                case 5: satisfied = value >= reference; break;
                case 6: satisfied = value > reference; break;
                default: break; // 0 = always
            }
            if (satisfied) return;
            if (std::chrono::steady_clock::now() >= deadline) {
                char detail[160];
                std::snprintf(detail, sizeof(detail), "wait on 0x%llx (function %u, reference 0x%llx, mask 0x%llx) timed out with value 0x%llx",
                    static_cast<unsigned long long>(address), function, static_cast<unsigned long long>(reference), static_cast<unsigned long long>(mask), static_cast<unsigned long long>(value));
                LogTolerated("WAIT_REG_MEM", detail);
                return;
            }
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, std::chrono::microseconds(1000));
        }
    }

    void draw(QueueState& queue, std::span<const std::uint32_t> packet, const Submission& submission) {
        PerformanceTimer timing("Driver.Draw");
        auto drawParameters = Pm4::ResolveDraw(packet, queue);
        if (!drawParameters.indexed && (drawParameters.indexCount == 0 || drawParameters.instanceCount == 0)) return;
        const auto graphics = Graphics::DecodeState(queue);
        {
            static int drawLogs = 0;
            if (drawLogs < 30) {
                ++drawLogs;
                std::fprintf(stderr, "[DRV] DRAW #%d op=%02x idx=%u size=%u inst=%u hasColor=%d color=0x%llx %ux%u fmt=%d bytes=%zu tile=%d render=%ux%u topo=%d vp=(%.0f,%.0f %.0fx%.0f) scissor=(%d,%d %ux%u) rectList=%d\n",
                    drawLogs, (packet[0] >> 8u) & 0xffu, drawParameters.indexCount, drawParameters.indexSize, drawParameters.instanceCount,
                    graphics.hasColorTarget ? 1 : 0, static_cast<unsigned long long>(graphics.color.address), graphics.color.extent.width, graphics.color.extent.height,
                    static_cast<int>(graphics.color.format), graphics.color.bytes, static_cast<int>(graphics.color.tileMode),
                    graphics.renderExtent.width, graphics.renderExtent.height, static_cast<int>(graphics.topology),
                    graphics.viewport.x, graphics.viewport.y, graphics.viewport.width, graphics.viewport.height,
                    graphics.scissor.offset.x, graphics.scissor.offset.y, graphics.scissor.extent.width, graphics.scissor.extent.height, graphics.rectList ? 1 : 0);
                std::fflush(stderr);
            }
        }
        struct Program {
            ShaderRecompiler::ShaderBinary binary;
            std::uint32_t userDataBase;
            std::uint32_t firstUserSgpr = 8;
            std::vector<std::uint32_t> userData;
            std::array<ShaderRecompiler::MemoryRegion, 2> memory;
        };
        const auto programAddress = [&](std::uint32_t base) {
            const auto high = readRegister(queue.shader, base + 1);
            require((high & ~0xffu) == 0, "reserved graphics program address bits are set");
            return (static_cast<std::uint64_t>(readRegister(queue.shader, base)) << 8u) | (static_cast<std::uint64_t>(high) << 40u);
        };
        const auto prepare = [&](std::uint64_t address, std::uint8_t type, ShaderRecompiler::ShaderStage stage, std::uint32_t rsrc2, std::uint32_t userDataBase) {
            auto it = submission.shaders.upper_bound(address);
            require(it != submission.shaders.begin(), "graphics program does not belong to a registered shader");
            --it;
            const auto& snapshot = *it->second;
            require(address - snapshot.codeAddress < snapshot.code.size() * sizeof(std::uint32_t), "graphics program is outside registered shader code");
            require(snapshot.type == type, "graphics program refers to an incompatible shader binary type");
            const auto resources = readRegister(queue.shader, rsrc2);
            const auto userCount = ((resources >> 1u) & 0x1fu) | (((resources >> 27u) & 1u) << 5u);
            require(userCount <= 32, "graphics user SGPR count exceeds the register bank");
            const auto codeOffset = static_cast<std::size_t>((address - snapshot.codeAddress) / sizeof(std::uint32_t));
            Program result{
                {stage, address, std::span(snapshot.code).subspan(codeOffset), snapshot.headerAddress, snapshot.header},
                userDataBase,
                8,
                {},
                {{{snapshot.codeAddress, std::as_bytes(std::span(snapshot.code))}, {snapshot.headerAddress, snapshot.header}}}
            };
            for (std::uint32_t i = 0; i < userCount; ++i) result.userData.push_back(readRegister(queue.shader, userDataBase + i));
            return result;
        };
        using Stage = ShaderRecompiler::ShaderStage;
        using Role = ShaderRecompiler::ProgramRole;
        std::vector<Program> programs;
        std::vector<Role> roles;
        programs.reserve(5);
        roles.reserve(5);
        const auto append = [&](std::uint32_t base, std::uint8_t type, Stage stage, std::uint32_t resources, std::uint32_t users, Role role) {
            programs.push_back(prepare(programAddress(base), type, stage, resources, users));
            roles.push_back(role);
        };
        const auto initializeMerged = [&](Program& program, std::uint32_t pointerBase, bool pointerRequired) {
            program.firstUserSgpr = 0;
            program.userData.insert(program.userData.begin(), 8, 0);
            if (pointerRequired) {
                const auto low = readRegister(queue.shader, pointerBase);
                const auto high = readRegister(queue.shader, pointerBase + 1);
                const auto address = static_cast<std::uint64_t>(low) | (static_cast<std::uint64_t>(high) << 32u);
                require(address != 0, "merged shader user-data address is null");
                GuestMemory::CheckRange(reinterpret_cast<const void*>(address), 8, 4);
                program.userData[0] = low;
                program.userData[1] = high;
            }
        };
        if (graphics.stages.path == Graphics::ShaderPath::Tessellation) {
            append(0x148, 5, Stage::Local, 0x10b, 0x10c, Role::Local);
            append(0x108, 7, Stage::TessellationControl, 0x10b, 0x10c, Role::Hull);
            initializeMerged(programs.back(), 0x102, true);
            append(0x0c8, 2, Stage::TessellationEvaluation, 0x08b, 0x08c, Role::Domain);
        } else if (graphics.stages.path == Graphics::ShaderPath::Geometry) {
            const auto frontAddress = programAddress(0xc8);
            auto snapshot = submission.shaders.upper_bound(frontAddress);
            require(snapshot != submission.shaders.begin(), "geometry front program is not registered");
            --snapshot;
            const auto type = snapshot->second->type;
            require(type == 2 || type == 4, "invalid geometry front binary type");
            append(0xc8, type, Stage::Mesh, 0x8b, 0x8c, Role::Main);
            initializeMerged(programs.back(), 0x82, type == 4);
            if (type == 4) append(0x88, 6, Stage::Mesh, 0x8b, 0x8c, Role::GeometryBack);
        } else {
            append(0xc8, 2, Stage::Vertex, 0x8b, 0x8c, Role::Main);
        }
        append(0x008, 1, Stage::Fragment, 0x00b, 0x00c, Role::Fragment);
        programs.back().firstUserSgpr = 0;
        std::array<std::uint8_t, Graphics::MaxColorTargets> exportMappings{};
        for (std::uint32_t i = 0; i < graphics.colorCount; ++i) exportMappings[i] = graphics.colors[i].componentMapping;
        const auto pixel = Graphics::DecodePixelStageInfo(queue.context, std::span<const std::uint8_t>(exportMappings.data(), graphics.colorCount));
        std::vector<ShaderRecompiler::MemoryRegion> memory;
        std::vector<ShaderRecompiler::LinkedProgram> linked;
        for (std::size_t i = 0; i < programs.size(); ++i) {
            const auto& program = programs[i];
            memory.insert(memory.end(), program.memory.begin(), program.memory.end());
            linked.push_back({roles[i], program.binary, program.userDataBase, program.firstUserSgpr, program.userData});
        }
        timing.Mark("prepare");
        std::lock_guard gpuLock(gpuMutex);
        timing.Mark("gpu_mutex_wait");
        if (device == nullptr) device = std::make_shared<VulkanDevice>();
        timing.Mark("device_setup");
        const GuestMemory::MemoryAccessScope memoryScope(device.get(), [](void* context, std::uint64_t address, std::size_t bytes, bool writable) {
            static_cast<VulkanDevice*>(context)->ResolveMemory(address, bytes, writable);
        });
        ShaderMemory shaderMemory(memory);
        std::vector<ShaderRecompiler::RecompileResult> results;
        std::vector<Graphics::CompiledShader> stages;
        results.reserve(programs.size() + (graphics.rectList ? 2u : 0u));
        stages.reserve(programs.size());
        std::uint32_t pushCursorBytes = 0;
        for (std::size_t i = 0; i < programs.size(); ++i) {
            if (roles[i] == Role::GeometryBack) continue;
            const auto& program = programs[i];
            const auto waveSize = program.binary.stage == Stage::Fragment ? graphics.stages.fragmentWaveSize : graphics.stages.vertexWaveSize;
            ShaderRecompiler::RecompileRequest request{
                program.binary,
                {waveSize, program.firstUserSgpr, program.userData, std::nullopt, program.binary.stage == Stage::Fragment ? std::optional(pixel) : std::nullopt, program.binary.stage == Stage::Fragment ? std::nullopt : std::optional(Graphics::DecodeVertexStageInfo(program.binary.header, program.binary.headerAddress, program.userData)), memory},
                device->Target(),
                {0, 0, pushCursorBytes, Graphics::PipelinePushConstantBytes - pushCursorBytes},
                ShaderRecompiler::GraphicsCompileContext{program.firstUserSgpr, linked, graphics.stages.mesh, graphics.stages.tessellation, {drawParameters.indexAddress, drawParameters.indexCount, drawParameters.indexSize, drawParameters.instanceCount}}
            };
            PerformanceTimer shaderTiming("Driver.GraphicsShader");
            shaderMemory.Capture(request);
            shaderTiming.Mark("memory_capture");
            memory = shaderMemory.Regions();
            request.context.memory = memory;
            shaderTiming.Mark("request_memory");
            {
                StallGuard compileGuard("ShaderRecompiler::Recompile", "graphics", 0, 0, 15);
                results.push_back(ShaderRecompiler::Recompile(request));
            }
            if (!results.back().cacheHit) CountEvent(FrameTiming::ShaderCompiles);
            shaderTiming.Mark(results.back().cacheHit ? "cache_hit" : "compile");
            const auto& result = results.back();
            if (!drawParameters.indexed && i == 0) {
                const auto offsetValue = [&](std::int32_t sgpr) {
                    require(sgpr >= 0 && static_cast<std::uint32_t>(sgpr) >= program.firstUserSgpr, "invalid draw offset SGPR");
                    const auto index = static_cast<std::uint32_t>(sgpr) - program.firstUserSgpr;
                    require(index < program.userData.size(), "draw offset SGPR exceeds user data");
                    return program.userData[index];
                };
                if (drawParameters.firstVertex == 0 && result.vertexOffsetSgpr >= 0) drawParameters.firstVertex = offsetValue(result.vertexOffsetSgpr);
                if (result.instanceOffsetSgpr >= 0) drawParameters.firstInstance = offsetValue(result.instanceOffsetSgpr);
            }
            require(result.pushConstants.size() <= Graphics::PipelinePushConstantBytes - pushCursorBytes, "stage push constants exceed the pipeline push constant block");
            stages.push_back({program.binary.stage, &result, result.pushConstants.empty() ? 0u : pushCursorBytes});
            pushCursorBytes += static_cast<std::uint32_t>(result.pushConstants.size());
        }
        timing.Mark("shaders");
        if (graphics.rectList) {
            require(stages.size() == 2, "rect-list requires vertex and fragment programs");
            auto rectangle = ShaderRecompiler::BuildRectListShaders(results[0], results[1], device->Target());
            results.push_back(std::move(rectangle.control));
            results.push_back(std::move(rectangle.evaluation));
            stages.insert(stages.begin() + 1, {{Stage::TessellationControl, &results[2], 0}, {Stage::TessellationEvaluation, &results[3], 0}});
        }
        std::vector<Graphics::GuestMemorySnapshot> snapshots;
        for (const auto& region : memory) snapshots.push_back({region.guestAddress, region.bytes});
        timing.Mark("post_compile_prepare");
        if (submission.serial != textureEpochSerial) {
            textureEpochSerial = submission.serial;
            ++textureEpoch;
        }
        device->SetTextureValidationEpoch(textureEpoch);
        CountEvent(FrameTiming::Draws);
        device->EnqueueDraw(graphics, drawParameters, stages, snapshots);
        timing.Mark("draw_and_resource_release");
    }

    void includeSubmission(const Submission& submission, bool firstSegment) {
        if (frameTiming == nullptr) {
            require(frameSerial != std::numeric_limits<std::uint64_t>::max(), "frame serial overflow");
            frameTiming = std::make_shared<FrameTiming>(++frameSerial);
        }
        const auto dequeued = firstSegment ? submission.dequeued : FrameTiming::Clock::now();
        frameTiming->IncludeSubmission(submission.serial, submission.received, submission.enqueued, dequeued, firstSegment);
        if (firstSegment) {
            frameTiming->Add(frameTiming->Get("Submission", "copy"), submission.copied - submission.received, submission.commands.size() * sizeof(std::uint32_t));
            frameTiming->Add(frameTiming->Get("Submission", "validate"), submission.validated - submission.copied);
            frameTiming->Add(frameTiming->Get("Submission", "reserve_enqueue"), submission.enqueued - submission.validated);
        }
    }

    void execute(const Submission& submission) {
        includeSubmission(submission, true);
        if (submission.suspend) {
            PerformanceContext timingContext(frameTiming.get());
            PerformanceTimer timing("Driver.Suspend");
            std::lock_guard gpuLock(gpuMutex);
            timing.Mark("gpu_mutex_wait");
            if (device != nullptr) device->WaitIdle();
            timing.Mark("device_idle_wait");
            resetGraphics = true;
            return;
        }
        if (submission.queue == 0 && resetGraphics) {
            queues.erase(0);
            resetGraphics = false;
        }
        auto& queue = queues[submission.queue];
        for (std::size_t cursor = 0; cursor < submission.commands.size();) {
            if (frameTiming == nullptr) includeSubmission(submission, false);
            const auto header = submission.commands[cursor];
            const auto count = static_cast<std::size_t>((header >> 16u) & 0x3fffu) + 2;
            const auto packet = std::span(submission.commands).subspan(cursor, count);
            const auto opcode = (header >> 8u) & 0xffu;
            // Packets that can write guest memory (dispatch, WRITE_DATA, DMA, RELEASE_MEM, ...) end the current texture validation epoch.
            if (opcode == 0x15 || opcode == 0x16 || opcode == 0x37 || opcode == 0x40 || opcode == 0x46 || opcode == 0x49 || opcode == 0x50) {
                ++textureEpoch;
                PerformanceContext bumpContext(frameTiming.get());
                PerformanceTimer bumpTiming("Graphics.TextureCache.epoch_bump");
            }
            const auto flipIt = submission.flips.find(cursor);
            if (submission.skipped.count(cursor) != 0) {
                if (flipIt != submission.flips.end()) flipIt->second->GpuReady(nullptr);
                cursor += count;
                continue;
            }
            std::exception_ptr packetError;
            try {
                PerformanceContext timingContext(frameTiming.get());
                PerformanceTimer timing("Driver.Packet");
                CheckFailure();
                timing.Mark("failure_check");
                if (opcode == 0x37 || opcode == 0x40 || opcode == 0x50 || opcode == 0x42 || opcode == 0x49 || opcode == 0x46 || opcode == 0x58 || header == FlipPacketHeader) {
                    std::lock_guard gpuLock(gpuMutex);
                    timing.Mark("gpu_mutex_wait");
                    const auto eventType = opcode == 0x46 ? packet[1] & 0x3fu : 0u;
                    const auto memoryTransfer = opcode == 0x37 || opcode == 0x40 || opcode == 0x50;
                    // Draw inputs are snapshotted at enqueue and MemoryAccessScope::ResolveMemory already drains exactly the GPU work that overlaps a
                    // transfer's ranges, so a plain memory transfer needs no full drain; only the end-of-pipe events below signal completion.
                    const bool strictSync = StrictSyncEnabled();
                    const auto legacyWaitDraws = memoryTransfer || opcode == 0x42 || opcode == 0x49 || (opcode == 0x46 && (eventType == 0x07 || eventType == 0x0f || eventType == 0x10));
                    // Asynchronous model: transfers, pure ordering events (PFP_SYNC_ME, partial flushes) have no guest-visible effect of their own and
                    // RELEASE_MEM labels are deferred until their batch completes, so none of them drains the GPU. strict_sync.txt restores the drains.
                    const auto waitDraws = strictSync && legacyWaitDraws;
                    const auto gpuCacheBarrier = opcode == 0x58 && Pm4::UsesGpuCacheBarrier(packet);
                    if (device != nullptr) {
                        if (gpuCacheBarrier) device->AcquireGpuMemory();
                        else if (legacyWaitDraws && !strictSync) {
                            // no drain: hazards are resolved per range by ResolveMemory when the packet executes
                        } else if (waitDraws) device->WaitDraws();
                        else if (!strictSync && header != FlipPacketHeader) {
                            // ACQUIRE_MEM / cache-flush events only order GPU cache state; guest-visible hazards are tracked per range, so no full drain.
                        } else {
                            const auto scope = header == FlipPacketHeader ? "Driver.FlipWait" : opcode == 0x58 ? "Driver.AcquireMemoryWait" : "Driver.CacheEventWait";
                            PerformanceTimer waitTiming(scope);
                            device->WaitIdle();
                        }
                    }
                    timing.Mark(gpuCacheBarrier ? "gpu_cache_barrier" : waitDraws ? "draw_wait" : "device_idle_wait");
                }
                if (header == RenderingWaitPacketHeader) {
                    submission.renderingWaits.at(cursor)->Wait();
                    timing.Mark("rendering_wait");
                } else if (header == FlipPacketHeader) {
                    CheckFailure();
                    timing.Mark("flip_prepare");
                } else if (opcode == 0x15) {
                    dispatch(queue, packet, submission);
                } else if (opcode == 0x16) {
                    std::array<std::uint32_t, 5> direct;
                    {
                        std::lock_guard gpuLock(gpuMutex);
                        const GuestMemory::MemoryAccessScope memoryScope(device.get(), [](void* context, std::uint64_t address, std::size_t bytes, bool writable) {
                            if (context) static_cast<VulkanDevice*>(context)->ResolveMemory(address, bytes, writable);
                        });
                        direct = Pm4::ResolveDispatch(packet, queue);
                    }
                    dispatch(queue, direct, submission);
                } else if (opcode == 0x3c || opcode == 0x93) {
                    waitRegMem(packet);
                } else if (opcode == 0x35 || opcode == 0x2d || opcode == 0x27) {
                    draw(queue, packet, submission);
                } else if (opcode == 0x49 && !StrictSyncEnabled() && device != nullptr) {
                    // RELEASE_MEM label: land it when everything recorded so far has completed, without draining the GPU here.
                    std::lock_guard gpuLock(gpuMutex);
                    auto copy = std::make_shared<std::vector<std::uint32_t>>(packet.begin(), packet.end());
                    device->DeferUntilDrawsComplete([copy] {
                        QueueState scratch{};
                        Pm4::Execute(*copy, scratch);
                    });
                    CountEvent(FrameTiming::LabelsDeferred);
                    timing.Mark("label_deferred");
                } else if (opcode != 0x42 && opcode != 0x46 && opcode != 0x58) {
                    std::lock_guard gpuLock(gpuMutex);
                    const GuestMemory::MemoryAccessScope memoryScope(device.get(), [](void* context, std::uint64_t address, std::size_t bytes, bool writable) {
                        if (context) static_cast<VulkanDevice*>(context)->ResolveMemory(address, bytes, writable);
                    });
                    Pm4::Execute(packet, queue);
                    timing.Mark("pm4_execute");
                }
            } catch (...) {
                packetError = std::current_exception();
                std::string detail = "packet execution failed";
                try {
                    if (packetError) std::rethrow_exception(packetError);
                } catch (const std::exception& error) {
                    detail += ": ";
                    detail += error.what();
                } catch (...) {
                }
                char tail[64];
                std::snprintf(tail, sizeof(tail), " [packet header=0x%x program=0x%llx]", header,
                              static_cast<unsigned long long>(failingProgram));
                detail += tail;
                failingProgram = 0;
                LogTolerated("Execute", detail.c_str());
            }
            if (flipIt != submission.flips.end()) {
                if (packetError == nullptr) {
                    Graphics::DepthFrameEpoch.fetch_add(1, std::memory_order_relaxed);
                    if (frameTiming != nullptr) {
                        {
                            static long long lastBusy = 0, lastIdle = 0;
                            const auto busy = g_busyNs.load(), idle = g_idleNs.load();
                            frameTiming->Count(FrameTiming::WorkerBusyUs, static_cast<std::uint64_t>((busy - lastBusy) / 1000));
                            frameTiming->Count(FrameTiming::WorkerIdleUs, static_cast<std::uint64_t>((idle - lastIdle) / 1000));
                            lastBusy = busy;
                            lastIdle = idle;
                            g_lastFlipNs.store(std::chrono::duration_cast<std::chrono::nanoseconds>(FrameTiming::Clock::now().time_since_epoch()).count());
                            {
                                static auto lastSave = FrameTiming::Clock::now();
                                if (device != nullptr && FrameTiming::Clock::now() - lastSave > std::chrono::seconds(30)) {
                                    lastSave = FrameTiming::Clock::now();
                                    device->SavePipelineCache();
                                }
                            }
                        }
                        frameTiming->SetFlip(submission.serial, cursor, submission.received, FrameTiming::Clock::now());
                        const auto completedFrame = std::exchange(frameTiming, nullptr);
                        flipIt->second->GpuReady(completedFrame);
                    } else {
                        flipIt->second->GpuReady(nullptr);
                    }
                } else {
                    flipIt->second->Fail(packetError);
                }
            }
            cursor += count;
        }
    }

    void run() noexcept {
        Submission submission;
        try {
            for (;;) {
                {
                    PerformanceContext timingContext(frameTiming.get());
                    PerformanceTimer timing("Driver.Worker");
                    submission = Submission{};
                    timing.Mark("submission_release");
                    std::unique_lock lock(mutex);
                    timing.Mark("queue_mutex_wait");
                    const auto idleStart = FrameTiming::Clock::now();
                    while (!(failure || stopping || !pending.empty())) {
                        if (device != nullptr && device->HasDeferred()) {
                            // Deferred labels must land even when the guest submits nothing new: poll the GPU while idle.
                            changed.wait_for(lock, std::chrono::microseconds(500));
                            lock.unlock();
                            {
                                std::lock_guard gpuLock(gpuMutex);
                                device->PollCompletions();
                            }
                            lock.lock();
                        } else {
                            changed.wait(lock);
                        }
                    }
                    timing.Mark("wait_for_submission");
                    g_idleNs += std::chrono::duration_cast<std::chrono::nanoseconds>(FrameTiming::Clock::now() - idleStart).count();
                    rethrowFailure();
                    if (pending.empty()) {
                        break;
                    }
                    submission = std::move(pending.front());
                    pending.pop_front();
                    submission.dequeued = FrameTiming::Clock::now();
                }
                const auto executeStart = FrameTiming::Clock::now();
                execute(submission);
                pipeWorkerNs += std::chrono::duration_cast<std::chrono::nanoseconds>(FrameTiming::Clock::now() - executeStart).count();
                g_busyNs += std::chrono::duration_cast<std::chrono::nanoseconds>(FrameTiming::Clock::now() - executeStart).count();
                if (pipeSubmissions.fetch_add(1) % 120 == 119) {
                    const auto guest = pipeGuestNs.exchange(0);
                    const auto worker = pipeWorkerNs.exchange(0);
                    const auto suspend = pipeSuspendNs.exchange(0);
                    const auto suspendCalls = pipeSuspendCalls.exchange(0);
                    const auto waitIdle = pipeWaitIdleNs.exchange(0);
                    const auto waitIdleCalls = pipeWaitIdleCalls.exchange(0);
                    if (VerboseLog()) std::fprintf(stderr, "[PIPE] submissions=120 guest_produce=%.3fms worker_execute=%.3fms suspend=%.3fms/%lld wait_idle=%.3fms/%lld\n",
                        guest / 1e6, worker / 1e6, suspend / 1e6, static_cast<long long>(suspendCalls), waitIdle / 1e6, static_cast<long long>(waitIdleCalls));
                    std::fflush(stderr);
                }
                {
                    PerformanceContext timingContext(frameTiming.get());
                    PerformanceTimer timing("Driver.SubmissionCompletion");
                    std::lock_guard gpuLock(gpuMutex);
                    if (device && StrictSyncEnabled()) device->WaitIdle();
                }
                {
                    PerformanceContext timingContext(frameTiming.get());
                    PerformanceTimer timing("Driver.Completion");
                    std::lock_guard lock(mutex);
                    timing.Mark("mutex_wait");
                    rethrowFailure();
                    completed = submission.serial;
                }
                changed.notify_all();
            }
            std::lock_guard gpuLock(gpuMutex);
            device.reset();
        } catch (...) {
            const auto error = std::current_exception();
            for (const auto& [offset, flip] : submission.flips) flip->Fail(error);
            ReportFailure(error);
            {
                std::lock_guard gpuLock(gpuMutex);
                device.reset();
            }
        }
    }
};

}

void Submit(const Packet* packet, std::uint32_t queue) {
    Driver::Get().Submit(packet, queue);
}

void WaitIdle() {
    Driver::Get().WaitIdle();
}

void RegisterShader(const Shader* shader) {
    Driver::Get().RegisterShader(shader);
}

void SuspendPoint() {
    Driver::Get().SuspendPoint();
}

void RegisterVideoOutput(std::uint32_t handle, const std::shared_ptr<IVideoOutput>& output) {
    Driver::Get().RegisterVideoOutput(handle, output);
}

void UnregisterVideoOutput(std::uint32_t handle, const std::shared_ptr<IVideoOutput>& output) {
    Driver::Get().UnregisterVideoOutput(handle, output);
}

void PresentClear(const PresentationWindow& window, bool opaque, void (*gpuReady)(void*), void* context) {
    Driver::Get().Present(window, nullptr, opaque, gpuReady, context);
}

void PresentBuffer(const PresentationWindow& window, const DisplayBuffer& buffer, void (*gpuReady)(void*), void* context) {
    Driver::Get().Present(window, &buffer, true, gpuReady, context);
}

void ReleaseWindow(void* window) {
    Driver::Get().ReleaseWindow(window);
}

void ReportFailure(std::exception_ptr error) {
    Driver::Get().ReportFailure(error);
}

}

extern "C" void AgcDriverWaitIdle_nid_postfix() {
    AgcDriver::WaitIdle();
}

extern "C" void AgcDriverRegisterShader_nid_postfix(const Shader* shader) {
    AgcDriver::RegisterShader(shader);
}

extern "C" void AgcDriverSuspendPoint_nid_postfix() {
    AgcDriver::SuspendPoint();
}

extern "C" void AgcDriverRegisterVideoOutput_nid_postfix(std::uint32_t handle, const std::shared_ptr<AgcDriver::IVideoOutput>& output) {
    AgcDriver::RegisterVideoOutput(handle, output);
}

extern "C" void AgcDriverUnregisterVideoOutput_nid_postfix(std::uint32_t handle, const std::shared_ptr<AgcDriver::IVideoOutput>& output) {
    AgcDriver::UnregisterVideoOutput(handle, output);
}

extern "C" void AgcDriverPresentClear_nid_postfix(const AgcDriver::PresentationWindow& window, bool opaque, void (*gpuReady)(void*), void* context) {
    AgcDriver::PresentClear(window, opaque, gpuReady, context);
}

extern "C" void AgcDriverPresentBuffer_nid_postfix(const AgcDriver::PresentationWindow& window, const AgcDriver::DisplayBuffer& buffer, void (*gpuReady)(void*), void* context) {
    AgcDriver::PresentBuffer(window, buffer, gpuReady, context);
}

extern "C" void AgcDriverReleaseWindow_nid_postfix(void* window) {
    AgcDriver::ReleaseWindow(window);
}

extern "C" void AgcDriverReportFailure_nid_postfix(std::exception_ptr error) {
    AgcDriver::ReportFailure(error);
}
