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

int APS5_VABI sceAcmBatchStartBuffer(AcmContextId context, const void* batch_commands, size_t batch_size, AcmBatchError* batch_error, AcmBatchId* batch) {
 (void)context;
 (void)batch_commands;
 (void)batch_size;
 (void)batch_error;
 (void)batch;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAcmBatchStartBuffers(AcmContextId context, uint32_t batch_info_count, const AcmBatchInfo* const batch_info[], AcmBatchError* batch_error, AcmBatchId* batch) {
 (void)context;
 (void)batch_info_count;
 (void)batch_info;
 (void)batch_error;
 (void)batch;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAcmBatchWait(AcmContextId context, AcmBatchId batch, uint32_t timeout) {
 (void)context;
 (void)batch;
 (void)timeout;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAcmContextCreate(AcmContextId* context) {
 (void)context;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAcmContextDestroy(AcmContextId context) {
 (void)context;
 return SoftFailNp(__func__);
 return 0;
}
}
