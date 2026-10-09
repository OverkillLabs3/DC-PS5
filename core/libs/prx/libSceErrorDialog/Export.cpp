#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0x800242e2 (online service not available)\n", func);
    std::fflush(stderr);
    return static_cast<int>(0x800242e2u);
}
}  // namespace

extern "C" {

int APS5_VABI sceErrorDialogClose(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceErrorDialogGetStatus(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceErrorDialogInitialize(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceErrorDialogOpen(const void* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceErrorDialogTerminate(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceErrorDialogUpdateStatus(void) {
 return SoftFailNp(__func__);
 return 0;
}
}
