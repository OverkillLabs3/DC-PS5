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

int APS5_VABI sceAudioOut2MasteringInit(uint32_t flags) {
    (void)flags;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2MasteringTerm(void) {
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2MasteringGetState(AudioOut2MasteringStatesHeader* state, uint32_t output, AudioOut2UserHandle user) {
    (void)state;
    (void)output;
    (void)user;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2MasteringSetParam(const AudioOut2MasteringParamsHeader* param, uint32_t output, uint32_t flags) {
    (void)param;
    (void)output;
    (void)flags;
    return SoftFailNp(__func__);
    return 0;
}
}
