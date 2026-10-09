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

int APS5_VABI sceAudioInGetSilentState(int handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudioInInput(int handle, void* dest) {
 (void)handle;
 (void)dest;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudioInOpen(int user_id, uint32_t type, uint32_t index, uint32_t len, uint32_t freq, uint32_t param) {
 (void)user_id;
 (void)type;
 (void)index;
 (void)len;
 (void)freq;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
}
