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

int APS5_VABI pthread_attr_destroy_nid_postfix(PthreadAttr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_get_np_nid_postfix(Pthread thread, PthreadAttr* attr) {
 (void)thread;
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_getdetachstate_nid_postfix(const PthreadAttr* attr, int* state) {
 (void)attr;
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_getguardsize_nid_postfix(const PthreadAttr* attr, size_t* guard_size) {
 (void)attr;
 (void)guard_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_getschedparam_nid_postfix(const PthreadAttr* attr, KernelSchedParam* param) {
 (void)attr;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_getschedpolicy_nid_postfix(const PthreadAttr* attr, int* policy) {
 (void)attr;
 (void)policy;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_getstack_nid_postfix(const PthreadAttr* __restrict attr, void** __restrict stack_addr, size_t* __restrict stack_size) {
 (void)attr;
 (void)stack_addr;
 (void)stack_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_getstacksize_nid_postfix(const PthreadAttr* attr, size_t* stack_size) {
 (void)attr;
 (void)stack_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_init_nid_postfix(PthreadAttr* attr) {
 (void)attr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_setdetachstate_nid_postfix(PthreadAttr* attr, int state) {
 (void)attr;
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_setguardsize_nid_postfix(PthreadAttr* attr, size_t guard_size) {
 (void)attr;
 (void)guard_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_setinheritsched_nid_postfix(PthreadAttr* attr, int inherit_sched) {
 (void)attr;
 (void)inherit_sched;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_setschedparam_nid_postfix(PthreadAttr* attr, const KernelSchedParam* param) {
 (void)attr;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_setschedpolicy_nid_postfix(PthreadAttr* attr, int policy) {
 (void)attr;
 (void)policy;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_attr_setstacksize_nid_postfix(PthreadAttr* attr, size_t stack_size) {
 (void)attr;
 (void)stack_size;
 return SoftFailNp(__func__);
 return 0;
}
}
