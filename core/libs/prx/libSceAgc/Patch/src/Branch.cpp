#include "prx/libSceAgc/Patch/include/Branch.hpp"

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

int APS5_VABI sceAgcBranchPatchSetCompareAddress(std::uint32_t* cmd, const volatile std::uint64_t* compareAddress) {
    (void)cmd;
    (void)compareAddress;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAgcBranchPatchSetThenTarget(std::uint32_t* cmd, const volatile std::uint32_t* target, std::uint32_t sizeInDwords) {
    (void)cmd;
    (void)target;
    (void)sizeInDwords;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAgcBranchPatchSetElseTarget(std::uint32_t* cmd, const volatile std::uint32_t* target, std::uint32_t sizeInDwords) {
    (void)cmd;
    (void)target;
    (void)sizeInDwords;
    return SoftFailNp(__func__);
    return 0;
}
}
