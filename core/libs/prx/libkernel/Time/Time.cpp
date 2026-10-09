#include "prx/libkernel/Time/include/Time.hpp"

#include "prx/libc/include/General.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

// Windows' default system timer resolution is ~15.6 ms. std::condition_variable::wait_for in
// MinGW libstdc++ bottoms out on that tick, so every guest timed wait in libkernel (condvar,
// semaphore, eventflag) rounds up to 15.6 ms. The game's audio producer needs a 5.3 ms cadence
// (256 frames @ 48 kHz) and underruns continuously otherwise. Raising the resolution is
// process-wide and fixes all of those waits at once; SleepNanos's high-resolution timer only
// covers sceKernelUsleep/nanosleep.
#ifdef _WIN32
namespace {
using NtQueryTimerResolution_t = LONG(NTAPI*)(PULONG, PULONG, PULONG);
using NtSetTimerResolution_t = LONG(NTAPI*)(ULONG, BOOLEAN, PULONG);

struct ProcessTimerResolution {
    ProcessTimerResolution() {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll == nullptr) {
            return;
        }
        auto query = reinterpret_cast<NtQueryTimerResolution_t>(
            GetProcAddress(ntdll, "NtQueryTimerResolution"));
        auto set = reinterpret_cast<NtSetTimerResolution_t>(
            GetProcAddress(ntdll, "NtSetTimerResolution"));
        if (set == nullptr) {
            return;
        }
        ULONG lowest = 0, highest = 0, current = 0;
        if (query != nullptr) {
            query(&lowest, &highest, &current);
        }
        ULONG before = current;
        // 100 ns units: 5000 = 0.5 ms, the smallest value Windows accepts in practice.
        LONG status = set(5000, TRUE, &current);
        if (query != nullptr) {
            query(&lowest, &highest, &current);
        }
        std::fprintf(stderr,
                     "[TIME] timer resolution 100ns units: requested=5000 min=%u max=%u "
                     "before=%u after=%u status=%ld\n",
                     lowest, highest, before, current, status);
        std::fflush(stderr);
    }
};
const ProcessTimerResolution g_process_timer_resolution;
}  // namespace
#endif

static std::uint64_t GetMonotonicNanos() {
#ifdef _WIN32
    static const std::uint64_t freq = [] {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return static_cast<std::uint64_t>(f.QuadPart);
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return static_cast<std::uint64_t>(counter.QuadPart) * 1000000000ULL / freq;
#else
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec);
#endif
}

static std::uint64_t GetStartNanos() {
    static const std::uint64_t start = GetMonotonicNanos();
    return start;
}

static void SleepNanos(std::uint64_t nanos) {
    if (nanos == 0) {
        return;
    }
#ifdef _WIN32
    // Sleep(ms) rounds to the ~15.6 ms system tick, which makes guest loops that poll with usleep(1000) about 15x slower than
    // on the console. A high-resolution waitable timer honours sub-millisecond requests.
    struct TimerHandle {
        HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        ~TimerHandle() { if (handle != nullptr) CloseHandle(handle); }
    };
    thread_local TimerHandle timer;
    if (timer.handle != nullptr) {
        LARGE_INTEGER due{};
        due.QuadPart = -static_cast<LONGLONG>(std::max<std::uint64_t>(nanos / 100ULL, 1ULL)); // relative, 100 ns units
        if (SetWaitableTimerEx(timer.handle, &due, 0, nullptr, nullptr, nullptr, 0) != FALSE) {
            WaitForSingleObject(timer.handle, INFINITE);
            return;
        }
    }
    const DWORD millis = static_cast<DWORD>((nanos + 999999ULL) / 1000000ULL);
    Sleep(millis);
#else
    struct timespec req{};
    req.tv_sec = static_cast<time_t>(nanos / 1000000000ULL);
    req.tv_nsec = static_cast<long>(nanos % 1000000000ULL);
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {}
#endif
}


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}

// Unsupported clock ids / null timeouts: POSIX returns an error, never aborts.
int KTimeoutErr(const char* what) {
    static int logs = 0;
    if (logs < 20) {
        ++logs;
        std::fprintf(stderr, "[TIME] %s -> EINVAL (tolerated #%d)\n", what, logs);
        std::fflush(stderr);
    }
    return -1;
}
}  // namespace

extern "C" {

std::uint64_t APS5_VABI sceKernelGetProcessTime() {
    return (GetMonotonicNanos() - GetStartNanos()) / 1000ULL;
}

std::uint64_t APS5_VABI sceKernelGetProcessTimeCounter() {
    return GetMonotonicNanos() - GetStartNanos();
}

std::uint64_t APS5_VABI sceKernelGetProcessTimeCounterFrequency() {
    return 1000000000ULL;
}

int APS5_VABI sceKernelUsleep_nid_postfix(KernelUseconds microseconds) {
    SleepNanos(static_cast<std::uint64_t>(microseconds) * 1000ULL);
    return 0;
}

int APS5_VABI sceKernelNanosleep(const KernelTimespec* rqtp, KernelTimespec* rmtp) {
    if (rqtp == nullptr) {
        return -1;
    }
    if (rqtp->tv_sec < 0 || rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1000000000LL) {
        return -1;
    }
    SleepNanos(static_cast<std::uint64_t>(rqtp->tv_sec) * 1000000000ULL +
               static_cast<std::uint64_t>(rqtp->tv_nsec));
    if (rmtp != nullptr) {
        rmtp->tv_sec = 0;
        rmtp->tv_nsec = 0;
    }
    return 0;
}

int APS5_VABI nanosleep_nid_postfix(const KernelTimespec* rqtp, KernelTimespec* rmtp) {
    return sceKernelNanosleep(rqtp, rmtp);
}

int APS5_VABI clock_gettime_nid_postfix(int clockId, KernelTimespec* tp) {
    if (tp == nullptr) {
        return KTimeoutErr(__func__);
    }
#ifdef _WIN32
    if (clockId == 0 || clockId == 9 || clockId == 10) {
        FILETIME ft{};
        GetSystemTimePreciseAsFileTime(&ft);
        std::uint64_t t = (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        t -= 116444736000000000ULL;
        t *= 100ULL;
        tp->tv_sec = static_cast<std::int64_t>(t / 1000000000ULL);
        tp->tv_nsec = static_cast<std::int64_t>(t % 1000000000ULL);
        return 0;
    }
    if (clockId == 4 || clockId == 1 || clockId == 5 || clockId == 7 || clockId == 8 || clockId == 11 || clockId == 12) {
        std::uint64_t nanos = GetMonotonicNanos();
        tp->tv_sec = static_cast<std::int64_t>(nanos / 1000000000ULL);
        tp->tv_nsec = static_cast<std::int64_t>(nanos % 1000000000ULL);
        return 0;
    }
    return KTimeoutErr(__func__);
#else
    clockid_t nativeId;
    switch (clockId) {
        case 0:
        case 9:
        case 10:
            nativeId = CLOCK_REALTIME;
            break;
        case 4:
        case 7:
        case 11:
            nativeId = CLOCK_MONOTONIC;
            break;
        case 5:
        case 8:
        case 12:
            nativeId = CLOCK_MONOTONIC;
            break;
        case 14:
            nativeId = CLOCK_THREAD_CPUTIME_ID;
            break;
        case 15:
            nativeId = CLOCK_PROCESS_CPUTIME_ID;
            break;
        default:
            return KTimeoutErr(__func__);
    }
    struct timespec ts{};
    if (clock_gettime(nativeId, &ts) != 0) {
        return -1;
    }
    tp->tv_sec = static_cast<std::int64_t>(ts.tv_sec);
    tp->tv_nsec = static_cast<std::int64_t>(ts.tv_nsec);
    return 0;
#endif
}

int APS5_VABI gettimeofday_nid_postfix(KernelTimeval* tv, KernelTimezone* tz) {
    if (tv == nullptr) {
        return KTimeoutErr(__func__);
    }
#ifdef _WIN32
    FILETIME ft{};
    GetSystemTimePreciseAsFileTime(&ft);
    std::uint64_t t = (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    t -= 116444736000000000ULL;
    tv->tv_sec = static_cast<std::int64_t>(t / 10000000ULL);
    tv->tv_usec = static_cast<std::int64_t>((t % 10000000ULL) / 10ULL);
#else
    struct timeval native{};
    if (gettimeofday(&native, nullptr) != 0) {
        return -1;
    }
    tv->tv_sec = static_cast<std::int64_t>(native.tv_sec);
    tv->tv_usec = static_cast<std::int64_t>(native.tv_usec);
#endif
    if (tz != nullptr) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }
    return 0;
}

int APS5_VABI clock_getres_nid_postfix(int clockId, KernelTimespec* res) {
    if (res == nullptr) {
        return KTimeoutErr(__func__);
    }
#ifdef _WIN32
    if (clockId == 0 || clockId == 9) {
        res->tv_sec = 0;
        res->tv_nsec = 100LL;
        return 0;
    }
    if (clockId == 4 || clockId == 1 || clockId == 5 || clockId == 7 || clockId == 8 || clockId == 10 || clockId == 11 || clockId == 12) {
        static const std::uint64_t freq = [] {
            LARGE_INTEGER f{};
            QueryPerformanceFrequency(&f);
            return static_cast<std::uint64_t>(f.QuadPart);
        }();
        std::uint64_t nsPerTick = (1000000000ULL + freq - 1ULL) / freq;
        res->tv_sec = 0;
        res->tv_nsec = static_cast<std::int64_t>(nsPerTick);
        return 0;
    }
    return KTimeoutErr(__func__);
#else
    clockid_t nativeId;
    switch (clockId) {
        case 0:
        case 9:
        case 10:
            nativeId = CLOCK_REALTIME;
            break;
        case 4:
        case 7:
        case 11:
            nativeId = CLOCK_MONOTONIC;
            break;
        case 5:
        case 8:
        case 12:
            nativeId = CLOCK_MONOTONIC;
            break;
        case 14:
            nativeId = CLOCK_THREAD_CPUTIME_ID;
            break;
        case 15:
            nativeId = CLOCK_PROCESS_CPUTIME_ID;
            break;
        default:
            return KTimeoutErr(__func__);
    }
    struct timespec ts{};
    if (clock_getres(nativeId, &ts) != 0) {
        return -1;
    }
    res->tv_sec = static_cast<std::int64_t>(ts.tv_sec);
    res->tv_nsec = static_cast<std::int64_t>(ts.tv_nsec);
    return 0;
#endif
}

// ---------------------------------------------------------------------------
// Moved as-is (not yet implemented) from the monolithic libkernel/Export.cpp.
// ---------------------------------------------------------------------------

int APS5_VABI sceKernelClockGetres(KernelClockid clock_id, KernelTimespec* tp) {
 return clock_getres_nid_postfix(static_cast<int>(clock_id), tp);
}

int APS5_VABI sceKernelClockGettime(KernelClockid clock_id, KernelTimespec* tp) {
 return clock_gettime_nid_postfix(static_cast<int>(clock_id), tp);
}

int APS5_VABI sceKernelConvertLocaltimeToUtc(int64_t local_time, int64_t reserved, int64_t* utc_time, KernelTimezone* timezone, int32_t* dst_seconds) {
 (void)local_time;
 (void)reserved;
 (void)utc_time;
 (void)timezone;
 (void)dst_seconds;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelConvertUtcToLocaltime(int64_t utc_time, int64_t* local_time, KernelTimesec* st, uint64_t* dst_sec) {
 (void)utc_time;
 (void)local_time;
 (void)st;
 (void)dst_sec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelGettimeofday(KernelTimeval* tp) {
 return gettimeofday_nid_postfix(tp, nullptr);
}

int APS5_VABI sceKernelGettimezone(KernelTimezone* tz) {
 (void)tz;
 return SoftFailNp(__func__);
 return 0;
}
// The TSC stream here is the monotonic nanosecond clock; guests use
// ReadTsc/GetTscFrequency as a ratio, so consistency matters more than the
// nominal PS5 hardware tick rate.
uint64_t APS5_VABI sceKernelReadTsc(void) {
 return GetMonotonicNanos();
}

uint64_t APS5_VABI sceKernelGetTscFrequency(void) {
 return 1000000000ULL;
}

unsigned int APS5_VABI sceKernelSleep(unsigned int seconds) {
 SleepNanos(static_cast<std::uint64_t>(seconds) * 1000000000ULL);
 return 0;
}

}
