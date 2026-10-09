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

int32_t APS5_VABI sceAudiodecClearContext(int32_t handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudiodecCreateDecoder(AudiodecCtrl* ctrl, uint32_t codec_type) {
 (void)ctrl;
 (void)codec_type;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudiodecDecode(int32_t handle, AudiodecCtrl* ctrl) {
 (void)handle;
 (void)ctrl;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudiodecDeleteDecoder(int32_t handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudiodecInitLibrary(uint32_t codec_type) {
 (void)codec_type;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudiodecTermLibrary(uint32_t codec_type) {
 (void)codec_type;
 return SoftFailNp(__func__);
 return 0;
}
}
