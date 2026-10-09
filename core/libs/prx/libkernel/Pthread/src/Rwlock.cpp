#include "../include/Pthread.hpp"
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

int APS5_VABI scePthreadRwlockDestroy(PthreadRwlock* rwlock) {
 (void)rwlock;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockInit(PthreadRwlock* rwlock, const PthreadRwlockattr* attr, const char* name) {
 (void)rwlock;
 (void)attr;
 (void)name;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockRdlock(PthreadRwlock* rwlock) {
 (void)rwlock;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockTryrdlock(PthreadRwlock* rwlock) {
 (void)rwlock;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockTrywrlock(PthreadRwlock* rwlock) {
 (void)rwlock;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockUnlock(PthreadRwlock* rwlock) {
 (void)rwlock;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockWrlock(PthreadRwlock* rwlock) {
 (void)rwlock;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockattrDestroy(PthreadRwlockattr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockattrInit(PthreadRwlockattr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadRwlockattrSettype(PthreadRwlockattr* attr, int type) {
 (void)attr;
 (void)type;
 return SoftFailNp(__func__);
 return 0;
}
}
