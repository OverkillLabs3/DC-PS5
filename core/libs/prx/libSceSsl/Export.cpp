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

int APS5_VABI sceSslFreeCaCerts(int ssl_ctx_id, void* ca_certs) {
 (void)ssl_ctx_id;
 (void)ca_certs;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSslGetCaCerts(int ssl_ctx_id, void* ca_certs) {
 (void)ssl_ctx_id;
 (void)ca_certs;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSslInit_nid_postfix(uint64_t pool_size) {
 (void)pool_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceSslTerm_nid_postfix(int ssl_ctx_id) {
 (void)ssl_ctx_id;
 return SoftFailNp(__func__);
 return 0;
}
}
