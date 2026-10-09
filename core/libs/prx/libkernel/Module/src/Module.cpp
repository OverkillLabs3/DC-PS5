#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

int APS5_VABI sceKernelDlsym(KernelModule handle, const char* symbol, void** addr) {
 (void)handle;
 (void)symbol;
 (void)addr;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelGetModuleInfoForUnwind(uint64_t addr, int flags, ModuleInfoForUnwind* info) {
 (void)addr;
 (void)flags;
 (void)info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelGetModuleInfoFromAddr(uint64_t addr, int n, ModuleInfo* r) {
 (void)addr;
 (void)n;
 (void)r;
 return SoftFailNp(__func__);
 return 0;
}
KernelModule APS5_VABI sceKernelLoadStartModule(const char* module_file_name, size_t args, const void* argp, uint32_t flags, const KernelLoadModuleOpt* opt, int* res) {
 (void)module_file_name;
 (void)args;
 (void)argp;
 (void)flags;
 (void)opt;
 (void)res;
 return SoftFailNp(__func__);
 return {};
}
int APS5_VABI sceKernelStopUnloadModule(KernelModule handle, size_t args, const void* argp, uint32_t flags, const KernelUnloadModuleOpt* opt, int* res) {
 (void)handle;
 (void)args;
 (void)argp;
 (void)flags;
 (void)opt;
 (void)res;
 return SoftFailNp(__func__);
 return 0;
}
}
