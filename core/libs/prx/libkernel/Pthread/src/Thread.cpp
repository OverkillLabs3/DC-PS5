#include <cstdio>
#include "../include/Pthread.hpp"
#include "prx/libc/include/General.hpp"
#include <cstdlib>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <system_error>

#ifndef _WIN32
#include <pthread.h>
#endif

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_EINVAL = 0x80020016;

static constexpr std::size_t DEFAULT_STACK_SIZE = 1u << 20;
static constexpr int DETACH_DETACHED = 1;

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <limits>
#endif

namespace {
// Throwing out of a pthread call reaches std::terminate and kills the game.
// POSIX says: report the error number to the caller instead.
int KpSoftFail(const char* what) {
    static int logs = 0;
    if (logs < 80) {
        ++logs;
        std::fprintf(stderr, "[KP] %s -> EINVAL (tolerated #%d)\n", what, logs);
        std::fflush(stderr);
    }
    return 22;  // EINVAL
}

// Unimplemented SCE API: log once per call site and no-op instead of throwing.
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

struct ThreadArgs {
    PthreadEntry entry;
    void* arg;
    PthreadPrivate* self;
};

static void FinishThread(PthreadPrivate* self, void* retval) {
    {
        std::unique_lock<std::mutex> lk(self->_join_mtx);
        self->_retval = retval;
        self->_finished.store(true, std::memory_order_release);
    }
    self->_join_cv.notify_all();
}

static void RunThread(std::unique_ptr<ThreadArgs> args) {
    APS5_LOG_OUT("RunThread entry=0x%llx arg=%p self=%p", static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(args->entry)), args->arg, static_cast<void*>(args->self));
    const auto entry = args->entry;
    void* arg = args->arg;
    PthreadPrivate* self = args->self;
    args.reset();
    FinishThread(self, entry(arg));
}

#ifdef _WIN32
static thread_local PthreadPrivate* currentThread = nullptr;

static void ReleaseThread(PthreadPrivate* thread) {
    if (thread->references.fetch_sub(1, std::memory_order_acq_rel) != 1)
        return;
    if (!CloseHandle(thread->nativeHandle))
        KpSoftFail("ReleaseThread: CloseHandle failed");
    delete thread;
}

struct NativeThreadArgs {
    std::unique_ptr<ThreadArgs> guest;
    std::future<bool> start;
    std::promise<void> initialized;
};

// Stack checks throw inside the try block on purpose: scePthreadCreate's
// catch cleans up the half-started native thread and returns EINVAL.
static unsigned __stdcall StartNativeThread(void* opaque) {
    std::unique_ptr<NativeThreadArgs> args(static_cast<NativeThreadArgs*>(opaque));
    auto* self = args->guest->self;
    try {
        ULONG_PTR low = 0;
        ULONG_PTR high = 0;
        GetCurrentThreadStackLimits(&low, &high);
        if (high <= low || high - low < self->stackSize)
            throw std::runtime_error("Cannot query guest thread stack");
        self->stackAddress = reinterpret_cast<void*>(high - self->stackSize);
        for (auto cursor = high - self->stackSize; cursor < high;) {
            MEMORY_BASIC_INFORMATION memory{};
            if (VirtualQuery(reinterpret_cast<void*>(cursor), &memory, sizeof(memory)) != sizeof(memory) || memory.State != MEM_COMMIT || memory.Protect != PAGE_READWRITE || memory.RegionSize == 0)
                throw std::runtime_error("Guest thread stack is not fully committed");
            cursor = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
        }
        self->threadId = std::this_thread::get_id();
        currentThread = self;
        args->initialized.set_value();
    } catch (...) {
        args->initialized.set_exception(std::current_exception());
        return 0;
    }
    if (!args->start.get())
        return 0;
    auto guest = std::move(args->guest);
    args.reset();
    RunThread(std::move(guest));
    currentThread = nullptr;
    ReleaseThread(self);
    return 0;
}
#endif

extern "C" {

int APS5_VABI scePthreadCreate(Pthread* thread, const PthreadAttr* attr, PthreadEntry entry, void* arg, const char*) {
    if (!thread || !entry)
        return KpSoftFail("scePthreadCreate: null arg");
    if (attr && !*attr)
        return KpSoftFail("scePthreadCreate: null attributes");
    auto p = std::make_unique<PthreadPrivate>();
    bool detached = false;
    if (attr && *attr) detached = ((*attr)->_detachstate == DETACH_DETACHED);
    p->_detached = detached;
    p->stackSize = attr ? (*attr)->_stacksize : DEFAULT_STACK_SIZE;
    std::promise<bool> start;
    auto args = std::make_unique<ThreadArgs>(ThreadArgs{entry, arg, p.get()});
#ifdef _WIN32
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    if (p->stackSize < 16384 || p->stackSize % system.dwPageSize != 0 || p->stackSize > std::numeric_limits<unsigned>::max())
        return KpSoftFail("scePthreadCreate: invalid Windows stack size");
    auto native = std::make_unique<NativeThreadArgs>(NativeThreadArgs{std::move(args), start.get_future(), {}});
    auto initialized = native->initialized.get_future();
    const auto handle = _beginthreadex(nullptr, static_cast<unsigned>(p->stackSize), StartNativeThread, native.get(), 0, nullptr);
    if (handle == 0)
        return KpSoftFail("scePthreadCreate: _beginthreadex failed");
    p->nativeHandle = reinterpret_cast<void*>(handle);
    native.release();
    try {
        initialized.get();
    } catch (const std::exception&) {
        KpSoftFail("scePthreadCreate: guest thread stack setup failed");
        start.set_value(false);
        WaitForSingleObject(p->nativeHandle, INFINITE);
        CloseHandle(p->nativeHandle);
        return SCE_KERNEL_ERROR_EINVAL;
    }
    auto* published = p.release();
    *thread = published;
    start.set_value(true);
    if (detached)
        ReleaseThread(published);
#else
    p->_thr = std::thread([args = std::move(args), ready = start.get_future()]() mutable {
        if (ready.get()) RunThread(std::move(args));
    });
    try {
        if (detached) p->_thr.detach();
    } catch (...) {
        start.set_value(false);
        p->_thr.join();
        return KpSoftFail("scePthreadCreate: detach failed");
    }
    *thread = p.release();
    start.set_value(true);
#endif
    return SCE_OK;
}

int APS5_VABI scePthreadJoin(Pthread thread, void** retval) {
    if (!thread)
        return KpSoftFail("scePthreadJoin: null thread");
    if (thread->_detached) return SCE_KERNEL_ERROR_EINVAL;
#ifdef _WIN32
    if (thread == currentThread)
        return KpSoftFail("scePthreadJoin: cannot join current thread");
    if (WaitForSingleObject(thread->nativeHandle, INFINITE) != WAIT_OBJECT_0)
        return KpSoftFail("scePthreadJoin: wait failed");
    if (retval) *retval = thread->_retval;
    ReleaseThread(thread);
#else
    if (thread->_thr.joinable()) thread->_thr.join();
    if (retval) *retval = thread->_retval;
    delete thread;
#endif
    return SCE_OK;
}

int APS5_VABI scePthreadDetach(Pthread thread) {
    if (!thread)
        return KpSoftFail("scePthreadDetach: null thread");
    if (thread->_detached) return SCE_KERNEL_ERROR_EINVAL;
    thread->_detached = true;
#ifdef _WIN32
    ReleaseThread(thread);
#else
    if (thread->_thr.joinable()) thread->_thr.detach();
#endif
    return SCE_OK;
}

void APS5_VABI scePthreadExit(void* retval) {
#ifdef _WIN32
    if (!currentThread) {
        KpSoftFail("scePthreadExit: current thread is not registered");
        return;
    }
    auto* self = currentThread;
    FinishThread(self, retval);
    currentThread = nullptr;
    ReleaseThread(self);
    _endthreadex(0);
#else
    pthread_exit(retval);
#endif
    __builtin_unreachable();
}

Pthread APS5_VABI scePthreadSelf() {
#ifdef _WIN32
    return currentThread;
#else
    return nullptr;
#endif
}

void APS5_VABI scePthreadYield() {
    std::this_thread::yield();
}

int APS5_VABI scePthreadCancel(Pthread thread) {
 (void)thread;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadEqual(Pthread thread1, Pthread thread2) {
 (void)thread1;
 (void)thread2;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadGetaffinity(Pthread thread, KernelCpumask* mask) {
 (void)thread;
 (void)mask;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadGetname(Pthread thread, char* name) {
 (void)thread;
 (void)name;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadGetprio(Pthread thread, int* prio) {
 (void)thread;
 (void)prio;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadGetthreadid(void) {
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadRename(Pthread thread, const char* name) {
 (void)thread;
 (void)name;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadSetaffinity(Pthread thread, KernelCpumask mask) {
 (void)thread;
 (void)mask;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadSetcancelstate(int state, int* old_state) {
 (void)state;
 (void)old_state;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadSetcanceltype(int type, int* old_type) {
 (void)type;
 (void)old_type;
 return SoftFailNp(__func__);
}

int APS5_VABI scePthreadSetprio(Pthread thread, int prio) {
 (void)thread;
 (void)prio;
 return SoftFailNp(__func__);
}

}
