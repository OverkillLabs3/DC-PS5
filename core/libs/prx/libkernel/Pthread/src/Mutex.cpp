#include <cstdio>
#include "../include/Pthread.hpp"
#include "prx/libc/include/General.hpp"
#include <cerrno>
#include <chrono>
#include <stdexcept>

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000C;
static constexpr int SCE_KERNEL_ERROR_EDEADLK = 0x80020023;
static constexpr int SCE_KERNEL_ERROR_EPERM = 0x80020001;


namespace {
// Throwing from a pthread call reaches std::terminate and kills the game.
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
}  // namespace
namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

int APS5_VABI scePthreadMutexattrInit(PthreadMutexattr* attr) {
    if (!attr)
        return KpSoftFail("scePthreadMutexattrInit: null attr");
    auto* p = new (std::nothrow) PthreadMutexattrPrivate{MutexType::Normal};
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    *attr = p;
    return SCE_OK;
}

int APS5_VABI scePthreadMutexattrDestroy(PthreadMutexattr* attr) {
    if (!attr || !*attr)
        return KpSoftFail("scePthreadMutexattrDestroy: null attr");
    delete *attr;
    *attr = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadMutexattrSettype(PthreadMutexattr* attr, int type) {
    if (!attr || !*attr)
        return KpSoftFail("scePthreadMutexattrSettype: null attr");
    switch (type) {
    case 1: (*attr)->type = MutexType::ErrorCheck; break;
    case 2: (*attr)->type = MutexType::Recursive; break;
    case 3: (*attr)->type = MutexType::Normal; break;
    default: return KpSoftFail("scePthreadMutexattrSettype: invalid type");
    }
    return SCE_OK;
}

int APS5_VABI scePthreadMutexInit(PthreadMutex* mutex, const PthreadMutexattr* attr, const char*) {
    if (!mutex)
        return KpSoftFail("scePthreadMutexInit: null mutex");
    MutexType t = MutexType::Normal;
    if (attr && *attr) t = (*attr)->type;
    auto* p = new (std::nothrow) PthreadMutexPrivate();
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    p->_type = t;
    *mutex = p;
    return SCE_OK;
}

int APS5_VABI scePthreadMutexDestroy(PthreadMutex* mutex) {
    if (!mutex || !*mutex)
        return KpSoftFail("scePthreadMutexDestroy: null mutex");
    delete *mutex;
    *mutex = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadMutexLock(PthreadMutex* mutex) {
    if (!mutex || !*mutex)
        return KpSoftFail("scePthreadMutexLock: null mutex");
    auto* m = *mutex;
    const auto tid = std::this_thread::get_id();
    if (m->_type == MutexType::Recursive) {
        m->_rmtx.lock();
        m->_owner.store(tid, std::memory_order_relaxed);
        ++m->_count;
        return SCE_OK;
    }
    if (m->_type == MutexType::ErrorCheck) {
        if (m->_owner.load(std::memory_order_acquire) == tid) return SCE_KERNEL_ERROR_EDEADLK;
    }
    m->_mtx.lock();
    m->_owner.store(tid, std::memory_order_relaxed);
    return SCE_OK;
}

int APS5_VABI scePthreadMutexUnlock(PthreadMutex* mutex) {
    if (!mutex || !*mutex)
        return KpSoftFail("scePthreadMutexUnlock: null mutex");
    auto* m = *mutex;
    if (m->_type == MutexType::ErrorCheck || m->_type == MutexType::Normal) {
        if (m->_owner.load(std::memory_order_acquire) != std::this_thread::get_id())
            return SCE_KERNEL_ERROR_EPERM;
    }
    if (m->_type == MutexType::Recursive) {
        if (--m->_count == 0) m->_owner.store(std::thread::id{}, std::memory_order_relaxed);
        m->_rmtx.unlock();
        return SCE_OK;
    }
    m->_owner.store(std::thread::id{}, std::memory_order_relaxed);
    m->_mtx.unlock();
    return SCE_OK;
}

int APS5_VABI scePthreadMutexTimedlock(PthreadMutex* mutex, KernelUseconds usec) {
 (void)mutex;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadMutexTrylock(PthreadMutex* mutex) {
 (void)mutex;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadMutexattrSetprotocol(PthreadMutexattr* attr, int protocol) {
 (void)attr;
 (void)protocol;
 return SoftFailNp(__func__);
 return 0;
}
}
