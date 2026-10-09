#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/DirectMemory/DirectMemory.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


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

int APS5_VABI getargc_nid_postfix(void) {
 return SoftFailNp(__func__);
 return 0;
}
const char** APS5_VABI getargv_nid_postfix(void) {
 return nullptr;  // SoftFailNp: pointer-returning
}
int APS5_VABI getpagesize_nid_postfix(void) {
 return PS5_PAGE_SIZE;
}
int APS5_VABI getpid_nid_postfix(void) {
#ifdef _WIN32
 return static_cast<int>(::GetCurrentProcessId());
#else
 return static_cast<int>(::getpid());
#endif
}

void APS5_VABI exit_nid_postfix(int code) {
 (void)code;
 SoftFailNp(__func__);
}
int APS5_VABI sceKernelGetCurrentCpu(void) {
 return SoftFailNp(__func__);
 return 0;
}
uint64_t APS5_VABI sceKernelGetGPI(void) {
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceKernelSetGPO(uint32_t bits) {
 (void)bits;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceKernelGetOpenPsId(void* open_ps_id) {
 (void)open_ps_id;
 return SoftFailNp(__func__);
 return 0;
}
void* APS5_VABI sceKernelGetProcParam(void) {
 SoftFailNp(__func__);

 return nullptr;
}
int APS5_VABI sceKernelUuidCreate(uint32_t* uuid) {
 (void)uuid;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceKernelSync(void) {
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sched_get_priority_max_nid_postfix(int policy) {
 (void)policy;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sched_get_priority_min_nid_postfix(int policy) {
 (void)policy;
 return SoftFailNp(__func__);
 return 0;
}
}
