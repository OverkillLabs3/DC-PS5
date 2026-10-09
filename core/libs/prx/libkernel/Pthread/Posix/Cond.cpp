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

int APS5_VABI pthread_cond_broadcast_nid_postfix(PthreadCond* cond) {
 (void)cond;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_cond_init_nid_postfix(PthreadCond* cond, const PthreadCondattr* attr) {
 (void)cond;
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_cond_signal_nid_postfix(PthreadCond* cond) {
 (void)cond;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_cond_timedwait_nid_postfix(PthreadCond* cond, PthreadMutex* mutex, const KernelTimespec* abstime) {
 (void)cond;
 (void)mutex;
 (void)abstime;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_cond_wait_nid_postfix(PthreadCond* cond, PthreadMutex* mutex) {
 (void)cond;
 (void)mutex;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_condattr_destroy_nid_postfix(PthreadCondattr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_condattr_init_nid_postfix(PthreadCondattr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_condattr_setclock_nid_postfix(PthreadCondattr* attr, KernelClockid clock_id) {
 (void)attr;
 (void)clock_id;
 return SoftFailNp(__func__);
 return 0;
}
}
