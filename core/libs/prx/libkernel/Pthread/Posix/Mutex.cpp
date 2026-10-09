#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


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

int APS5_VABI pthread_mutex_destroy_nid_postfix(PthreadMutex* mutex) {
 (void)mutex;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutex_init_nid_postfix(PthreadMutex* mutex, const PthreadMutexattr* attr) {
 (void)mutex;
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutex_lock_nid_postfix(PthreadMutex* mutex) {
 (void)mutex;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutex_timedlock_nid_postfix(PthreadMutex* mutex, const KernelTimespec* abstime) {
 (void)mutex;
 (void)abstime;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutex_trylock_nid_postfix(PthreadMutex* mutex) {
 (void)mutex;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutex_unlock_nid_postfix(PthreadMutex* mutex) {
 (void)mutex;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutexattr_destroy_nid_postfix(PthreadMutexattr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutexattr_init_nid_postfix(PthreadMutexattr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutexattr_setprotocol_nid_postfix(PthreadMutexattr* attr, int protocol) {
 (void)attr;
 (void)protocol;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_mutexattr_settype_nid_postfix(PthreadMutexattr* attr, int type) {
 (void)attr;
 (void)type;
 return SoftFailNp(__func__);
 return 0;
}
}
