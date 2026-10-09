#include "prx/libSceAgcDriver/State/include/Status.hpp"

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented GPU command emitter - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

bool APS5_VABI sceAgcDriverIsCaptureInProgress(void) {
 return SoftFailNp(__func__);
 return false;
}
bool APS5_VABI sceAgcDriverIsTraceInProgress(void) {
 return SoftFailNp(__func__);
 return false;
}
bool APS5_VABI sceAgcDriverIsSubmitValidationEnabled(void) {
 return SoftFailNp(__func__);
 return false;
}
int APS5_VABI sceAgcDriverRequestCaptureStart(const char* path) {
    (void)path;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAgcDriverRequestCaptureStop() {
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAgcDriverTriggerCapture() {
    return SoftFailNp(__func__);
    return 0;
}
}
