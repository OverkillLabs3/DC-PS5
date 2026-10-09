#include "prx/libSceAgc/Command/include/Draw.hpp"
#include "prx/libSceAgc/DcbDraw/include/DrawIndexed.hpp"

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

uint32_t* APS5_VABI sceAgcDcbDrawIndex(CommandBuffer* buf, uint32_t index_count, const volatile void* index_addr, uint64_t modifier) {
    const auto address = reinterpret_cast<std::uintptr_t>(index_addr);
    Agc::Command::CheckAddress(address, 1, __func__);
    // DRAW_INDEX_2: max_size, index_base_lo, index_base_hi, index_count, draw_initiator (6 dwords, see GetSize).
    return Agc::Command::Emit(buf, 0x27u, {index_count == 0 ? 1u : index_count, static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(static_cast<std::uint64_t>(address) >> 32u), index_count, Agc::Command::DrawInitiator(modifier, true, __func__)}, __func__);
}
std::uint32_t APS5_VABI sceAgcDcbDrawIndexGetSize() {
    return 24;
}

std::uint32_t* APS5_VABI sceAgcDcbDrawIndexAuto(CommandBuffer* buf, std::uint32_t indexCount, std::uint64_t modifier) {
    return Agc::Command::Emit(buf, 0x2du, {indexCount, Agc::Command::DrawInitiator(modifier, false, __func__)}, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbDrawIndexAutoGetSize() {
    return 12;
}

std::uint32_t* APS5_VABI sceAgcDcbDrawIndexOffset(CommandBuffer* buf, std::uint32_t indexOffset, std::uint32_t indexCount, std::uint64_t modifier) {
    return Agc::Command::Emit(buf, 0x35u, {indexCount == 0 ? 1u : indexCount, indexOffset, indexCount, Agc::Command::DrawInitiator(modifier, true, __func__)}, __func__);
}

uint32_t APS5_VABI sceAgcDcbDrawIndexOffsetGetSize(void) {
    return 20; // DRAW_INDEX_OFFSET_2: header + 4 dwords
}
uint32_t* APS5_VABI sceAgcDcbDrawIndexIndirect(CommandBuffer* buf, uint32_t data_offset_in_bytes, uint64_t modifier) {
    Agc::Command::Require((data_offset_in_bytes & 3u) == 0, __func__, "misaligned indirect argument offset");
    const auto offsets = Agc::Command::DrawPatchOffsets(modifier, __func__);
    // DRAW_INDEX_INDIRECT: data_offset, base_vertex_loc, start_instance_loc, draw_initiator (5 dwords, see GetSize).
    return Agc::Command::Emit(buf, 0x25u, {data_offset_in_bytes, static_cast<std::uint32_t>(offsets), static_cast<std::uint32_t>(offsets >> 32u), Agc::Command::DrawInitiator(modifier, true, __func__)}, __func__);
}
std::uint32_t APS5_VABI sceAgcDcbDrawIndexIndirectGetSize() {
    return 20;
}

uint32_t* APS5_VABI sceAgcDcbDrawIndexIndirectMulti(CommandBuffer* buf, uint32_t data_offset_in_bytes, uint32_t count_indirect, uint32_t max_count_or_count, const volatile void* count_addr, uint32_t stride_in_bytes, uint64_t modifier) {
    (void)buf; (void)data_offset_in_bytes; (void)count_indirect; (void)max_count_or_count; (void)count_addr; (void)stride_in_bytes; (void)modifier;
    Agc::Command::ReportUnimplemented(__func__);
    return nullptr;
}
std::uint32_t APS5_VABI sceAgcDcbDrawIndexIndirectMultiGetSize() {
    return 40;
}

uint32_t* APS5_VABI sceAgcDcbDrawIndexMultiInstanced(CommandBuffer* buf, uint32_t index_count, const volatile void* index_addr, const volatile void* object_ids, uint32_t instance_count, uint64_t modifier) {
    (void)buf; (void)index_count; (void)index_addr; (void)object_ids; (void)instance_count; (void)modifier;
    Agc::Command::ReportUnimplemented(__func__);
    return nullptr;
}
uint32_t APS5_VABI sceAgcDcbDrawIndexMultiInstancedGetSize(void) {
    Agc::Command::ReportUnimplemented(__func__);
    return 0;
}
}
