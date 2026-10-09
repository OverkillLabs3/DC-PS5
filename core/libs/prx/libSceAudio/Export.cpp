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

using SceAudioOutPortId = std::int32_t;

std::int32_t APS5_VABI sceAudioOutInit(void) {
 return SoftFailNp(__func__);
 return 0;
}
SceAudioOutPortId APS5_VABI sceAudioOutOpen(std::int32_t userId, std::int32_t type, std::int32_t index, std::uint32_t len, std::uint32_t freq, std::uint32_t param) {
    (void)userId; (void)type; (void)index; (void)len; (void)freq; (void)param;
    return SoftFailNp(__func__);
    return 0;
}
std::int32_t APS5_VABI sceAudioOutOutput(SceAudioOutPortId handle, const void* ptr) {
    (void)handle; (void)ptr;
    return SoftFailNp(__func__);
    return 0;
}
std::int32_t APS5_VABI sceAudioOutSetVolume(SceAudioOutPortId handle, std::int32_t flag, std::int32_t* vol) {
    (void)handle; (void)flag; (void)vol;
    return SoftFailNp(__func__);
    return 0;
}
std::int32_t APS5_VABI sceAudioOutClose(SceAudioOutPortId handle) {
    (void)handle;
    return SoftFailNp(__func__);
    return 0;
}
}
