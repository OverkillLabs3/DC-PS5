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

intptr_t APS5_VABI sceKernelGetEventData(const KernelEvent* ev) {
 (void)ev;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelGetEventError(const KernelEvent* ev) {
 (void)ev;
 return SoftFailNp(__func__);
 return 0;
}
intptr_t APS5_VABI sceKernelGetEventFflags(const KernelEvent* ev) {
 (void)ev;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelGetEventFilter(const KernelEvent* ev) {
 (void)ev;
 return SoftFailNp(__func__);
 return 0;
}
uintptr_t APS5_VABI sceKernelGetEventId(const KernelEvent* ev) {
 (void)ev;
 return SoftFailNp(__func__);
 return 0;
}
void* APS5_VABI sceKernelGetEventUserData(const KernelEvent* ev) {
 (void)ev;
 SoftFailNp(__func__);

 return nullptr;
}
}
