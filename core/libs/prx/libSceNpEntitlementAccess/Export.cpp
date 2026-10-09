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

int APS5_VABI sceNpEntitlementAccessGetAddcontEntitlementInfo(uint32_t service_label, const NpUnifiedEntitlementLabel* entitlement_label, NpEntitlementAccessAddcontEntitlementInfo* info) {
 (void)service_label;
 (void)entitlement_label;
 (void)info;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpEntitlementAccessGetAddcontEntitlementInfoList(uint32_t service_label, NpEntitlementAccessAddcontEntitlementInfo* list, uint32_t list_num, uint32_t* hit_num) {
 (void)service_label;
 (void)list;
 (void)list_num;
 (void)hit_num;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpEntitlementAccessGetSkuFlag(uint32_t* sku_flag) {
 (void)sku_flag;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNpEntitlementAccessInitialize(const NpEntitlementAccessInitParam* init_param, NpEntitlementAccessBootParam* boot_param) {
 (void)init_param;
 (void)boot_param;
 return SoftFailNp(__func__);
 return 0;
}
}
