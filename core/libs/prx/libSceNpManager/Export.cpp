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

int APS5_VABI sceNpAbortRequest(int req_id) {
 (void)req_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpCheckCallback(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpCheckNpAvailability(int req_id, const char* user, void* result) {
 (void)req_id;
 (void)user;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpCheckNpReachability(int req_id, int user_id) {
 (void)req_id;
 (void)user_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpCheckPremium(int req_id, const NpCheckPremiumParameter* param, NpCheckPremiumResult* result) {
 (void)req_id;
 (void)param;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpCreateAsyncRequest(const NpCreateAsyncRequestParameter* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpCreateRequest(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpDeleteRequest(int req_id) {
 (void)req_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetAccountAge(int req_id, int user_id, uint8_t* age) {
 (void)req_id;
 (void)user_id;
 (void)age;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetAccountCountryA(int user_id, void* country_code) {
 (void)user_id;
 (void)country_code;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetAccountIdA(int user_id, uint64_t* account_id) {
 (void)user_id;
 (void)account_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetNpId(int user_id, NpId* np_id) {
 (void)user_id;
 (void)np_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetNpReachabilityState(int user_id, uint32_t* state) {
 (void)user_id;
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetOnlineId(int user_id, NpOnlineId* online_id) {
 (void)user_id;
 (void)online_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpGetState(int user_id, uint32_t* state) {
 (void)user_id;
 (void)state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpHasSignedUp(int user_id, bool* has_signed_up) {
 (void)user_id;
 (void)has_signed_up;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpPollAsync(int req_id, int* result) {
 (void)req_id;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceNpRegisterGamePresenceCallback(void* callback, void* userdata) {
 (void)callback;
 (void)userdata;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceNpRegisterNpReachabilityStateCallback(void* callback, void* userdata) {
 (void)callback;
 (void)userdata;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpRegisterPlusEventCallback(void* callback, void* userdata) {
 (void)callback;
 (void)userdata;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpRegisterPremiumEventCallback(void* callback, void* userdata) {
 (void)callback;
 (void)userdata;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpRegisterStateCallback(void* callback, void* userdata) {
 (void)callback;
 (void)userdata;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpSetContentRestriction(const NpContentRestriction* restriction) {
 (void)restriction;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpSetNpTitleId(const NpTitleId* title_id, const NpTitleSecret* title_secret) {
 (void)title_id;
 (void)title_secret;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpUnregisterStateCallback(void) {
 return SoftFailNp(__func__);
 return 0;
}
}
