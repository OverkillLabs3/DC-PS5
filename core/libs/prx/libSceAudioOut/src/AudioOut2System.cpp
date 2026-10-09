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

int APS5_VABI sceAudioOut2Initialize(void) {
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2GetSystemState(AudioOut2SystemState* state) {
    (void)state;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2SetSystemDebugState(const AudioOut2SystemDebugStateParam* param) {
    (void)param;
    return SoftFailNp(__func__);
    return 0;
}
}
