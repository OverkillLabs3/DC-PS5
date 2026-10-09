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

int APS5_VABI sceImeDialogAbort(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogGetPanelPositionAndForm(PositionAndForm* form) {
 (void)form;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogGetPanelSize(const Param* param, uint32_t* width, uint32_t* height) {
 (void)param;
 (void)width;
 (void)height;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogGetPanelSizeExtended(const Param* param, const ExtendedParam* extended, uint32_t* width, uint32_t* height) {
 (void)param;
 (void)extended;
 (void)width;
 (void)height;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogGetResult(Result* result) {
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogGetStatus(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogInit(const Param* param, const ExtendedParam* extended) {
 (void)param;
 (void)extended;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceImeDialogTerm(void) {
 return SoftFailNp(__func__);
 return 0;
}
}
