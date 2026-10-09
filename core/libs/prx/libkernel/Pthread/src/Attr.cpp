#include <cstdio>
#include "../include/Pthread.hpp"
#include "prx/libc/include/General.hpp"
#include <stdexcept>

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000C;

static constexpr std::size_t DEFAULT_STACK_SIZE = 1u << 20;
static constexpr int DETACH_JOINABLE = 0;
static constexpr int DETACH_DETACHED = 1;
static constexpr int SCHED_FIFO_PS5 = 1;

#ifdef _WIN32
#include <windows.h>
#include <limits>
#endif


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

int APS5_VABI scePthreadAttrInit(PthreadAttr* attr) {
    if (!attr)
        return KpSoftFail("scePthreadAttrInit: null attr");
    auto* p = new (std::nothrow) PthreadAttrPrivate{};
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    p->_stacksize = DEFAULT_STACK_SIZE;
    p->_detachstate = DETACH_JOINABLE;
    p->_schedpriority = 700;
    p->_schedpolicy = SCHED_FIFO_PS5;
    p->_inheritsched = 4;
    *attr = p;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrDestroy(PthreadAttr* attr) {
    if (!attr || !*attr)
        return KpSoftFail("scePthreadAttrDestroy: null attr");
    delete *attr;
    *attr = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrSetdetachstate(PthreadAttr* attr, int detachstate) {
    if (!attr || !*attr)
        return KpSoftFail("scePthreadAttrSetdetachstate: null attr");
    if (detachstate != DETACH_JOINABLE && detachstate != DETACH_DETACHED)
        return KpSoftFail("scePthreadAttrSetdetachstate: invalid state");
    (*attr)->_detachstate = detachstate;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrSetschedparam(PthreadAttr* attr, const KernelSchedParam* param) {
    if (!attr || !*attr || !param)
        return KpSoftFail("scePthreadAttrSetschedparam: null arg");
    (*attr)->_schedpriority = param->sched_priority;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrSetstacksize(PthreadAttr* attr, std::size_t stacksize) {
    if (!attr || !*attr)
        return KpSoftFail("scePthreadAttrSetstacksize: null attr");
    if (stacksize < 16384)
        return KpSoftFail("scePthreadAttrSetstacksize: too small");
#ifdef _WIN32
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    if (stacksize % system.dwPageSize != 0 || stacksize > std::numeric_limits<unsigned>::max())
        return KpSoftFail("scePthreadAttrSetstacksize: invalid Windows stack size");
#endif
    (*attr)->_stacksize = stacksize;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrGetstack(const PthreadAttr* attr, void** stackaddr, std::size_t* stacksize) {
    if (!attr || !*attr || !stackaddr || !stacksize)
        return KpSoftFail("scePthreadAttrGetstack: null arg");
    *stackaddr = (*attr)->stackAddress;
    *stacksize = (*attr)->_stacksize;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrGet(Pthread thread, PthreadAttr* attr) {
    if (!thread || !attr || !*attr)
        return KpSoftFail("scePthreadAttrGet: null arg");
    (*attr)->_stacksize = thread->stackSize;
    (*attr)->stackAddress = thread->stackAddress;
    (*attr)->_detachstate = thread->_detached ? DETACH_DETACHED : DETACH_JOINABLE;
    (*attr)->_schedpriority = 700;
    (*attr)->_schedpolicy = SCHED_FIFO_PS5;
    (*attr)->_inheritsched = 4;
    return SCE_OK;
}

int APS5_VABI scePthreadAttrGetaffinity(const PthreadAttr* attr, KernelCpumask* mask) {
 (void)attr;
 (void)mask;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrGetdetachstate(const PthreadAttr* attr, int* state) {
 (void)attr;
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrGetguardsize(const PthreadAttr* attr, size_t* guard_size) {
 (void)attr;
 (void)guard_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrGetschedparam(const PthreadAttr* attr, KernelSchedParam* param) {
 (void)attr;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrGetsolosched(const PthreadAttr* attr, int* solosched) {
 (void)attr;
 (void)solosched;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrGetstackaddr(const PthreadAttr* attr, void** stack_addr) {
 (void)attr;
 (void)stack_addr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrGetstacksize(const PthreadAttr* attr, size_t* stack_size) {
 (void)attr;
 (void)stack_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetaffinity(PthreadAttr* attr, KernelCpumask mask) {
 (void)attr;
 (void)mask;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetguardsize(PthreadAttr* attr, size_t guard_size) {
 (void)attr;
 (void)guard_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetinheritsched(PthreadAttr* attr, int inherit_sched) {
 (void)attr;
 (void)inherit_sched;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetschedpolicy(PthreadAttr* attr, int policy) {
 (void)attr;
 (void)policy;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetsolosched(PthreadAttr* attr, int solosched) {
 (void)attr;
 (void)solosched;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetstack(PthreadAttr* attr, void* addr, size_t size) {
 (void)attr;
 (void)addr;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadAttrSetstackaddr(PthreadAttr* attr, void* addr) {
 (void)attr;
 (void)addr;
 return SoftFailNp(__func__);
 return 0;
}
}
