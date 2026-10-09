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

int APS5_VABI sceMsgDialogClose(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogGetResult(void* result) {
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogGetStatus(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogInitialize(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogOpen(const void* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogProgressBarInc(int target, uint32_t delta) {
 (void)target;
 (void)delta;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogProgressBarSetMsg(int target, const char* msg) {
 (void)target;
 (void)msg;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogProgressBarSetValue(int target, uint32_t rate) {
 (void)target;
 (void)rate;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogTerminate(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMsgDialogUpdateStatus(void) {
 return SoftFailNp(__func__);
 return 0;
}
}
