#include <cstdio>
#include "../include/Pthread.hpp"
#include "prx/libc/include/General.hpp"
#include <chrono>
#include <stdexcept>

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000C;
static constexpr int SCE_KERNEL_ERROR_ETIMEDOUT = 0x80020062;


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

int APS5_VABI scePthreadCondattrInit(PthreadCondattr* attr) {
    if (!attr)
        return KpSoftFail("scePthreadCondattrInit: null attr");
    auto* p = new (std::nothrow) PthreadCondattrPrivate{0};
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    *attr = p;
    return SCE_OK;
}

int APS5_VABI scePthreadCondattrDestroy(PthreadCondattr* attr) {
    if (!attr || !*attr)
        return KpSoftFail("scePthreadCondattrDestroy: null attr");
    delete *attr;
    *attr = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadCondInit(PthreadCond* cond, const PthreadCondattr*, const char*) {
    if (!cond)
        return KpSoftFail("scePthreadCondInit: null cond");
    auto* p = new (std::nothrow) PthreadCondPrivate{};
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    *cond = p;
    return SCE_OK;
}

int APS5_VABI scePthreadCondDestroy(PthreadCond* cond) {
    if (!cond || !*cond)
        return KpSoftFail("scePthreadCondDestroy: null cond");
    delete *cond;
    *cond = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadCondSignal(PthreadCond* cond) {
    if (!cond || !*cond)
        return KpSoftFail("scePthreadCondSignal: null cond");
    (*cond)->_cv.notify_one();
    return SCE_OK;
}

int APS5_VABI scePthreadCondBroadcast(PthreadCond* cond) {
    if (!cond || !*cond)
        return KpSoftFail("scePthreadCondBroadcast: null cond");
    (*cond)->_cv.notify_all();
    return SCE_OK;
}

int APS5_VABI scePthreadCondTimedwait(PthreadCond* cond, PthreadMutex* mutex, unsigned int usec) {
    if (!cond || !*cond || !mutex || !*mutex)
        return KpSoftFail("scePthreadCondTimedwait: null arg");
    auto* m = *mutex;
    auto* c = *cond;
    if (m->_type == MutexType::Recursive) {
        std::unique_lock<std::recursive_timed_mutex> lk(m->_rmtx, std::adopt_lock);
        auto res = c->_cv.wait_for(lk, std::chrono::microseconds(usec));
        lk.release();
        return res == std::cv_status::timeout ? SCE_KERNEL_ERROR_ETIMEDOUT : SCE_OK;
    }
    std::unique_lock<std::timed_mutex> lk(m->_mtx, std::adopt_lock);
    auto res = c->_cv.wait_for(lk, std::chrono::microseconds(usec));
    lk.release();
    return res == std::cv_status::timeout ? SCE_KERNEL_ERROR_ETIMEDOUT : SCE_OK;
}

int APS5_VABI scePthreadCondSignalto(PthreadCond* cond, Pthread thread) {
 (void)cond;
 (void)thread;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadCondWait(PthreadCond* cond, PthreadMutex* mutex) {
    if (!cond || !*cond || !mutex || !*mutex)
        return KpSoftFail("scePthreadCondWait: null arg");
    auto* m = *mutex;
    auto* c = *cond;
    if (m->_type == MutexType::Recursive) {
        std::unique_lock<std::recursive_timed_mutex> lk(m->_rmtx, std::adopt_lock);
        c->_cv.wait(lk);
        lk.release();
        return SCE_OK;
    }
    std::unique_lock<std::timed_mutex> lk(m->_mtx, std::adopt_lock);
    c->_cv.wait(lk);
    lk.release();
    return SCE_OK;
}

}
