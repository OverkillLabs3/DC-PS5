#include <cstdint>
#include <cstdio>
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

int32_t APS5_VABI sceJpegDecCreate(const void* param, void* memory_address, uint32_t memory_size, void** handle) {
 (void)param;
 (void)memory_address;
 (void)memory_size;
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceJpegDecDecode(void* handle, const void* param, void* image_info) {
 (void)handle;
 (void)param;
 (void)image_info;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceJpegDecDelete(void* handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceJpegDecQueryMemorySize(const void* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
}
