#include <cstddef>
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

int APS5_VABI sceAudioOut2SpeakerArrayCreate(AudioOut2SpeakerArrayHandle* handle, const void* vbap_params, const void* ambi_params) {
    (void)handle;
    (void)vbap_params;
    (void)ambi_params;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2SpeakerArrayDestroy(AudioOut2SpeakerArrayHandle handle) {
    (void)handle;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(AudioOut2SpeakerArrayHandle handle, uint32_t ambisonics_channel, float* coefficients, uint32_t num_coefficients) {
    (void)handle;
    (void)ambisonics_channel;
    (void)coefficients;
    (void)num_coefficients;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2GetSpeakerArrayCoefficients(AudioOut2SpeakerArrayHandle handle, AudioOut2Position pos, float spread, float* coefficients, uint32_t num_coefficients, uint8_t height_aware, float downmix_spread_radius) {
    (void)handle;
    (void)pos;
    (void)spread;
    (void)coefficients;
    (void)num_coefficients;
    (void)height_aware;
    (void)downmix_spread_radius;
    return SoftFailNp(__func__);
    return 0;
}
size_t APS5_VABI sceAudioOut2GetSpeakerArrayMemorySize(uint32_t num_speakers, uint8_t is_3d, uint8_t is_ambisonics) {
    (void)num_speakers;
    (void)is_3d;
    (void)is_ambisonics;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2GetSpeakerInfo(AudioOut2SpeakerInfo* info, uint32_t flags) {
    (void)info;
    (void)flags;
    return SoftFailNp(__func__);
    return 0;
}
}
