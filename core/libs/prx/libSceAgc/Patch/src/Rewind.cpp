#include "prx/libSceAgc/Patch/include/Rewind.hpp"

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

int APS5_VABI sceAgcRewindPatchSetRewindState(uint32_t* cmd, uint8_t state) {
 (void)cmd;
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAgcAsyncRewindPatchSetRewindState(std::uint32_t* cmd, std::uint8_t state) {
    (void)cmd;
    (void)state;
    return SoftFailNp(__func__);
    return 0;
}
}
