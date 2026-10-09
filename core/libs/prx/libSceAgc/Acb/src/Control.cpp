#include "prx/libSceAgc/Acb/include/Control.hpp"

#include "prx/libSceAgc/Command/include/Memory.hpp"
#include "prx/libSceAgc/Command/include/Packet.hpp"
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented GPU command emitter - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

// unknown signature
APS5_EXPORT("gQkqkLttcpw", sceAgcAcb_gQkqkLttcpw);
void* APS5_VABI sceAgcAcb_gQkqkLttcpw (void) {
    SoftFailNp(__func__);
 return nullptr;
    return nullptr;
}
std::uint32_t* APS5_VABI sceAgcAcbJump(CommandBuffer* buf, std::uint8_t cachePolicy, const std::uint32_t* target, std::uint32_t sizeInDwords) {
    (void)buf;
    (void)cachePolicy;
    (void)target;
    (void)sizeInDwords;
    SoftFailNp(__func__);
 return nullptr;
    return 0;
}
uint32_t APS5_VABI sceAgcAcbJumpGetSize(void) {
 return SoftFailNp(__func__);
 return 0;
}
uint32_t* APS5_VABI sceAgcAcbResetQueue(CommandBuffer* buf, uint32_t op) {
 (void)buf;
 (void)op;
 return nullptr;  // SoftFailNp: pointer-returning
}
std::uint32_t* APS5_VABI sceAgcAcbRewind(CommandBuffer* buf, std::uint32_t initialState) {
    (void)buf;
    (void)initialState;
    SoftFailNp(__func__);
 return nullptr;
    return nullptr;
}
std::uint32_t APS5_VABI sceAgcAcbRewindGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t* APS5_VABI sceAgcAcbWaitUntilSafeForRendering(CommandBuffer* buf, std::uint32_t videoOutHandle, std::uint32_t displayBufferIndex) {
    (void)buf;
    (void)videoOutHandle;
    (void)displayBufferIndex;
    SoftFailNp(__func__);
 return nullptr;
    return nullptr;
}
std::uint32_t* APS5_VABI sceAgcAcbSetFlip(CommandBuffer* buf, std::uint32_t videoOutHandle, std::int32_t displayBufferIndex, std::uint32_t flipMode, std::int64_t flipArg) {
    (void)buf;
    (void)videoOutHandle;
    (void)displayBufferIndex;
    (void)flipMode;
    (void)flipArg;
    SoftFailNp(__func__);
 return nullptr;
    return nullptr;
}
uint32_t* APS5_VABI sceAgcAcbPushMarker(CommandBuffer* buf, const char* str, uint32_t color) {
 (void)buf;
 (void)str;
 (void)color;
 return nullptr;  // SoftFailNp: pointer-returning
}
uint32_t* APS5_VABI sceAgcAcbPopMarker(CommandBuffer* buf) {
 (void)buf;
 return nullptr;  // SoftFailNp: pointer-returning
}
uint32_t* APS5_VABI sceAgcAcbSetMarker(CommandBuffer* buf, const char* str, uint32_t color) {
 (void)buf;
 (void)str;
 (void)color;
 return nullptr;  // SoftFailNp: pointer-returning
}
}
