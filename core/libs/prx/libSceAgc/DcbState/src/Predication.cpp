#include "prx/libSceAgc/DcbState/include/Predication.hpp"

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

uint32_t* APS5_VABI sceAgcDcbSetPredication(CommandBuffer* buf, uint8_t condition, uint8_t op, uint8_t wait_op, const volatile void* address, uint32_t count_in_dwords) {
    (void)buf; (void)condition; (void)op; (void)wait_op; (void)address; (void)count_in_dwords;
    Agc::Command::ReportUnimplemented(__func__);
    return nullptr;
}
std::uint32_t APS5_VABI sceAgcDcbSetZPassPredicationEnableGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t APS5_VABI sceAgcDcbSetPredicationDisableGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t APS5_VABI sceAgcDcbSetBoolPredicationEnableGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
}
