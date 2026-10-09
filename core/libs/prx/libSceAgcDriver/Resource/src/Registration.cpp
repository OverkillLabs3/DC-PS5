#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"

#include <atomic>
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {
std::atomic<uint32_t> g_nextOwnerHandle{1};
}

extern "C" {

// Owner/resource registration tracks which guest submission context owns which
// command buffers. The translating driver keys its Vulkan state off the command
// buffers themselves, so a monotonic non-zero handle is the full contract here.
int APS5_VABI sceAgcDriverRegisterOwner(void) {
 return static_cast<int>(g_nextOwnerHandle.fetch_add(1));
}

int APS5_VABI sceAgcDriverRegisterResource(void) {
 return 0;
}

int APS5_VABI sceAgcDriverRegisterWorkloadStream(uint32_t stream_id, const void* stream) {
 (void)stream_id;
 (void)stream;
 return 0;
}

int APS5_VABI sceAgcDriverUnregisterOwnerAndResources(uint32_t owner_handle) {
 (void)owner_handle;
 return 0;
}

int APS5_VABI sceAgcDriverUnregisterResource(void) {
 return 0;
}

}
