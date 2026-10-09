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

int32_t APS5_VABI sceFiberFinalize(FiberObject* fiber) {
 (void)fiber;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberGetInfo(FiberObject* fiber, FiberInfo* fiber_info) {
 (void)fiber;
 (void)fiber_info;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberGetSelf(FiberObject** fiber) {
 (void)fiber;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberGetThreadFramePointerAddress(uint64_t* addr_frame_pointer) {
 (void)addr_frame_pointer;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberOptParamInitialize(FiberOptParam* opt_param) {
 (void)opt_param;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberRename(FiberObject* fiber, const char* name) {
 (void)fiber;
 (void)name;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberReturnToThread(uint64_t arg_on_return, uint64_t* arg_on_run) {
 (void)arg_on_return;
 (void)arg_on_run;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberRun_nid_postfix(FiberObject* fiber, uint64_t arg_on_run, uint64_t* arg_on_return) {
 (void)fiber;
 (void)arg_on_run;
 (void)arg_on_return;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberStartContextSizeCheck(uint32_t flags) {
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberStopContextSizeCheck(void) {
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceFiberSwitch(FiberObject* fiber, uint64_t arg_on_run, uint64_t* arg_on_return) {
 (void)fiber;
 (void)arg_on_run;
 (void)arg_on_return;
 return SoftFailNp(__func__);
 return 0;
}
}
