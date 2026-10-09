#include "prx/libSceAgcDriver/Eq/include/Query.hpp"

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented GPU command emitter - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

uint32_t APS5_VABI sceAgcDriverGetEqContextId(const KernelEvent* ev) {
 (void)ev;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAgcDriverGetEqEventType(const KernelEvent* ev) {
    if (ev == nullptr || reinterpret_cast<std::uintptr_t>(ev) % alignof(KernelEvent) != 0) {
        throw std::runtime_error(std::string(__func__) + ": null or misaligned event");
    }
    if (ev->filter == -14) {
        if (ev->ident > static_cast<std::uintptr_t>(std::numeric_limits<int>::max())) {
            throw std::runtime_error(std::string(__func__) + ": graphics event type overflow");
        }
        return static_cast<int>(ev->ident);
    }
    if (ev->data < std::numeric_limits<int>::min() || ev->data > std::numeric_limits<int>::max()) {
        throw std::runtime_error(std::string(__func__) + ": event type overflow");
    }
    return static_cast<int>(ev->data);
}

}
