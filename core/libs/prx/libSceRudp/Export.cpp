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

int APS5_VABI sceRudpEnableInternalIOThread(uint32_t stack_size, uint32_t priority) {
 (void)stack_size;
 (void)priority;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceRudpInit_nid_postfix(void* mem_pool, int mem_pool_size) {
 (void)mem_pool;
 (void)mem_pool_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceRudpSetEventHandler(RudpEventHandler handler, void* arg) {
 (void)handler;
 (void)arg;
 return SoftFailNp(__func__);
 return 0;
}
}
