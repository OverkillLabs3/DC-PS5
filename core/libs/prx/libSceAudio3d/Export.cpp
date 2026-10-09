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

void APS5_VABI sceAudio3dGetDefaultOpenParameters(Audio3dOpenParameters* p) {
 (void)p;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceAudio3dInitialize(int64_t reserved) {
 (void)reserved;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudio3dPortAdvance(uint32_t port_id) {
 (void)port_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudio3dPortGetQueueLevel(uint32_t port_id, uint32_t* queue_level, uint32_t* queue_available) {
 (void)port_id;
 (void)queue_level;
 (void)queue_available;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudio3dPortOpen(int user_id, const Audio3dOpenParameters* parameters, uint32_t* id) {
 (void)user_id;
 (void)parameters;
 (void)id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudio3dPortPush(uint32_t port_id, uint32_t blocking) {
 (void)port_id;
 (void)blocking;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAudio3dPortSetAttribute(uint32_t port_id, uint32_t attribute_id, const void* attribute, size_t attribute_size) {
 (void)port_id;
 (void)attribute_id;
 (void)attribute;
 (void)attribute_size;
 return SoftFailNp(__func__);
 return 0;
}
}
