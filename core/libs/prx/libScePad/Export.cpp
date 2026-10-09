#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <thread>

#include "SceTypes.hpp"
#include "prx//libc/include/General.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"

// A thrown exception here reaches std::terminate and aborts the whole game process.
namespace {
int PadSoftFail(const char* func) {
    std::fprintf(stderr, "[PAD] %s -> 0 (no-op stub)\n", func);
    std::fflush(stderr);
    return 0;
}

// Output-only pad features (vibration, light bar, trigger effects, ...) have no host counterpart on the
// virtual pad, so they validate the handle and succeed without side effects, exactly like a pad without haptics.
int PadOutputOnly(int handle) {
    return handle == PAD_HANDLE ? PAD_OK : PAD_ERROR_INVALID_HANDLE;
}

// Single virtual pad on port 0 for the standard/special port types or the system remote.
bool PadPortValid(int userId, int type, int index) {
    if (index != 0) return false;
    const bool personalPort = (type == PAD_PORT_TYPE_STANDARD || type == PAD_PORT_TYPE_SPECIAL);
    const bool systemRemote = (userId == PAD_USER_ID_SYSTEM && type == PAD_PORT_TYPE_REMOTE);
    return personalPort || systemRemote;
}
}  // namespace

extern "C" {
int APS5_VABI scePadClose_nid_postfix(int handle) {
 if (handle != PAD_HANDLE) {
  return PAD_ERROR_INVALID_HANDLE;
 }
 return PAD_OK;
}

int APS5_VABI scePadDeviceClassGetExtendedInformation(int handle, PadDeviceClassExtendedInformation* info) {
 (void)handle;
 if (info != nullptr) std::memset(info, 0, sizeof(*info));
 return PadSoftFail(__func__);
}

int APS5_VABI scePadDeviceClassParseData(int handle, const PadData* data, PadDeviceClassData* class_data) {
 (void)handle;
 (void)data;
 if (class_data != nullptr) std::memset(class_data, 0, sizeof(*class_data));
 return PadSoftFail(__func__);
}

int APS5_VABI scePadGetControllerInformation(int handle, PadControllerInformation* info) {
 if (handle != PAD_HANDLE) {
  return PAD_ERROR_INVALID_HANDLE;
 }
 if (info == nullptr) {
  return PAD_ERROR_INVALID_ARG;
 }
 std::memset(info, 0, sizeof(*info));
 info->touchPadInfo.pixelDensity = 44.86f;
 info->touchPadInfo.resolution.x = 1920;
 info->touchPadInfo.resolution.y = 943;
 info->stickInfo.deadZoneLeft = 2;
 info->stickInfo.deadZoneRight = 2;
 info->connectionType = PAD_CONNECTION_TYPE_LOCAL;
 info->connectedCount = 1;
 info->connected = true;
 info->deviceClass = PAD_DEVICE_CLASS_STANDARD;
 return PAD_OK;
}

int APS5_VABI scePadGetHandle(int user_id, int type, int index) {
 // Returns the handle of the single virtual pad, like scePadOpen for an already opened port.
 return PadPortValid(user_id, type, index) ? PAD_HANDLE : PAD_ERROR_INVALID_ARG;
}

int APS5_VABI scePadGetTriggerEffectState(int handle, PadTriggerEffectStateInformation* info) {
 (void)handle;
 if (info != nullptr) std::memset(info, 0, sizeof(*info));
 return PadSoftFail(__func__);
}

int APS5_VABI scePadInit_nid_postfix(void) {
 Pad::Initialize();
 return PAD_OK;
}

int APS5_VABI scePadOpen_nid_postfix(int userId, int type, int index, const void* param) {
 (void)param;
 return PadPortValid(userId, type, index) ? PAD_HANDLE : PAD_ERROR_INVALID_ARG;
}

int APS5_VABI scePadRead_nid_postfix(int handle, PadData* data, int num) {
 if (handle != PAD_HANDLE || data == nullptr || num < 1) {
  return PAD_ERROR_INVALID_ARG;
 }
 (void)num;
 *data = Pad::ReadState();
 return 1;
}

int APS5_VABI scePadReadState(int handle, PadData* data) {
 if (handle != PAD_HANDLE) return PAD_ERROR_INVALID_HANDLE;
 if (data == nullptr) return PAD_ERROR_INVALID_ARG;

 *data = Pad::ReadState();

 return PAD_OK;
}

int APS5_VABI scePadResetLightBar(int handle) {
 return PadOutputOnly(handle);
}

int APS5_VABI scePadResetOrientation(int handle) {
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetAngularVelocityDeadbandState(int handle, bool enable) {
 (void)enable;
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetLightBar(int handle, const PadLightBarParam* param) {
 (void)param;
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetMotionSensorState(int handle, bool enable) {
 (void)enable;
 // if (enable) {
 //  throw std::runtime_error("scePadSetMotionSensorState: motion sensor not supported");
 // }
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetTiltCorrectionState(int handle, bool enabled) {
 (void)enabled;
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetTriggerEffect(int handle, const void* param) {
 (void)param;
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetVibration(int handle, const PadVibrationParam* param) {
 (void)param;
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetVibrationMode(int handle, int mode) {
 (void)mode;
 return PadOutputOnly(handle);
}

int APS5_VABI scePadSetVibrationTriggerEffectWeakWhileEmbeddedMicInUse(bool enabled) {
 (void)enabled;
 return PAD_OK;
}

}
