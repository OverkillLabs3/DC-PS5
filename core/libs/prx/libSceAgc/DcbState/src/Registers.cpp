#include "prx/libSceAgc/DcbState/include/Registers.hpp"

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

uint32_t* APS5_VABI sceAgcDcbSetCxRegisterDirect(CommandBuffer* buf, ShaderRegister reg) {
 (void)buf;
 (void)reg;
 return nullptr;  // SoftFailNp: pointer-returning
}
uint32_t APS5_VABI sceAgcDcbSetCxRegisterDirectGetSize(void) {
 return SoftFailNp(__func__);
 return 0;
}
std::uint32_t* APS5_VABI sceAgcDcbSetCxRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, std::uint32_t numRegs) {
    return Agc::Command::WriteIndirectRegisters(buf, 0x9fu, regs, numRegs, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbSetCxRegistersIndirectGetSize(std::uint32_t numRegs) {
    (void)numRegs;
    return SoftFailNp(__func__);
    return 0;
}
uint32_t* APS5_VABI sceAgcDcbSetShRegisterDirect(CommandBuffer* buf, ShaderRegister reg) {
 (void)buf;
 (void)reg;
 return nullptr;  // SoftFailNp: pointer-returning
}
std::uint32_t APS5_VABI sceAgcDcbSetShRegisterDirectGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t* APS5_VABI sceAgcDcbSetShRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, std::uint32_t numRegs) {
    return Agc::Command::WriteIndirectRegisters(buf, 0x63u, regs, numRegs, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbSetShRegistersIndirectGetSize(std::uint32_t numRegs) {
    Agc::Command::CheckBits(numRegs, 0x3fffu, __func__);
    return 20;
}

uint32_t* APS5_VABI sceAgcDcbSetUcRegisterDirect(CommandBuffer* buf, ShaderRegister reg) {
 (void)buf;
 (void)reg;
 return nullptr;  // SoftFailNp: pointer-returning
}
std::uint32_t APS5_VABI sceAgcDcbSetUcRegisterDirectGetSize() {
    return SoftFailNp(__func__);
    return 0;
}
std::uint32_t* APS5_VABI sceAgcDcbSetUcRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, std::uint32_t numRegs) {
    return Agc::Command::WriteIndirectRegisters(buf, 0x64u, regs, numRegs, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbSetUcRegistersIndirectGetSize(std::uint32_t numRegs) {
    (void)numRegs;
    return SoftFailNp(__func__);
    return 0;
}
}
