#include "prx/libSceAgc/DcbFlow/include/Dispatch.hpp"

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

uint32_t* APS5_VABI sceAgcDcbDispatchIndirect(CommandBuffer* buf, uint32_t data_offset_in_bytes, uint32_t flags) {
    Agc::Command::Require((data_offset_in_bytes & 3u) == 0, __func__, "misaligned indirect argument offset");
    // DISPATCH_INDIRECT: data_offset, dispatch_initiator (3 dwords, see GetSize); 0x41 = compute enable like DISPATCH_DIRECT.
    return Agc::Command::Emit(buf, 0x16u, {data_offset_in_bytes, flags | 0x41u}, __func__);
}
std::uint32_t APS5_VABI sceAgcDcbDispatchIndirectGetSize() {
    return 12;
}

}
