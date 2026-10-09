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

int APS5_VABI RemotePlayGetConnectionStatus(int user_id, int* status) {
 (void)user_id;
 (void)status;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI ShareGetCurrentStatus(uint32_t feature_flag, ShareCurrentStatus* status) {
 (void)feature_flag;
 (void)status;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI ShareInitialize(size_t heap_size, int thread_priority, uint64_t affinity_mask) {
 (void)heap_size;
 (void)thread_priority;
 (void)affinity_mask;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI ShareTerminate(void) {
 return SoftFailNp(__func__);
 return 0;
}
}
