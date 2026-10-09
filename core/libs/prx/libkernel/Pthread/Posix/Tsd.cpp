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

void* APS5_VABI pthread_getspecific_nid_postfix(PthreadKey key) {
 (void)key;
 SoftFailNp(__func__);

 return nullptr;
}
int APS5_VABI pthread_setspecific_nid_postfix(PthreadKey key, void* value) {
 (void)key;
 (void)value;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_key_create_nid_postfix(PthreadKey* key, pthread_key_destructor_func_t destructor) {
 (void)key;
 (void)destructor;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI pthread_key_delete_nid_postfix(PthreadKey key) {
 (void)key;
 return SoftFailNp(__func__);
 return 0;
}
}
