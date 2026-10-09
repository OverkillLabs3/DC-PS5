#include "prx/libSceAgc/Patch/include/IndirectRegisters.hpp"

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

int APS5_VABI sceAgcSetCxRegIndirectPatchAddRegisters(std::uint32_t* cmd, std::uint32_t numRegs) {
    Agc::Command::PatchIndirectCount(cmd, 0x9fu, numRegs, __func__);
    return 0;
}

int APS5_VABI sceAgcSetCxRegIndirectPatchSetAddress(std::uint32_t* cmd, const volatile ShaderRegister* regs) {
    Agc::Command::PatchIndirectAddress(cmd, 0x9fu, regs, __func__);
    return 0;
}

int APS5_VABI sceAgcSetCxRegIndirectPatchSetNumRegisters(uint32_t* cmd, uint32_t num_regs) {
 (void)cmd;
 (void)num_regs;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAgcSetShRegIndirectPatchAddRegisters(std::uint32_t* cmd, std::uint32_t numRegs) {
    Agc::Command::PatchIndirectCount(cmd, 0x63u, numRegs, __func__);
    return 0;
}

int APS5_VABI sceAgcSetShRegIndirectPatchSetAddress(std::uint32_t* cmd, const volatile ShaderRegister* regs) {
    Agc::Command::PatchIndirectAddress(cmd, 0x63u, regs, __func__);
    return 0;
}

int APS5_VABI sceAgcSetShRegIndirectPatchSetNumRegisters(uint32_t* cmd, uint32_t num_regs) {
 (void)cmd;
 (void)num_regs;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAgcSetUcRegIndirectPatchAddRegisters(std::uint32_t* cmd, std::uint32_t numRegs) {
    Agc::Command::PatchIndirectCount(cmd, 0x64u, numRegs, __func__);
    return 0;
}

int APS5_VABI sceAgcSetUcRegIndirectPatchSetAddress(std::uint32_t* cmd, const volatile ShaderRegister* regs) {
    Agc::Command::PatchIndirectAddress(cmd, 0x64u, regs, __func__);
    return 0;
}

int APS5_VABI sceAgcSetUcRegIndirectPatchSetNumRegisters(uint32_t* cmd, uint32_t num_regs) {
 (void)cmd;
 (void)num_regs;
 return SoftFailNp(__func__);
 return 0;
}
}
