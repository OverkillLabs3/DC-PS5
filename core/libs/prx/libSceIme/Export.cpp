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

int APS5_VABI sceImeClose_nid_postfix(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeGetPanelSize(const Param* param, uint32_t* width, uint32_t* height) {
 (void)param;
 (void)width;
 (void)height;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeKeyboardClose(int32_t user_id) {
 (void)user_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeKeyboardGetInfo(uint32_t resource_id, KeyboardInfo* info) {
 (void)resource_id;
 (void)info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeKeyboardGetResourceId(int32_t user_id, KeyboardResourceIdArray* resource_ids) {
 (void)user_id;
 (void)resource_ids;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeKeyboardOpen(int32_t user_id, const KeyboardParam* param) {
 (void)user_id;
 (void)param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeKeyboardSetMode(int32_t user_id, uint32_t mode) {
 (void)user_id;
 (void)mode;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeOpen_nid_postfix(const Param* param, const ExtendedParam* extended) {
 (void)param;
 (void)extended;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceImeParamInit(Param* param) {
 (void)param;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceImeSetCaret(const Caret* caret) {
 (void)caret;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeSetText(const char16_t* text, uint32_t length) {
 (void)text;
 (void)length;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeSetTextGeometry(TextAreaMode mode, const TextGeometry* geometry) {
 (void)mode;
 (void)geometry;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeUpdate(EventHandler handler) {
 (void)handler;
 return SoftFailNp(__func__);
 return 0;
}
}
