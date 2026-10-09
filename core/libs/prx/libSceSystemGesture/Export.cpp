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

int APS5_VABI sceSystemGestureAppendTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
 (void)gesture_handle;
 (void)recognizer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureClose(int32_t gesture_handle) {
 (void)gesture_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureCreateTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, int32_t type, const SystemGestureRectangle* rectangle, const void* param) {
 (void)gesture_handle;
 (void)recognizer;
 (void)type;
 (void)rectangle;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureFinalizePrimitiveTouchRecognizer(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByIndex(int32_t gesture_handle, uint32_t index, SystemGesturePrimitiveTouchEvent* event) {
 (void)gesture_handle;
 (void)index;
 (void)event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(int32_t gesture_handle, uint16_t primitiveId, SystemGesturePrimitiveTouchEvent* event) {
 (void)gesture_handle;
 (void)primitiveId;
 (void)event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetPrimitiveTouchEvents(int32_t gesture_handle, SystemGesturePrimitiveTouchEvent* event_buffer, uint32_t capacity_of_buffer, uint32_t* number_of_event) {
 (void)gesture_handle;
 (void)event_buffer;
 (void)capacity_of_buffer;
 (void)number_of_event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventsCount(int32_t gesture_handle) {
 (void)gesture_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetTouchEventByEventID(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, uint32_t eventId, SystemGestureTouchEvent* event) {
 (void)gesture_handle;
 (void)recognizer;
 (void)eventId;
 (void)event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetTouchEventByIndex(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, uint32_t index, SystemGestureTouchEvent* event) {
 (void)gesture_handle;
 (void)recognizer;
 (void)index;
 (void)event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetTouchEvents(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchEvent* event_buffer, uint32_t capacity_of_buffer, uint32_t* number_of_event) {
 (void)gesture_handle;
 (void)recognizer;
 (void)event_buffer;
 (void)capacity_of_buffer;
 (void)number_of_event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetTouchEventsCount(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer) {
 (void)gesture_handle;
 (void)recognizer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureGetTouchRecognizerInformation(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchRecognizerInformation* information) {
 (void)gesture_handle;
 (void)recognizer;
 (void)information;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureInitializePrimitiveTouchRecognizer(const void* param) {
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int32_t APS5_VABI sceSystemGestureOpen(int32_t input_type, const void* param) {
 (void)input_type;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureRemoveTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
 (void)gesture_handle;
 (void)recognizer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureResetPrimitiveTouchRecognizer(int32_t gesture_handle) {
 (void)gesture_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureResetTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
 (void)gesture_handle;
 (void)recognizer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureUpdateAllTouchRecognizer(int32_t gesture_handle) {
 (void)gesture_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureUpdatePrimitiveTouchRecognizer(int32_t gesture_handle, const void* param) {
 (void)gesture_handle;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureUpdateTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
 (void)gesture_handle;
 (void)recognizer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSystemGestureUpdateTouchRecognizerRectangle(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, const SystemGestureRectangle* rectangle) {
 (void)gesture_handle;
 (void)recognizer;
 (void)rectangle;
 return SoftFailNp(__func__);
 return 0;
}
}
