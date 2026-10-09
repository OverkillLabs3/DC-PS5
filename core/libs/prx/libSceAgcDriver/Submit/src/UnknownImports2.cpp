#include <cstdint>
#include <cstdio>
#include "prx/libc/include/General.hpp"

// Instrumented capture-only stubs for ELF imports whose real SCE names are not
// present in any PRX export set. Bound by raw NID via APS5_EXPORT; each logs
// its arguments the first few times it is called so behavior can be inferred
// from live traces before writing a real implementation.

namespace {
int g_stubCount[32];

struct WinMBI {
    void* BaseAddress;
    void* AllocationBase;
    unsigned long AllocationProtect;
    unsigned long __rva1;
    unsigned long long RegionSize;
    unsigned long State;
    unsigned long Protect;
    unsigned long Type;
};
extern "C" __attribute__((dllimport)) unsigned long __attribute__((stdcall))
VirtualQuery(const void*, void*, unsigned long);

bool queryPtr(uint64_t a, uint64_t len, unsigned long* protectOut) {
    // boot29 crashed here: 0x4000 passes a range check but is not committed
    // memory; only touch args VirtualQuery reports as committed.
    WinMBI mbi;
    if (VirtualQuery(reinterpret_cast<const void*>(a), &mbi, sizeof(mbi)) != sizeof(mbi))
        return false;
    if (mbi.State != 0x1000 /* MEM_COMMIT */)
        return false;
    const unsigned long p = mbi.Protect & 0xff;
    if (p == 0x01 /* NOACCESS */ || mbi.Protect & 0x100 /* GUARD */)
        return false;
    const uint64_t regionStart = reinterpret_cast<uint64_t>(mbi.BaseAddress);
    if (regionStart + mbi.RegionSize < a + len)
        return false;
    if (protectOut)
        *protectOut = p;
    return true;
}

bool readablePtr(uint64_t a, uint64_t len) {
    unsigned long p = 0;
    if (!queryPtr(a, len, &p))
        return false;
    // readable: anything except PAGE_NOACCESS(1) and PAGE_EXECUTE(0x10)
    return p != 0x01 && p != 0x10;
}

bool writablePtr(uint64_t a, uint64_t len) {
    unsigned long p = 0;
    if (!queryPtr(a, len, &p))
        return false;
    return p == 0x04 || p == 0x08 || p == 0x40 || p == 0x80; // RW/WC/ERW/EWC
}

void LogStub(int slot, const char* nid, uint64_t a0, uint64_t a1, uint64_t a2,
             uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t ret) {
    if (g_stubCount[slot] < 6) {
        g_stubCount[slot]++;
        std::fprintf(stderr,
            "[STUB-%s] #%d ret=%llx a0=%llx a1=%llx a2=%llx a3=%llx a4=%llx a5=%llx a6=%llx\n",
            nid, g_stubCount[slot],
            (unsigned long long)ret,
            (unsigned long long)a0, (unsigned long long)a1,
            (unsigned long long)a2, (unsigned long long)a3,
            (unsigned long long)a4, (unsigned long long)a5,
            (unsigned long long)a6);
        const uint64_t args[7] = {a0, a1, a2, a3, a4, a5, a6};
        for (int i = 0; i < 7; ++i) {
            const uint64_t a = args[i];
            if (readablePtr(a, 32)) {
                const auto* p = reinterpret_cast<const uint64_t*>(a);
                std::fprintf(stderr, "  a%d@%llx: %llx %llx %llx %llx\n", i,
                    (unsigned long long)a,
                    (unsigned long long)p[0], (unsigned long long)p[1],
                    (unsigned long long)p[2], (unsigned long long)p[3]);
            }
        }
        std::fflush(stderr);
    }
}
} // namespace

#define STUB(NIDSTR, IDENT, SLOT) \
    extern "C" int64_t APS5_VABI stub_##IDENT(uint64_t a0, uint64_t a1, uint64_t a2, \
        uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) { \
        LogStub(SLOT, NIDSTR, a0, a1, a2, a3, a4, a5, a6, (uint64_t)__builtin_return_address(0)); \
        return 0; \
    } \
    APS5_EXPORT(NIDSTR, stub_##IDENT)

// Size-query variants: callers alloca(*(uint32*)arg0) bytes right after the
// call (observed at guest 0x16b4afa: lea/and/sub rsp pattern), so the out slot
// must contain a sane small size or the stack pointer wraps and faults.
#define STUB_SIZE(NIDSTR, IDENT, SLOT) \
    extern "C" int64_t APS5_VABI stub_##IDENT(uint64_t a0, uint64_t a1, uint64_t a2, \
        uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) { \
        LogStub(SLOT, NIDSTR, a0, a1, a2, a3, a4, a5, a6, (uint64_t)__builtin_return_address(0)); \
        if (writablePtr(a0, 4)) \
            *reinterpret_cast<uint32_t*>(a0) = 0x4000; \
        return 0; \
    } \
    APS5_EXPORT(NIDSTR, stub_##IDENT)

STUB("1TDo-ImqkJc", n_1TDo_ImqkJc, 0);
STUB("CCbC+lqqvF0", n_CCbC_lqqvF0, 1);
STUB("m-I92Ab50W8", n_m_I92Ab50W8, 2);
STUB("DHmwsa6S8Tc", n_DHmwsa6S8Tc, 3);
STUB("DfSCDRA3EjY", n_DfSCDRA3EjY, 4);
STUB("dsqCVsNM0Zg", n_dsqCVsNM0Zg, 5);
STUB("0aR2aWmQal4", n_0aR2aWmQal4, 6);
STUB("EHQEDVXZ0TI", n_EHQEDVXZ0TI, 7);
STUB_SIZE("F0ZXt5q0ZTA", n_F0ZXt5q0ZTA, 8);
STUB_SIZE("uJziRsODk1c", n_uJziRsODk1c, 9);
STUB("-KRzWekV120", n_KRzWekV120, 10);
STUB("T6xuVw0KUJo", n_T6xuVw0KUJo, 11);
STUB("AAeX-U5-P3M", n_AAeX_U5_P3M, 12);
STUB("DYhhWbJSeRg", n_DYhhWbJSeRg, 13);
STUB("pv9CI5VC+R0", n_pv9CI5VC_R0, 14);
STUB("TXFFFiNldU8", n_TXFFFiNldU8, 15);
