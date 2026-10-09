#include <cstdint>
#include <cstdio>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

int unknownCallCount = 0;

void dumpWords(const char* tag, const void* pointer, int words) {
    if (pointer == nullptr) {
        std::fprintf(stderr, "%s=(null)\n", tag);
        std::fflush(stderr);
        return;
    }
    const auto* words32 = static_cast<const std::uint32_t*>(pointer);
    char line[512];
    int pos = std::snprintf(line, sizeof(line), "%s=", tag);
    for (int i = 0; i < words && pos < static_cast<int>(sizeof(line)) - 16; ++i)
        pos += std::snprintf(line + pos, sizeof(line) - pos, " %08x", words32[i]);
    std::fprintf(stderr, "%s\n", line);
    std::fflush(stderr);
}

}

extern "C" {

// Unresolved libSceAgcDriver import NID U9ueyEhSkF4; name unknown (not in any
// local firmware or SDK listing). Instrumented capture-only implementation.
APS5_EXPORT("U9ueyEhSkF4", sceAgcDriverUnknownU9ueyEhSkF4);
int APS5_VABI sceAgcDriverUnknownU9ueyEhSkF4(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, const void* a4, std::uint64_t a5, std::uint64_t a6) {
    if (unknownCallCount < 64) {
        ++unknownCallCount;
        std::fprintf(stderr, "[U9ueyEhSkF4] #%d a0=%llx a1=%llx a2=%llx a3=%llx a4=%p a5=%llx a6=%llx\n",
            unknownCallCount,
            static_cast<unsigned long long>(a0), static_cast<unsigned long long>(a1),
            static_cast<unsigned long long>(a2), static_cast<unsigned long long>(a3),
            a4, static_cast<unsigned long long>(a5), static_cast<unsigned long long>(a6));
        dumpWords("  a4[0..15]", a4, 16);
        std::fflush(stderr);
    }
    return 0;
}

}
