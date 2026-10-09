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

int32_t APS5_VABI scePngDecCreate(const PngDecCreateParam* param, void* memory_address, uint32_t memory_size, void** handle) {
 (void)param;
 (void)memory_address;
 (void)memory_size;
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI scePngDecDecode(void* handle, const PngDecDecodeParam* param, PngDecImageInfo* image_info) {
 (void)handle;
 (void)param;
 (void)image_info;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI scePngDecDelete(void* handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI scePngDecParseHeader(const PngDecParseParam* param, PngDecImageInfo* image_info) {
 (void)param;
 (void)image_info;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI scePngDecQueryMemorySize(const PngDecCreateParam* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
}
