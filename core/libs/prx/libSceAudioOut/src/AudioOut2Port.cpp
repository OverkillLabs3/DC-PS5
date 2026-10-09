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

int APS5_VABI sceAudioOut2PortCreate(AudioOut2ContextHandle ctx, const AudioOut2PortParam* params, AudioOut2PortHandle* port) {
    (void)ctx;
    (void)params;
    (void)port;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2PortDestroy(AudioOut2PortHandle port) {
    (void)port;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2PortGetState(AudioOut2PortHandle port, AudioOut2PortState* state) {
    (void)port;
    (void)state;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2PortSetAttributes(AudioOut2PortHandle port, const AudioOut2Attribute* attributes, uint32_t num) {
    (void)port;
    (void)attributes;
    (void)num;
    return SoftFailNp(__func__);
    return 0;
}
}
