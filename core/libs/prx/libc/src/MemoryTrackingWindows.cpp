#include "prx/libc/include/MemoryTrackingPlatform.hpp"
#include <algorithm>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <system_error>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace GuestMemoryTracking::Platform {
namespace {

FaultHandler faultHandler = nullptr;

// Prints "module+offset" for an address inside a loaded module, or nothing useful otherwise.
void describeAddress(unsigned long long address, char* out, std::size_t size) {
    HMODULE module = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(address), &module) && module != nullptr) {
        char path[MAX_PATH] = {};
        GetModuleFileNameA(module, path, MAX_PATH);
        const char* name = path;
        for (const char* c = path; *c; ++c) if (*c == '\\' || *c == '/') name = c + 1;
        std::snprintf(out, size, "%s+0x%llx", name, address - static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(module)));
    } else {
        std::snprintf(out, size, "?");
    }
}

LONG CALLBACK handleException(EXCEPTION_POINTERS* exception) {
    const auto* record = exception->ExceptionRecord;
    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || record->NumberParameters < 2 || record->ExceptionInformation[0] > 1) return EXCEPTION_CONTINUE_SEARCH;
    try {
        LARGE_INTEGER faultStart{};
        QueryPerformanceCounter(&faultStart);
        if (faultHandler(record->ExceptionInformation[1], record->ExceptionInformation[0] == 1)) {
            static volatile LONG handled = 0;
            static volatile LONGLONG totalTicks = 0;
            LARGE_INTEGER faultEnd{};
            QueryPerformanceCounter(&faultEnd);
            InterlockedAdd64(&totalTicks, faultEnd.QuadPart - faultStart.QuadPart);
            const auto count = static_cast<unsigned long>(InterlockedIncrement(&handled));
            if (count % 500 == 0) {
                LARGE_INTEGER frequency{};
                QueryPerformanceFrequency(&frequency);
                std::fprintf(stderr, "[TRACKFAULT] handled=%lu total=%.1fms last=%s address=0x%llx\n", count, 1000.0 * static_cast<double>(totalTicks) / static_cast<double>(frequency.QuadPart), record->ExceptionInformation[0] == 1 ? "write" : "read", static_cast<unsigned long long>(record->ExceptionInformation[1]));
                std::fflush(stderr);
            }
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    } catch (...) {
        std::terminate();
    }
    // Nobody owns this fault: report where it happened once before the default handling ends the process.
    static volatile LONG reported = 0;
    if (InterlockedExchange(&reported, 1) == 0) {
        const auto* context = exception->ContextRecord;
        std::fprintf(stderr, "[FATAL-AV] unhandled access violation: %s address=0x%llx rip=0x%llx rsp=0x%llx rax=0x%llx rbx=0x%llx rcx=0x%llx rdx=0x%llx rdi=0x%llx rsi=0x%llx\n",
            record->ExceptionInformation[0] == 1 ? "write" : "read", static_cast<unsigned long long>(record->ExceptionInformation[1]),
            static_cast<unsigned long long>(context->Rip), static_cast<unsigned long long>(context->Rsp), static_cast<unsigned long long>(context->Rax),
            static_cast<unsigned long long>(context->Rbx), static_cast<unsigned long long>(context->Rcx), static_cast<unsigned long long>(context->Rdx),
            static_cast<unsigned long long>(context->Rdi), static_cast<unsigned long long>(context->Rsi));
        char where[300];
        describeAddress(context->Rip, where, sizeof(where));
        MEMORY_BASIC_INFORMATION page{};
        VirtualQuery(reinterpret_cast<const void*>(record->ExceptionInformation[1]), &page, sizeof(page));
        std::fprintf(stderr, "[FATAL-AV] rip in %s; fault page state=0x%lx protect=0x%lx regionBase=0x%llx regionSize=0x%llx\n", where, page.State, page.Protect, static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(page.BaseAddress)), static_cast<unsigned long long>(page.RegionSize));
        // Return-address candidates: stack words that point into loaded modules (stale words give false hints).
        const auto* stack = reinterpret_cast<const unsigned long long*>(context->Rsp);
        int shown = 0;
        for (int i = 0; i < 1024 && shown < 24; ++i) {
            unsigned long long value = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), stack + i, &value, sizeof(value), nullptr)) break;
            HMODULE module = nullptr;
            if (value > 0x10000 && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(value), &module) && module != nullptr) {
                describeAddress(value, where, sizeof(where));
                std::fprintf(stderr, "[FATAL-AV]   stack[%d] %s\n", i, where);
                ++shown;
            }
        }
        std::fflush(stderr);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void protect(std::uint64_t address, std::size_t bytes, DWORD protection) {
    DWORD previous = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), bytes, protection, &previous)) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "guest memory tracking VirtualProtect failed");
}

}

std::size_t PageSize() {
    static const auto size = [] {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        if (info.dwPageSize == 0) throw std::runtime_error("invalid native page size");
        return static_cast<std::size_t>(info.dwPageSize);
    }();
    return size;
}

void Install(FaultHandler handler) {
    if (handler == nullptr || faultHandler != nullptr) throw std::runtime_error("invalid guest memory fault handler installation");
    faultHandler = handler;
    if (AddVectoredExceptionHandler(1, handleException) == nullptr) {
        const auto error = GetLastError();
        faultHandler = nullptr;
        throw std::system_error(static_cast<int>(error), std::system_category(), "guest memory fault handler installation failed");
    }
}

std::vector<Region> Query(std::uint64_t address, std::size_t bytes) {
    std::vector<Region> regions;
    const auto end = address + bytes;
    while (address < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) != sizeof(info)) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "tracked guest memory query failed");
        if (info.State != MEM_COMMIT || info.Protect != PAGE_READWRITE) throw std::runtime_error("tracked render memory must be committed, writable and non-executable");
        const auto next = std::min(end, reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize);
        if (next <= address) throw std::runtime_error("invalid tracked guest memory mapping");
        regions.push_back({address, static_cast<std::size_t>(next - address), info.Protect});
        address = next;
    }
    return regions;
}

void Protect(std::uint64_t address, std::size_t bytes, Protection protection) {
    if (protection != Protection::None && protection != Protection::Read) throw std::invalid_argument("invalid tracked page protection");
    protect(address, bytes, protection == Protection::None ? PAGE_NOACCESS : PAGE_READONLY);
}

void Restore(const std::vector<Region>& regions) {
    for (const auto& region : regions) protect(region.address, region.bytes, static_cast<DWORD>(region.protection));
}

}
