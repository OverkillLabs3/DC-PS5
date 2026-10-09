#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

void fillMountPoint(AppContentMountPoint* mount_point, const char* name) {
    std::memset(mount_point, 0, sizeof(*mount_point));
    std::strncpy(mount_point->data, name, sizeof(mount_point->data) - 1);
}

}

extern "C" {

int APS5_VABI sceAppContentAddcontMount(uint32_t service_label, const NpUnifiedEntitlementLabel* entitlement_label, AppContentMountPoint* mount_point) {
    (void)entitlement_label;
    APS5_LOG_ERR("accepted service_label=%u, faking mount point", service_label);
    char name[16];
    std::snprintf(name, sizeof(name), "AC%08u", service_label);
    fillMountPoint(mount_point, name);
    return 0;
}

int APS5_VABI sceAppContentAddcontUnmount(const AppContentMountPoint* mount_point) {
    (void)mount_point;
    return 0;
}

int APS5_VABI sceAppContentAppParamGetInt(uint32_t param_id, int32_t* value) {
    APS5_LOG_ERR("param_id=%u returning 0", param_id);
    *value = 0;
    return 0;
}

int APS5_VABI sceAppContentDownloadDataGetAvailableSpaceKb(const AppContentMountPoint* mount_point, size_t* available_space_kb) {
    (void)mount_point;
    *available_space_kb = 100ull * 1024ull * 1024ull;
    return 0;
}

int APS5_VABI sceAppContentInitialize(const AppContentInitParam* init_param, AppContentBootParam* boot_param) {
    APS5_LOG_ERR("init_param=%p boot_param=%p", static_cast<const void*>(init_param), static_cast<void*>(boot_param));
    if (boot_param != nullptr)
        std::memset(boot_param, 0, sizeof(*boot_param));
    return 0;
}

int APS5_VABI sceAppContentTemporaryDataFormat(const AppContentMountPoint* mount_point) {
    (void)mount_point;
    return 0;
}

int APS5_VABI sceAppContentTemporaryDataGetAvailableSpaceKb(const AppContentMountPoint* mount_point, size_t* available_space_kb) {
    (void)mount_point;
    *available_space_kb = 100ull * 1024ull * 1024ull;
    return 0;
}

int APS5_VABI sceAppContentTemporaryDataMount2(uint32_t option, AppContentMountPoint* mount_point) {
    APS5_LOG_ERR("option=%u faking mount point", option);
    fillMountPoint(mount_point, "TMPDATA");
    return 0;
}

}
