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

int APS5_VABI sceKernelMemoryPoolBatch(const KernelMemoryPoolBatchEntry* entries, int num_entries, int* num_entries_out, int flags) {
 (void)entries;
 (void)num_entries;
 (void)num_entries_out;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelMemoryPoolCommit(void* addr, size_t len, int type, int prot, int flags) {
 (void)addr;
 (void)len;
 (void)type;
 (void)prot;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelMemoryPoolDecommit(void* addr, size_t len, int flags) {
 (void)addr;
 (void)len;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelMemoryPoolExpand(int64_t search_start, int64_t search_end, size_t len, size_t alignment, int64_t* phys_addr_out) {
 (void)search_start;
 (void)search_end;
 (void)len;
 (void)alignment;
 (void)phys_addr_out;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelMemoryPoolGetBlockStats(KernelMemoryPoolBlockStats* output, size_t output_size) {
 (void)output;
 (void)output_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceKernelMemoryPoolReserve(void* addr_in, size_t len, size_t alignment, int flags, void** addr_out) {
 (void)addr_in;
 (void)len;
 (void)alignment;
 (void)flags;
 (void)addr_out;
 return SoftFailNp(__func__);
 return 0;
}
}
