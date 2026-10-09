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

int APS5_VABI sceKernelInstallExceptionHandler(int signum, void* handler) {
 (void)signum;
 (void)handler;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelRemoveExceptionHandler(int signum) {
 (void)signum;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelRaiseException(Pthread thread, int signum) {
 (void)thread;
 (void)signum;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceKernelDebugRaiseException(int c1, int c2) {
 (void)c1;
 (void)c2;
 SoftFailNp(__func__);
 return;
}
void APS5_VABI sceKernelDebugRaiseExceptionOnReleaseMode(int c1, int c2) {
 (void)c1;
 (void)c2;
 SoftFailNp(__func__);
 return;
}
}
