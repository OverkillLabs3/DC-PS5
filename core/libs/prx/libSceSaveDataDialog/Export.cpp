#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Throwing from these reaches std::terminate and aborts the game process.
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

 int APS5_VABI sceSaveDataDialogClose(const void* close_param) {
  (void)close_param;
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogGetResult(void* result) {
  (void)result;
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogInitialize(void) {
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogIsReadyToDisplay(void) {
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogOpen(const void* param) {
  (void)param;
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogProgressBarInc(int target, uint32_t delta) {
  (void)target;
  (void)delta;
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogProgressBarSetValue(int target, uint32_t rate) {
  (void)target;
  (void)rate;
  return SoftFailNp(__func__);
 }

 int APS5_VABI sceSaveDataDialogTerminate(void) {
  return SoftFailNp(__func__);
 }

}
