#include "prx/libSceAgcDriver/State/include/Configuration.hpp"

#include <cstdint>
#include <cstdio>
#include <cstddef>
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

int APS5_VABI sceAgcDriverSetHsOffchipParam(uint64_t value0, uint64_t value1, uint64_t value2) {
 (void)value0;
 (void)value1;
 (void)value2;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceAgcDriverSetTFRing(const volatile void* base, uint32_t size) {
 (void)base;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
}
