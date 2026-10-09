#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "SaveData.hpp"

static constexpr char SAVE_DIR[] = "_sd";

static std::atomic<std::int32_t> g_transaction_counter{1};
static bool g_initialized = false;

static std::string save_root() {
    return std::string(SAVE_DIR);
}

static bool dir_name_match(const char* str, const char* pattern) {
    if (pattern == nullptr || pattern[0] == '\0') {
        return true;
    }
    while (*str != '\0' && *pattern != '\0') {
        if (*pattern == '%') {
            for (const char* s = str;; s++) {
                if (dir_name_match(s, pattern + 1)) {
                    return true;
                }
                if (*s == '\0') {
                    break;
                }
            }
            return false;
        }
        if (*pattern == '_') {
            str++;
            pattern++;
            continue;
        }
        if (*pattern != *str) {
            return false;
        }
        str++;
        pattern++;
    }
    return *str == '\0' && *pattern == '\0';
}


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

int APS5_VABI sceSaveDataBackup(const SaveDataBackup* backup) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataBackup\n");
    std::fflush(stderr);
    (void)backup;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataCommit(const SaveDataCommitParam* param) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataCommit\n");
    std::fflush(stderr);
    (void)param;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataCreateTransactionResource(uint32_t size) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataCreateTransactionResource\n");
    std::fflush(stderr);
    (void)size;
    return g_transaction_counter.fetch_add(1);
}

int APS5_VABI sceSaveDataDelete(const SaveDataDelete* del) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataDelete\n");
    std::fflush(stderr);
    if (del == nullptr || del->dir_name == nullptr) {
        throw std::runtime_error("sceSaveDataDelete: null argument");
    }
    const std::string path = save_root() + "/" + std::string(del->dir_name->data);
    if (std::filesystem::is_directory(path)) {
        std::filesystem::remove_all(path);
    }
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataDeleteTransactionResource(int32_t resource) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataDeleteTransactionResource\n");
    std::fflush(stderr);
    (void)resource;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataDirNameSearch(const SaveDataDirNameSearchCond* cond, SaveDataDirNameSearchResult* result) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataDirNameSearch\n");
    std::fflush(stderr);
    if (cond == nullptr || result == nullptr) {
        throw std::runtime_error("sceSaveDataDirNameSearch: null argument");
    }
    result->hit_num = 0;
    result->set_num = 0;
    const char* pattern = (cond->dir_name != nullptr) ? cond->dir_name->data : nullptr;
    const std::string root = save_root();
    if (!std::filesystem::is_directory(root)) {
        return SAVE_DATA_OK;
    }
    std::uint32_t hit = 0;
    std::uint32_t set = 0;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (!dir_name_match(name.c_str(), pattern)) {
            continue;
        }
        hit++;
        if (result->dir_names != nullptr && set < result->dir_names_num) {
            std::snprintf(result->dir_names[set].data, sizeof(result->dir_names[set].data), "%s", name.c_str());
            if (result->params != nullptr) {
                std::memset(&result->params[set], 0, sizeof(SaveDataParam));
            }
            set++;
        }
    }
    result->hit_num = hit;
    result->set_num = set;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataGetEventResult(const void* event_param, SaveDataEvent* event) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataGetEventResult\n");
    std::fflush(stderr);
    (void)event_param;
    (void)event;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataGetMountInfo(const SaveDataMountPoint* mount_point, SaveDataMountInfo* info) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataGetMountInfo\n");
    std::fflush(stderr);
    if (mount_point == nullptr || info == nullptr) {
        throw std::runtime_error("sceSaveDataGetMountInfo: null argument");
    }
    if (find_slot_by_mount_point(mount_point->data) == -1) {
        return SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    std::memset(info, 0, sizeof(*info));
    info->blocks = SAVE_DATA_BLOCKS_MAX;
    info->free_blocks = SAVE_DATA_BLOCKS_MAX;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataGetParam(const SaveDataMountPoint* mount_point, uint32_t param_type, void* param_buf, size_t param_buf_size, size_t* got_size) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataGetParam\n");
    std::fflush(stderr);
    (void)param_type;
    if (mount_point == nullptr || param_buf == nullptr) {
        throw std::runtime_error("sceSaveDataGetParam: null argument");
    }
    if (find_slot_by_mount_point(mount_point->data) == -1) {
        return SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    std::memset(param_buf, 0, param_buf_size);
    if (got_size != nullptr) {
        *got_size = param_buf_size;
    }
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataGetSaveDataMemory2(SaveDataMemoryGet2* get_param) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataGetSaveDataMemory2\n");
    std::fflush(stderr);
    (void)get_param;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataInitialize3(const void* init) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataInitialize3\n");
    std::fflush(stderr);
    (void)init;
    // return SoftFailNp(__func__);
    if (g_initialized) {
        return SAVE_DATA_ERROR_ALREADY_INITIALIZED;
    }
    g_initialized = true;
    return SAVE_DATA_OK;
}
int APS5_VABI sceSaveDataLoadIcon(const SaveDataMountPoint* mount_point, SaveDataIcon* icon) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataLoadIcon\n");
    std::fflush(stderr);
    (void)icon;
    if (mount_point == nullptr) {
        throw std::runtime_error("sceSaveDataLoadIcon: null mount_point");
    }
    if (find_slot_by_mount_point(mount_point->data) == -1) {
        return SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    if (icon != nullptr) {
        icon->data_size = 0;
    }
    return SAVE_DATA_OK;
}

static int Mount3Impl(const SaveDataMount3* mount, SaveDataMountResult* mount_result) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataMount3\n");
    std::fflush(stderr);
    if (mount == nullptr || mount_result == nullptr || mount->dir_name == nullptr) {
        throw std::runtime_error("sceSaveDataMount3: null argument");
    }
    std::memset(mount_result, 0, sizeof(*mount_result));
    const bool create = (mount->mount_mode & SAVE_DATA_MOUNT_MODE_CREATE) != 0;
    const bool create2 = (mount->mount_mode & SAVE_DATA_MOUNT_MODE_CREATE2) != 0;
    const bool rdonly = (mount->mount_mode & SAVE_DATA_MOUNT_MODE_RDONLY) != 0;
    const bool rdwr = (mount->mount_mode & SAVE_DATA_MOUNT_MODE_RDWR) != 0;
    const bool open = !create && !create2 && (rdonly || rdwr);
    if (!create && !create2 && !open) {
        throw std::runtime_error("sceSaveDataMount3: unknown mount_mode");
    }
    const auto* nameEnd = static_cast<const char*>(std::memchr(mount->dir_name->data, '\0', sizeof(mount->dir_name->data)));
    if (nameEnd == nullptr) {
        throw std::runtime_error("sceSaveDataMount3: unterminated directory name");
    }
    const std::string dirName(mount->dir_name->data, static_cast<std::size_t>(nameEnd - mount->dir_name->data));
    if (dirName.empty() || dirName == "." || dirName == ".." || dirName.find_first_of("/\\:") != std::string::npos) {
        throw std::runtime_error("sceSaveDataMount3: invalid directory name");
    }
    const std::string real_path = save_root() + "/" + dirName;
    const std::string mountPoint = "/" + real_path;
    if (mountPoint.size() >= sizeof(mount_result->mount_point.data)) {
        throw std::runtime_error("sceSaveDataMount3: directory path exceeds mount point capacity");
    }
    if (find_slot_by_mount_point(mountPoint.c_str()) != -1) {
        return SAVE_DATA_ERROR_BUSY;
    }
    const bool exists = std::filesystem::is_directory(real_path);
    if (create && exists) {
        return SAVE_DATA_ERROR_EXISTS;
    }
    if (open && !exists) {
        return SAVE_DATA_ERROR_NOT_FOUND;
    }
    int slot = find_free_slot();
    if (slot == -1) {
        return SAVE_DATA_ERROR_MOUNT_FULL;
    }
    if (create || create2) {
        std::filesystem::create_directories(real_path);
    }
    g_slots[slot].used = true;
    g_slots[slot].mount_point = mountPoint;
    g_slots[slot].real_path = real_path;
    std::memcpy(mount_result->mount_point.data, mountPoint.c_str(), mountPoint.size() + 1);
    mount_result->required_blocks = 0;
    mount_result->mount_status = (create || create2) ? 1u : 0u;
    return SAVE_DATA_OK;
}

}

extern "C" {
int APS5_VABI sceSaveDataMount3(const SaveDataMount3* mount, SaveDataMountResult* mount_result) {
    const int result = Mount3Impl(mount, mount_result);
    std::fprintf(stderr, "[SAVEDATA:native] Mount3 dir=%s mode=0x%x -> 0x%08x mountPoint=%s\n",
        mount != nullptr && mount->dir_name != nullptr ? mount->dir_name->data : "?", mount != nullptr ? static_cast<unsigned>(mount->mount_mode) : 0u,
        static_cast<unsigned>(result), mount_result != nullptr ? mount_result->mount_point.data : "?");
    std::fflush(stderr);
    return result;
}
}

extern "C" {
int APS5_VABI sceSaveDataPrepare(const SaveDataMountPoint* mount_point, const SaveDataPrepareParam* param) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataPrepare\n");
    std::fflush(stderr);
    (void)mount_point;
    (void)param;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataSaveIcon(const SaveDataMountPoint* mount_point, const SaveDataIcon* icon) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataSaveIcon\n");
    std::fflush(stderr);
    (void)icon;
    if (mount_point == nullptr) {
        throw std::runtime_error("sceSaveDataSaveIcon: null mount_point");
    }
    if (find_slot_by_mount_point(mount_point->data) == -1) {
        return SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataSaveIconByPath(const SaveDataMountPoint* mount_point, const char* path) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataSaveIconByPath\n");
    std::fflush(stderr);
    (void)mount_point;
    (void)path;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataSetParam(const SaveDataMountPoint* mount_point, uint32_t param_type, const void* param_buf, size_t param_buf_size) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataSetParam\n");
    std::fflush(stderr);
    (void)param_type;
    (void)param_buf;
    (void)param_buf_size;
    if (mount_point == nullptr) {
        throw std::runtime_error("sceSaveDataSetParam: null mount_point");
    }
    if (find_slot_by_mount_point(mount_point->data) == -1) {
        return SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataSetSaveDataMemory2(const SaveDataMemorySet2* set_param) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataSetSaveDataMemory2\n");
    std::fflush(stderr);
    (void)set_param;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataSetupSaveDataMemory2(const SaveDataMemorySetup2* setup_param, SaveDataMemorySetupResult* result) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataSetupSaveDataMemory2\n");
    std::fflush(stderr);
    (void)setup_param;
    (void)result;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataSyncSaveDataMemory(const void* sync_param) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataSyncSaveDataMemory\n");
    std::fflush(stderr);
    (void)sync_param;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataTerminate(void) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataTerminate\n");
    std::fflush(stderr);
    if (!g_initialized) {
        return SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (any_slot_used()) {
        return SAVE_DATA_ERROR_BUSY;
    }
    g_initialized = false;
    return SAVE_DATA_OK;
}

int APS5_VABI sceSaveDataTransferringMount(const SaveDataTransferringMount* mount, SaveDataMountResult* mount_result) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataTransferringMount\n");
    std::fflush(stderr);
    (void)mount;
    (void)mount_result;
    return SoftFailNp(__func__);
    return 0;
}
int APS5_VABI sceSaveDataUmount2(uint32_t mode, const SaveDataMountPoint* mount_point) {
    std::fprintf(stderr, "[SAVEDATA:native] sceSaveDataUmount2\n");
    std::fflush(stderr);
    (void)mode;
    if (mount_point == nullptr) {
        throw std::runtime_error("sceSaveDataUmount2: null mount_point");
    }
    int slot = find_slot_by_mount_point(mount_point->data);
    if (slot == -1) {
        return SAVE_DATA_ERROR_NOT_MOUNTED;
    }
    g_slots[slot] = MountSlot{};
    return SAVE_DATA_OK;
}

}
