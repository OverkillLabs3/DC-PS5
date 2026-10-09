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

const uint8_t* sceCesRefersUcsProfileCp1252(void) {
 return nullptr;  // SoftFailNp: pointer-returning
}
int APS5_VABI sceCesSbcToUtf8(const uint8_t* profile, uint8_t sbc, uint8_t* utf8, uint32_t utf8max, uint32_t* utf8_len) {
 (void)profile;
 (void)sbc;
 (void)utf8;
 (void)utf8max;
 (void)utf8_len;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceCesUtf8ToSbc(const uint8_t* utf8, uint32_t utf8max, uint32_t* utf8_len, const uint8_t* profile, uint8_t* sbc) {
 (void)utf8;
 (void)utf8max;
 (void)utf8_len;
 (void)profile;
 (void)sbc;
 return SoftFailNp(__func__);
 return 0;
}
}
