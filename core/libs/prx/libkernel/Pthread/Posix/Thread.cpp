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

int APS5_VABI pthread_create_nid_postfix(Pthread* thread, const PthreadAttr* attr, pthread_entry_func_t entry, void* arg) {
 (void)thread;
 (void)attr;
 (void)entry;
 (void)arg;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_create_name_np_nid_postfix(Pthread* thread, const PthreadAttr* attr, pthread_entry_func_t entry, void* arg, const char* name) {
 (void)thread;
 (void)attr;
 (void)entry;
 (void)arg;
 (void)name;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_detach_nid_postfix(Pthread thread) {
 (void)thread;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI pthread_exit_nid_postfix(void* value) {
 (void)value;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI pthread_getschedparam_nid_postfix(Pthread thread, int* policy, KernelSchedParam* param) {
 (void)thread;
 (void)policy;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_join_nid_postfix(Pthread thread, void** value) {
 (void)thread;
 (void)value;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_rename_np_nid_postfix(Pthread thread, const char* name) {
 (void)thread;
 (void)name;
 return SoftFailNp(__func__);
 return 0;
}
Pthread APS5_VABI pthread_self_nid_postfix(void) {
 return nullptr;  // SoftFailNp: pointer-typed return
 return {};
}
int APS5_VABI pthread_setcancelstate_nid_postfix(int state, int* old_state) {
 (void)state;
 (void)old_state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_setprio_nid_postfix(Pthread thread, int prio) {
 (void)thread;
 (void)prio;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_setschedparam_nid_postfix(Pthread thread, int policy, const KernelSchedParam* param) {
 (void)thread;
 (void)policy;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI pthread_yield_nid_postfix(void) {
 SoftFailNp(__func__);
 return;
}
}
