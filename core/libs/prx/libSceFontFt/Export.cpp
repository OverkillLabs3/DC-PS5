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

void* APS5_VABI sceFontSelectLibraryFt(int value) {
 (void)value;
 SoftFailNp(__func__);

 return nullptr;
}
void* APS5_VABI sceFontSelectRendererFt(int value) {
 (void)value;
 SoftFailNp(__func__);

 return nullptr;
}
}
