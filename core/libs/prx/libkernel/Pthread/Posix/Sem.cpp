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

int APS5_VABI sem_init_nid_postfix(void* sem, int pshared, unsigned int value) {
 (void)sem;
 (void)pshared;
 (void)value;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_destroy_nid_postfix(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_wait_nid_postfix(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_trywait_nid_postfix(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_timedwait_nid_postfix(void* sem, const KernelTimespec* abstime) {
 (void)sem;
 (void)abstime;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_reltimedwait_np_nid_postfix(void* sem, uint32_t usec) {
 (void)sem;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_post_nid_postfix(void* sem) {
 (void)sem;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sem_getvalue_nid_postfix(void* sem, int* value) {
 (void)sem;
 (void)value;
 return SoftFailNp(__func__);
 return 0;
}
}
