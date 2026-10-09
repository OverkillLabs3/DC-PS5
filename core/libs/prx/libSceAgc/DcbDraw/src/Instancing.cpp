#include "prx/libSceAgc/DcbDraw/include/Instancing.hpp"

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

std::uint32_t* APS5_VABI sceAgcDcbSetNumInstances(CommandBuffer* buf, std::uint32_t numInstances) {
    return Agc::Command::Emit(buf, 0x2fu, {numInstances}, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbSetNumInstancesGetSize() {
    return 8;
}

uint32_t* APS5_VABI sceAgcDcbSetBaseIndirectArgs(CommandBuffer* buf, uint32_t shader_type, const volatile void* indirect_base_addr) {
    const auto address = reinterpret_cast<std::uintptr_t>(indirect_base_addr);
    Agc::Command::Require(shader_type <= 1, __func__, "invalid shader type");
    Agc::Command::CheckAddress(address, 8, __func__);
    // SET_BASE: base_index 1 (draw-indirect arguments), address_lo, address_hi; header flag bit 1 selects the compute queue.
    auto* packet = Agc::Command::Allocate(buf, 4, __func__);
    packet[0] = Agc::Command::Header(0x11u, 4, shader_type != 0 ? 2u : 0u);
    packet[1] = 1u;
    packet[2] = static_cast<std::uint32_t>(address);
    packet[3] = static_cast<std::uint32_t>(static_cast<std::uint64_t>(address) >> 32u);
    return packet;
}
}
