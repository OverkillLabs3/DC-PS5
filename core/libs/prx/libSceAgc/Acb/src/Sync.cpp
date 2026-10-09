#include "prx/libSceAgc/Acb/include/Sync.hpp"

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

uint32_t* APS5_VABI sceAgcAcbCondExec(CommandBuffer* buf, const volatile uint32_t* address, uint32_t num_dwords) {
 (void)buf;
 (void)address;
 (void)num_dwords;
 return nullptr;  // SoftFailNp: pointer-returning
}
uint32_t APS5_VABI sceAgcAcbCondExecGetSize(void) {
 return SoftFailNp(__func__);
 return 0;
}
std::uint32_t* APS5_VABI sceAgcAcbWaitRegMem(CommandBuffer* buf, std::uint8_t size, std::uint8_t compareFunction, std::uint8_t cachePolicy, const volatile void* address, std::uint64_t reference, std::uint64_t mask, std::uint32_t pollCycles) {
    return Agc::Command::WriteWait(buf, size, compareFunction, 0, cachePolicy, address, reference, mask, pollCycles, __func__);
}

std::uint64_t APS5_VABI sceAgcAcbWaitOnAddressGetSize(std::uint8_t size) {
    (void)size;
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t* APS5_VABI sceAgcAcbEventWrite(CommandBuffer* buf, std::uint8_t eventType, const volatile void* address) {
    Agc::Command::CheckBits(eventType, 0x3fu, __func__);
    Agc::Command::Require(address == nullptr, __func__, "ACB event address is not supported");
    return Agc::Command::Emit(buf, 0x46u, {eventType | (eventType == 7 ? 0x400u : 0u)}, __func__);
}

std::uint64_t APS5_VABI sceAgcAcbEventWriteGetSize(std::uint8_t eventType) {
    (void)eventType;
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t APS5_VABI sceAgcAcbQueueEndOfShaderActionGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t* APS5_VABI sceAgcAcbMemSemaphore(CommandBuffer* buf, std::uint8_t action, std::uint8_t clientCode, const volatile void* address) {
    (void)buf;
    (void)action;
    (void)clientCode;
    (void)address;
    SoftFailNp(__func__);
 return nullptr;
    return nullptr;
}
}
