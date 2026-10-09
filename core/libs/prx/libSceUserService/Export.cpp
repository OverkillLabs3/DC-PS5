#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceUserService/UserService.hpp"



extern "C" {

int APS5_VABI sceUserServiceGetAccessibilityChatTranscription(int user_id, int32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 0;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetAccessibilityPressAndHoldDelay(int user_id, int32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 0;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetAccessibilityTriggerEffect(int user_id, int32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 1;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetAccessibilityVibration(int user_id, int32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 1;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetAccessibilityZoomEnabled(int user_id, int32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 0;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetAgeLevel(int user_id, uint32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 0;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetEvent(SceUserServiceEvent* event) {
 // No login/logout events are ever raised for the single local user; leave *event untouched.
 (void)event;
 return USER_SERVICE_ERROR_NO_EVENT;
}
int APS5_VABI sceUserServiceGetGamePresets(int user_id, UserServiceGamePresets* presets) {
 (void)user_id;
 if (presets == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 std::memset(presets, 0, sizeof(*presets));
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetInitialUser(int* user_id) {
 if (user_id == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 *user_id = USER_SERVICE_INITIAL_USER_ID;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetLoginUserIdList(UserServiceLoginUserIdList* user_id_list) {
 if (user_id_list == nullptr) {
  return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 }
 user_id_list->user_id[0] = USER_SERVICE_INITIAL_USER_ID;
 user_id_list->user_id[1] = USER_SERVICE_USER_ID_INVALID;
 user_id_list->user_id[2] = USER_SERVICE_USER_ID_INVALID;
 user_id_list->user_id[3] = USER_SERVICE_USER_ID_INVALID;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceGetUserName(int user_id, char* name, size_t size) {
 (void)user_id;
 if (name == nullptr || size == 0) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 constexpr char kUserName[] = "Player";
 const size_t length = size - 1 < sizeof(kUserName) - 1 ? size - 1 : sizeof(kUserName) - 1;
 std::memcpy(name, kUserName, length);
 name[length] = '\0';
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceGetUserNumber(int user_id, int32_t* out) {
 if (out == nullptr) return USER_SERVICE_ERROR_INVALID_ARGUMENT;
 *out = 0;
 return USER_SERVICE_OK;
}
int APS5_VABI sceUserServiceInitialize(const void* params) {
 (void)params;
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceInitialize2(void) {
 return USER_SERVICE_OK;
}

int APS5_VABI sceUserServiceTerminate(void) {
 return USER_SERVICE_OK;
}

}
