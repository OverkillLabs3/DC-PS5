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

int APS5_VABI sceKernelAioDeleteRequest(int32_t id, int32_t* ret) {
 (void)id;
 (void)ret;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelAioInitializeImpl(void* param, int32_t size) {
 (void)param;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceKernelAioInitializeParam(void* param) {
 (void)param;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceKernelAioSubmitReadCommands(KernelAioRwRequest* req, int32_t size, int32_t prio, int32_t* id) {
 (void)req;
 (void)size;
 (void)prio;
 (void)id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelAioSubmitWriteCommands(KernelAioRwRequest* req, int32_t size, int32_t prio, int32_t* id) {
 (void)req;
 (void)size;
 (void)prio;
 (void)id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelAioWaitRequest(int32_t id, int32_t* state, uint32_t* usec) {
 (void)id;
 (void)state;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
}
