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

int APS5_VABI sceNetCtlCheckCallback(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlGetInfo(int code, NetCtlInfo* info) {
 (void)code;
 (void)info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlGetNatInfo(NetCtlNatInfo* nat_info) {
 (void)nat_info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlGetResult(int event_type, int* error_code) {
 (void)event_type;
 (void)error_code;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlGetState(int* state) {
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlGetStateV6(int* state) {
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlInit(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetCtlRegisterCallback(NetCtlCallback func, void* arg, int* cid) {
 (void)func;
 (void)arg;
 (void)cid;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceNetCtlTerm(void) {
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceNetCtlUnregisterCallback(int cid) {
 (void)cid;
 return SoftFailNp(__func__);
 return 0;
}
}
