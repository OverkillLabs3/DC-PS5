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

int APS5_VABI sceVoiceConnectIPortToOPort(uint32_t input_port_id, uint32_t output_port_id) {
 (void)input_port_id;
 (void)output_port_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceCreatePort(uint32_t* port_id, const VoicePortParam* param) {
 (void)port_id;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceDeletePort(uint32_t port_id) {
 (void)port_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceDisconnectIPortFromOPort(uint32_t input_port_id, uint32_t output_port_id) {
 (void)input_port_id;
 (void)output_port_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceEnd_nid_postfix(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceGetBitRate(uint32_t port_id, uint32_t* bitrate) {
 (void)port_id;
 (void)bitrate;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceGetPortAttr(uint32_t port_id, int32_t attr, void* value, int32_t size) {
 (void)port_id;
 (void)attr;
 (void)value;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceGetPortInfo(uint32_t port_id, VoicePortInfo* info) {
 (void)port_id;
 (void)info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceGetVolume(uint32_t port_id, float* volume) {
 (void)port_id;
 (void)volume;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceInit(VoiceInitParam* param, int32_t version) {
 (void)param;
 (void)version;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceReadFromOPort(uint32_t output_port_id, void* data, uint32_t* size) {
 (void)output_port_id;
 (void)data;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceSetThreadsParams(void* params) {
 (void)params;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceSetVolume(uint32_t port_id, float volume) {
 (void)port_id;
 (void)volume;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceStart(const VoiceStartParam* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceStop(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceVoiceWriteToIPort(uint32_t input_port_id, const void* data, uint32_t* size, int16_t frame_gaps) {
 (void)input_port_id;
 (void)data;
 (void)size;
 (void)frame_gaps;
 return SoftFailNp(__func__);
 return 0;
}
}
