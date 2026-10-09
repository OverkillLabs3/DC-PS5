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

int APS5_VABI sceMouseClose(int32_t handle) {
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMouseInit(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMouseOpen(int user_id, int32_t type, int32_t index, const void* param) {
 (void)user_id;
 (void)type;
 (void)index;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceMouseRead(int32_t handle, MouseData* data, int32_t num) {
 (void)handle;
 (void)data;
 (void)num;
 return SoftFailNp(__func__);
 return 0;
}
}
