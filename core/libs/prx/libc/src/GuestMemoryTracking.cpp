#include "prx/libc/include/GuestMemoryTracking.hpp"
#include "prx/libc/include/VerboseLog.hpp"
#include "prx/libc/include/MemoryTrackingPlatform.hpp"
#include "prx/libc/include/GuestMemoryBacking.hpp"
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <exception>
#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

namespace GuestMemoryTracking {
namespace {

struct Entry {
    std::uint64_t address;
    std::size_t bytes;
    void* context;
    Resolver resolver;
    Protection protection = Protection::ReadWrite;
    std::vector<Platform::Region> original;
    bool resolving = false;
    bool active = false;
};

struct Registry {
    std::recursive_mutex mutex;
    std::map<std::uint64_t, std::shared_ptr<Entry>> entries;
    bool installed = false;
};

Registry& registry() {
    static auto* value = new Registry;
    return *value;
}

std::atomic<std::uint64_t>& mappingEpoch() {
    static std::atomic<std::uint64_t> value{0};
    return value;
}

std::uint64_t checkedEnd(std::uint64_t address, std::size_t bytes) {
    if (address == 0 || bytes == 0 || bytes > std::numeric_limits<std::uint64_t>::max() - address) throw std::invalid_argument("invalid tracked guest memory range");
    return address + bytes;
}

void resolve(const std::shared_ptr<Entry>& entry, Access access) {
    if (entry->resolving) throw std::runtime_error("recursive guest memory ownership resolution");
    entry->resolving = true;
    try {
        entry->resolver(entry->context, access);
        if (entry->protection == Protection::None || (access != Access::Read && entry->protection != Protection::ReadWrite)) throw std::runtime_error("guest memory resolver did not release CPU access");
        entry->resolving = false;
    } catch (...) {
        entry->resolving = false;
        throw;
    }
}

std::vector<std::shared_ptr<Entry>> overlapping(std::uint64_t address, std::size_t bytes) {
    const auto end = checkedEnd(address, bytes);
    auto& entries = registry().entries;
    auto first = entries.upper_bound(address);
    if (first != entries.begin()) --first;
    std::vector<std::shared_ptr<Entry>> result;
    for (auto it = first; it != entries.end() && it->first < end; ++it) {
        if (it->first + it->second->bytes > address) result.push_back(it->second);
    }
    return result;
}

bool fault(std::uint64_t address, bool writable) {
    // A null or wrapping address can never belong to a tracked range: it is an ordinary guest fault, not ours.
    if (address == 0 || address == std::numeric_limits<std::uint64_t>::max()) return false;
    std::lock_guard lock(registry().mutex);
    const auto entries = overlapping(address, 1);
    if (entries.empty()) return false;
    bool handled = false;
    for (const auto& entry : entries) {
        if (!entry->active) continue;
        if (entry->protection == Protection::None || (writable && entry->protection == Protection::Read)) resolve(entry, writable ? Access::Write : Access::Read);
        handled = true;
    }
    return handled;
}

}

std::recursive_mutex& GuestMemoryTrackingMutex_nid_postfix() {
    return registry().mutex;
}

std::uint64_t GuestMemoryMappingEpoch_nid_postfix() {
    return mappingEpoch().load(std::memory_order_acquire);
}

void GuestMemoryMappingChanged_nid_postfix() {
    mappingEpoch().fetch_add(1, std::memory_order_acq_rel);
}

std::size_t GuestMemoryTrackingPageSize_nid_postfix() {
    return Platform::PageSize();
}

void* GuestMemoryTrackingCreate_nid_postfix(std::uint64_t address, std::size_t bytes, void* context, Resolver resolver) {
    const auto end = checkedEnd(address, bytes);
    const auto pageSize = Platform::PageSize();
    if (context == nullptr || resolver == nullptr) throw std::invalid_argument("missing guest memory ownership resolver");
    if (end > std::numeric_limits<std::uint64_t>::max() - (pageSize - 1)) throw std::overflow_error("tracked guest memory page range overflow");
    const auto first = address - address % pageSize;
    const auto last = (end + pageSize - 1) / pageSize * pageSize;
    std::lock_guard lock(registry().mutex);
    GuestMemoryBacking::GuestMemoryBackingRequire_nid_postfix(first, static_cast<std::size_t>(last - first));
    if (!overlapping(first, static_cast<std::size_t>(last - first)).empty()) throw std::runtime_error("overlapping guest memory ownership pages");
    static_cast<void>(Platform::Query(first, static_cast<std::size_t>(last - first)));
    if (!registry().installed) {
        Platform::Install(fault);
        registry().installed = true;
    }
    auto entry = std::make_shared<Entry>();
    entry->address = first;
    entry->bytes = static_cast<std::size_t>(last - first);
    entry->context = context;
    entry->resolver = resolver;
    auto handle = std::make_unique<std::shared_ptr<Entry>>(entry);
    registry().entries.emplace(first, std::move(entry));
    return handle.release();
}

void GuestMemoryTrackingDestroy_nid_postfix(void* handle) noexcept {
    if (handle == nullptr) std::terminate();
    std::lock_guard lock(registry().mutex);
    std::unique_ptr<std::shared_ptr<Entry>> owner(static_cast<std::shared_ptr<Entry>*>(handle));
    const auto& entry = **owner;
    if (entry.protection != Protection::ReadWrite) Platform::Restore(entry.original);
    registry().entries.erase(entry.address);
}

void GuestMemoryTrackingProtect_nid_postfix(void* handle, Protection protection) {
    if (handle == nullptr) throw std::invalid_argument("missing guest memory watch");
    // TEMPORARY MEASUREMENT: where a slow protection change spends its time (lock wait, query, VirtualProtect).
    const auto clock = [] { return std::chrono::steady_clock::now(); };
    const auto t0 = clock();
    std::lock_guard lock(registry().mutex);
    const auto t1 = clock();
    auto& entry = **static_cast<std::shared_ptr<Entry>*>(handle);
    if (entry.protection == protection) return;
    auto t2 = t1;
    if (protection == Protection::ReadWrite) {
        Platform::Restore(entry.original);
        // The original protection is kept: walking the range with VirtualQuery again on every re-arm costs milliseconds for a render target
        // while the VirtualProtect itself takes microseconds. The entry is destroyed, not reused, if the mapping changes.
        t2 = clock();
    } else {
        if (entry.original.empty()) entry.original = Platform::Query(entry.address, entry.bytes);
        t2 = clock();
        Platform::Protect(entry.address, entry.bytes, protection);
        entry.active = true;
    }
    entry.protection = protection;
    const auto t3 = clock();
    const auto us = [](auto from, auto to) { return static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(to - from).count()); };
    static std::atomic<int> slowLogged{0};
    if (us(t0, t3) > 300 && slowLogged.fetch_add(1) < 60) {
        if (VerboseLog()) std::fprintf(stderr, "[SLOWPROT] to=%d bytes=%zu lock=%lldus query_or_restore=%lldus protect=%lldus\n", static_cast<int>(protection), entry.bytes, us(t0, t1), us(t1, t2), us(t2, t3));
    }
}

void GuestMemoryTrackingResolve_nid_postfix(std::uint64_t address, std::size_t bytes, bool writable) {
    if (bytes == 0) return;
    std::lock_guard lock(registry().mutex);
    {
        // Runs on every guest read and write: decide without allocating whether any watched range needs resolving at all.
        const auto end = checkedEnd(address, bytes);
        auto& entries = registry().entries;
        auto it = entries.upper_bound(address);
        if (it != entries.begin()) --it;
        bool needed = false;
        for (; it != entries.end() && it->first < end; ++it) {
            const auto& entry = *it->second;
            if (it->first + entry.bytes > address && (entry.protection == Protection::None || (writable && entry.protection == Protection::Read))) {
                needed = true;
                break;
            }
        }
        if (!needed) return;
    }
    for (const auto& entry : overlapping(address, bytes)) {
        if (entry->protection == Protection::None || (writable && entry->protection == Protection::Read)) resolve(entry, writable ? Access::Write : Access::Read);
    }
}

void GuestMemoryTrackingInvalidate_nid_postfix(std::uint64_t address, std::size_t bytes) {
    if (bytes == 0) return;
    std::lock_guard lock(registry().mutex);
    for (const auto& entry : overlapping(address, bytes)) {
        resolve(entry, Access::Invalidate);
        entry->active = false;
        entry->original.clear(); // the mapping is changing: the saved native permissions no longer describe it
    }
}

void GuestMemoryTrackingValidate_nid_postfix(std::uint64_t address, std::size_t bytes, const std::function<void(std::uint64_t, std::size_t)>& validate) {
    if (bytes == 0) return;
    if (!validate) throw std::invalid_argument("missing native memory range validator");
    const auto end = checkedEnd(address, bytes);
    std::lock_guard lock(registry().mutex);
    for (const auto& entry : overlapping(address, bytes)) {
        if (address < entry->address) validate(address, static_cast<std::size_t>(entry->address - address));
        const auto first = std::max(address, entry->address);
        const auto last = std::min(end, entry->address + entry->bytes);
        if (entry->protection == Protection::ReadWrite) validate(first, static_cast<std::size_t>(last - first));
        else if (entry->original.empty()) throw std::runtime_error("protected guest memory has no native permission record");
        address = last;
    }
    if (address < end) validate(address, static_cast<std::size_t>(end - address));
}

}
