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

int APS5_VABI sceSaveDataBackup(const SaveDataBackup* backup) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataBackup\n");
    std::fflush(stderr);
 (void)backup;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSaveDataGetEventResult(const void* event_param, SaveDataEvent* event) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataGetEventResult\n");
    std::fflush(stderr);
 (void)event_param;
 (void)event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSaveDataGetSaveDataMemory2(SaveDataMemoryGet2* get_param) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataGetSaveDataMemory2\n");
    std::fflush(stderr);
 (void)get_param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSaveDataSetSaveDataMemory2(const SaveDataMemorySet2* set_param) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataSetSaveDataMemory2\n");
    std::fflush(stderr);
 (void)set_param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSaveDataSetupSaveDataMemory2(const SaveDataMemorySetup2* setup_param, SaveDataMemorySetupResult* result) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataSetupSaveDataMemory2\n");
    std::fflush(stderr);
 (void)setup_param;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSaveDataSyncSaveDataMemory(const void* sync_param) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataSyncSaveDataMemory\n");
    std::fflush(stderr);
 (void)sync_param;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSaveDataTransferringMount(const SaveDataTransferringMount* mount, SaveDataMountResult* mount_result) {
    std::fprintf(stderr, "[SAVEDATA:stub] sceSaveDataTransferringMount\n");
    std::fflush(stderr);
 (void)mount;
 (void)mount_result;
 return SoftFailNp(__func__);
 return 0;
}
}
