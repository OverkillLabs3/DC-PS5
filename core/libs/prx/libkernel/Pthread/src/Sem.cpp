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

int APS5_VABI scePthreadSemDestroy(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadSemGetvalue(void* sem, int* value) {
 (void)sem;
 (void)value;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadSemInit(void* sem, int flag, unsigned int value, const char* name) {
 (void)sem;
 (void)flag;
 (void)value;
 (void)name;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadSemPost(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadSemTimedwait(void* sem, KernelUseconds usec) {
 (void)sem;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadSemTrywait(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI scePthreadSemWait(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
}
