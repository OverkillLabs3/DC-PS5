#include "prx/libSceAgc/Acb/include/Dispatch.hpp"

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

uint32_t* APS5_VABI sceAgcAcbDispatchIndirect(CommandBuffer* buf, const volatile void* indirect_args, uint32_t modifier) {
 (void)buf;
 (void)indirect_args;
 (void)modifier;
 return nullptr;  // SoftFailNp: pointer-returning
}
std::uint32_t APS5_VABI sceAgcAcbDispatchIndirectGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
}
