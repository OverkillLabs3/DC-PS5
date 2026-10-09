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

int APS5_VABI sceNpAuthAbortRequest(int req_id) {
 (void)req_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthCreateAsyncRequest(const void* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthCreateRequest(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthDeleteRequest(int req_id) {
 (void)req_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthGetAuthorizationCodeV3(int req_id, const void* param, void* auth_code, int* issuer_id) {
 (void)req_id;
 (void)param;
 (void)auth_code;
 (void)issuer_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthGetIdTokenV3(int req_id, const void* param, void* id_token) {
 (void)req_id;
 (void)param;
 (void)id_token;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthPollAsync(int req_id, int* result) {
 (void)req_id;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpAuthWaitAsync(int req_id, int* result) {
 (void)req_id;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
}
