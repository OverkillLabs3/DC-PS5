#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

uint32_t Need_sceLibc = 1;


namespace {
// Throwing from these reaches std::terminate and aborts the game process.
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

    void APS5_VABI init_env_nid_postfix(const InitEnvParams* params) {
        (void)params;
        SoftFailNp(__func__);
    }

    void APS5_VABI catchReturnFromMain_nid_postfix(int status) {
        (void)status;
        SoftFailNp(__func__);
    }

    int APS5_VABI cxa_atexit_nid_postfix(void (*func)(void*), void* arg, void* d) {
        (void)func;
        (void)arg;
        (void)d;
        return SoftFailNp(__func__);
    }

    void APS5_VABI cxa_finalize_nid_postfix(void* d) {
        SoftFailNp(__func__);
        (void)d;
    }

    int APS5_VABI std_execute_once_nid_postfix(int* flag, int (*func)(void*, void*, void**), void* arg) {
        (void)flag;
        (void)func;
        (void)arg;
        return SoftFailNp(__func__);
    }

    void APS5_VABI LibcHeapGetTraceInfo_nid_postfix(LibcHeapInfo* info) {
        (void)info;
        SoftFailNp(__func__);
    }

    int APS5_VABI LibcInternalExtCxaThreadAtexit_nid_postfix(void (*destructor)(void*), void* object, void* module_id) {
        (void)destructor;
        (void)object;
        (void)module_id;
        return SoftFailNp(__func__);
    }

    int APS5_VABI LibcHeapErrorReportForGame_nid_postfix(
        uint64_t msp, uint64_t ptr, uint64_t error,
        uint64_t arg3, uint64_t arg4, uint64_t arg5
    ) {
        (void)msp; (void)ptr; (void)error;
        (void)arg3; (void)arg4; (void)arg5;
        return SoftFailNp(__func__);
    }

}
