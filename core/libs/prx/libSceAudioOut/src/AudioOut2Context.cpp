#include <cstddef>
#include <cstdint>
#include <cstdio>
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

int APS5_VABI sceAudioOut2ContextAdvance(AudioOut2ContextHandle ctx) {
    (void)ctx;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextCreate(const AudioOut2ContextParam* params, void* buffer, size_t buffer_size, AudioOut2ContextHandle* ctx) {
    (void)params;
    (void)buffer;
    (void)buffer_size;
    (void)ctx;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextDestroy(AudioOut2ContextHandle ctx) {
    (void)ctx;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextGetQueueLevel(AudioOut2ContextHandle ctx, uint32_t* queue_level, uint32_t* available_queues) {
    (void)ctx;
    (void)queue_level;
    (void)available_queues;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextPush(AudioOut2ContextHandle ctx, uint32_t blocking) {
    (void)ctx;
    (void)blocking;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextQueryMemory(const AudioOut2ContextParam* params, size_t* memory_size) {
    (void)params;
    (void)memory_size;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextResetParam(AudioOut2ContextParam* params) {
    (void)params;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceAudioOut2ContextSetAttributes(AudioOut2ContextHandle ctx, const AudioOut2Attribute* attributes, uint32_t num) {
    (void)ctx;
    (void)attributes;
    (void)num;
    return SoftFailNp(__func__);
    return 0;
}
}
