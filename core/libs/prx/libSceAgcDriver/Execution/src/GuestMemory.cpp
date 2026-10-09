#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libc/include/VerboseLog.hpp"
#include "prx/libc/include/GuestMemoryTracking.hpp"
#include "prx/libSceAgcDriver/Execution/include/MemoryAccessScope.hpp"
#include "prx/libSceAgcDriver/Execution/include/PerformanceTimer.hpp"
#include <algorithm>
#include <array>
#include <mutex>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fstream>
#include <sstream>
#endif

namespace AgcDriver::GuestMemory {
namespace {
void require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(std::string("AGC driver: ") + reason);
}

#ifdef _WIN32
// VirtualQuery on the section-backed guest views costs milliseconds per call (it scales with the size of the
// surrounding region), which made every draw pay tens of milliseconds. Regions that already passed validation are
// remembered per mapping epoch; the epoch changes whenever guest memory is mapped, unmapped or re-protected by the
// guest. Tracker page toggles do not matter: the tracker restores access to the requested range before validation.
struct VerifiedRegion {
    std::uintptr_t begin = 0;
    std::uintptr_t end = 0;
    std::uint64_t epoch = 0;
    bool writable = false;
    bool valid = false;
};
std::mutex verifiedMutex;
std::array<VerifiedRegion, 32> verifiedRegions;
std::size_t verifiedNext = 0;

bool coveredByVerified(std::uintptr_t begin, std::uintptr_t end, bool writable, std::uint64_t epoch) {
    std::lock_guard lock(verifiedMutex);
    // A range may span several adjacent verified regions (for example a read that crosses a page boundary).
    auto cursor = begin;
    while (cursor < end) {
        bool advanced = false;
        for (const auto& region : verifiedRegions) {
            if (region.valid && region.epoch == epoch && region.begin <= cursor && cursor < region.end && (!writable || region.writable)) {
                cursor = region.end;
                advanced = true;
                break;
            }
        }
        if (!advanced) return false;
    }
    return true;
}

void rememberVerified(std::uintptr_t begin, std::uintptr_t end, bool writable, std::uint64_t epoch) {
    std::lock_guard lock(verifiedMutex);
    for (auto& region : verifiedRegions) {
        if (region.valid && region.epoch == epoch && region.begin == begin && region.end == end) {
            region.writable = region.writable || writable;
            return;
        }
    }
    verifiedRegions[verifiedNext] = {begin, end, epoch, writable, true};
    verifiedNext = (verifiedNext + 1) % verifiedRegions.size();
}

#endif
}

void CheckRange(const void* pointer, std::size_t bytes, std::size_t alignment, bool writable) {
    require(alignment != 0, "zero guest memory alignment");
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    StallGuard stall("GuestMemory::CheckRange", writable ? "write" : "read", reinterpret_cast<std::uintptr_t>(pointer), bytes);
    require(address != 0 && address % alignment == 0, "null or misaligned address");
    require(bytes <= std::numeric_limits<std::uintptr_t>::max() - address, "address range overflow");
    MemoryAccessScope::Resolve(address, bytes, writable);
    GuestMemoryTracking::GuestMemoryTrackingResolve_nid_postfix(address, bytes, writable);
    auto cursor = address;
    const auto end = address + bytes;
#ifdef _WIN32
    const auto epoch = GuestMemoryTracking::GuestMemoryMappingEpoch_nid_postfix(); // read before querying: a concurrent change makes the entry stale
    const bool cached = coveredByVerified(address, end, writable, epoch);
    if (cached) return;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION memory{};
        require(VirtualQuery(reinterpret_cast<const void*>(cursor), &memory, sizeof(memory)) == sizeof(memory), "cannot query guest memory");
        require(memory.State == MEM_COMMIT && (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0, "guest memory is not readable");
        const auto protection = memory.Protect & 0xffu;
        require(protection == PAGE_READONLY || protection == PAGE_READWRITE || protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY, "guest memory has no read permission");
        const auto base = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        require(memory.RegionSize <= std::numeric_limits<std::uintptr_t>::max() - base && base + memory.RegionSize > cursor, "invalid guest memory mapping");
        require(!writable || protection == PAGE_READWRITE || protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY, "guest memory has no write permission");
        rememberVerified(base, base + memory.RegionSize, protection == PAGE_READWRITE || protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY, epoch);
        cursor = std::min(end, base + memory.RegionSize);
    }
#else
    std::ifstream maps("/proc/self/maps");
    require(maps.is_open(), "cannot query guest memory maps");
    std::string line;
    while (cursor < end && std::getline(maps, line)) {
        std::istringstream fields(line);
        std::uintptr_t first = 0;
        std::uintptr_t last = 0;
        char separator = 0;
        std::string permissions;
        require(static_cast<bool>(fields >> std::hex >> first >> separator >> last >> permissions) && separator == '-' && first < last && !permissions.empty(), "invalid guest memory map entry");
        if (last <= cursor) continue;
        require(first <= cursor && permissions[0] == 'r', "guest memory is not readable");
        require(!writable || (permissions.size() > 1 && permissions[1] == 'w'), "guest memory has no write permission");
        cursor = std::min(end, last);
    }
    require(cursor == end, "guest address range is not mapped");
#endif
}

void Read(std::uint64_t address, std::span<std::byte> destination, std::size_t alignment) {
    PerformanceTimer timing("GuestMemory.Read");
    if (destination.empty()) return;
    const auto* source = reinterpret_cast<const void*>(address);
    CheckRange(source, destination.size(), alignment);
    timing.Mark("range_check");
    std::memcpy(destination.data(), source, destination.size());
    timing.Mark("copy", destination.size());
    CountEvent(FrameTiming::ReadBytes, destination.size());
}

void Write(std::uint64_t address, std::span<const std::byte> source, std::size_t alignment) {
    PerformanceTimer timing("GuestMemory.Write");
    if (source.empty()) return;
    auto* destination = reinterpret_cast<void*>(address);
    CheckRange(destination, source.size(), alignment, true);
    timing.Mark("range_check");
    std::memcpy(destination, source.data(), source.size());
    timing.Mark("copy", source.size());
    CountEvent(FrameTiming::WriteBytes, source.size());
}

}

extern "C" void AgcDriverCheckGuestMemory_nid_postfix(const void* pointer, std::size_t bytes, std::size_t alignment, bool writable) {
    AgcDriver::GuestMemory::CheckRange(pointer, bytes, alignment, writable);
}
