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

int APS5_VABI sceKeyboardClose(int32_t handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKeyboardGetKey2Char(int32_t handle, int32_t arrange, uint32_t led, uint32_t modifier_key, uint16_t key_code, KeyboardCharData* char_data) {
 (void)handle;
 (void)arrange;
 (void)led;
 (void)modifier_key;
 (void)key_code;
 (void)char_data;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKeyboardInit(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKeyboardOpen(int user_id, int32_t type, int32_t index, const void* param) {
 (void)user_id;
 (void)type;
 (void)index;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKeyboardRead(int32_t handle, KeyboardData* data, int32_t num) {
 (void)handle;
 (void)data;
 (void)num;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKeyboardReadState(int32_t handle, KeyboardData* data) {
 (void)handle;
 (void)data;
 return SoftFailNp(__func__);
 return 0;
}
}
