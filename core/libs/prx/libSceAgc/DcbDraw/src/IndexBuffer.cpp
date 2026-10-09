#include "prx/libSceAgc/DcbDraw/include/IndexBuffer.hpp"

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

std::uint32_t* APS5_VABI sceAgcDcbSetIndexBuffer(CommandBuffer* buf, std::uint64_t indexAddress) {
    Agc::Command::CheckAddress(indexAddress, 2, __func__);
    return Agc::Command::Emit(buf, 0x26u, {static_cast<std::uint32_t>(indexAddress), static_cast<std::uint32_t>(indexAddress >> 32u)}, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbSetIndexBufferGetSize() {
    return 12; // INDEX_BASE: header + 2 dwords
}
uint32_t* APS5_VABI sceAgcDcbSetIndexCount(CommandBuffer* buf, uint32_t index_count) {
    return Agc::Command::Emit(buf, 0x13u, {index_count}, __func__); // INDEX_BUFFER_SIZE
}
std::uint32_t APS5_VABI sceAgcDcbSetIndexCountGetSize() {
    return 8; // header + 1 dword
}
std::uint32_t* APS5_VABI sceAgcDcbSetIndexSize(CommandBuffer* buf, std::uint8_t indexSize, std::uint8_t cachePolicy) {
    Agc::Command::Require(indexSize <= 2, __func__, "invalid index element size");
    Agc::Command::CheckBits(cachePolicy, 3, __func__);
    return Agc::Command::Emit(buf, 0x7au, {0x20000243u, 0x400u | indexSize | (static_cast<std::uint32_t>(cachePolicy) << 6u)}, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbSetIndexSizeGetSize() {
    return 12; // SET_UCONFIG_REG_INDEX: header + 2 dwords
}
std::uint32_t* APS5_VABI sceAgcDcbSetIndexIndirectArgs(CommandBuffer* buf, std::uint32_t dataOffsetInBytes) {
    (void)buf; (void)dataOffsetInBytes;
    Agc::Command::ReportUnimplemented(__func__);
    return nullptr;
}
std::uint32_t APS5_VABI sceAgcDcbSetIndexIndirectArgsGetSize() {
    Agc::Command::ReportUnimplemented(__func__);
    return 0;
}
}
