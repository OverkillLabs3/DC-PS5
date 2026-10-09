#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0x800242e2 (online service not available)\n", func);
    std::fflush(stderr);
    return static_cast<int>(0x800242e2u);
}
}  // namespace

extern "C" {

int APS5_VABI sceGameUpdateAbortRequest(int request_id) {
 (void)request_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceGameUpdateCheck(int request_id, const GameUpdateCheckParam* param, GameUpdateCheckResult* result) {
 (void)request_id;
 (void)param;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceGameUpdateCreateRequest(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceGameUpdateDeleteRequest(int request_id) {
 (void)request_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceGameUpdateGetAddcontLatestVersion(uint32_t service_label, const void* entitlement_label, GameUpdateAddcontVersionInfo* info) {
 (void)service_label;
 (void)entitlement_label;
 (void)info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceGameUpdateInitialize(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceGameUpdateTerminate(void) {
 return SoftFailNp(__func__);
 return 0;
}
}
