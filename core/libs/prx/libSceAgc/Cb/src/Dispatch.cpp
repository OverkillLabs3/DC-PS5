#include "prx/libSceAgc/Cb/include/Dispatch.hpp"

#include "prx/libSceAgc/Command/include/Packet.hpp"
#include <cstdint>
#include <cstddef>
#include <cstdio>
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

std::uint32_t* APS5_VABI sceAgcCbDispatch(CommandBuffer* buf, std::uint32_t threadGroupX, std::uint32_t threadGroupY, std::uint32_t threadGroupZ, std::uint32_t modifier) {
    // PS4-era mask 0xa038 rejects PS5 CsMode bits: Dead Cells passes modifier=0xe
    // (bits 1|2|3 = thread/group plus two PS5-only flags). Throwing here reaches
    // std::terminate and kills the game, so warn on unknown bits instead.
    static bool warned = false;
    if (const auto unknown = modifier & ~0xa038u; unknown && !warned) {
        warned = true;
        std::fprintf(stderr, "[AGC] sceAgcCbDispatch: modifier 0x%x has bits outside mask 0xa038 (unknown 0x%x)\n",
            modifier, unknown);
        std::fflush(stderr);
    }
    return Agc::Command::Emit(buf, 0x15u, {threadGroupX, threadGroupY, threadGroupZ, modifier | 0x41u}, __func__);
}

uint32_t APS5_VABI sceAgcCbDispatchGetSize(void) {
    return 20; // DISPATCH_DIRECT: header + 4 dwords
}
}
