#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"


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

void APS5_VABI sceKernelRtldSetApplicationHeapAPI(void* api[]) {
    ApplicationHeapRegister_nid_no_patch(api);
}

int APS5_VABI sceKernelRtldThreadAtexitDecrement(uint64_t* c) {
 (void)c;
 SoftFailNp(__func__);

 return 0;
}
int APS5_VABI sceKernelRtldThreadAtexitIncrement(uint64_t* c) {
 (void)c;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceKernelSetThreadAtexitCount(get_thread_atexit_count_func_t func) {
 (void)func;
 SoftFailNp(__func__);
 return;
}
void APS5_VABI sceKernelSetThreadAtexitReport(thread_atexit_report_func_t func) {
 (void)func;
 SoftFailNp(__func__);
 return;
}
void APS5_VABI sceKernelSetThreadDtors(thread_dtors_func_t dtors) {
 (void)dtors;
 SoftFailNp(__func__);
 return;
}
}
