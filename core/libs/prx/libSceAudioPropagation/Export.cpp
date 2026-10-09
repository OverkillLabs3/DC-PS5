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

int32_t APS5_VABI sceAudioPropagationRoomCreate(AudioPropagationHandle system_handle, AudioPropagationHandle* out_room_handle) {
 (void)system_handle;
 (void)out_room_handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudioPropagationSystemCreate(const void* options, AudioPropagationSystemMemory* memory, AudioPropagationHandle* out_system_handle) {
 (void)options;
 (void)memory;
 (void)out_system_handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudioPropagationSystemGetRays(AudioPropagationHandle system_handle, void* rays, uint32_t* num_rays) {
 (void)system_handle;
 (void)rays;
 (void)num_rays;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudioPropagationSystemQueryMemory(const void* options, AudioPropagationSystemMemory* out_memory) {
 (void)options;
 (void)out_memory;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudioPropagationSystemRegisterMaterial(AudioPropagationHandle system_handle, const void* material, AudioPropagationHandle* out_material_handle) {
 (void)system_handle;
 (void)material;
 (void)out_material_handle;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceAudioPropagationSystemSetAttributes(AudioPropagationHandle system_handle, const void* attributes, uint32_t num_attributes) {
 (void)system_handle;
 (void)attributes;
 (void)num_attributes;
 return SoftFailNp(__func__);
 return 0;
}
}
