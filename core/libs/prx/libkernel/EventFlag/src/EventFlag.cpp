#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <mutex>
#include <string>
#include <utility>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

constexpr int KERNEL_EFLAG_OK = 0;
constexpr int KERNEL_EFLAG_ERROR_EPERM = static_cast<int>(0x80020001);
constexpr int KERNEL_EFLAG_ERROR_EBUSY = static_cast<int>(0x80020010);
constexpr int KERNEL_EFLAG_ERROR_EINVAL = static_cast<int>(0x80020016);
constexpr int KERNEL_EFLAG_ERROR_ETIMEDOUT = static_cast<int>(0x8002003C);
constexpr int KERNEL_EFLAG_ERROR_ECANCELED = static_cast<int>(0x80020055);

// SDK wait modes and event flag attributes.
constexpr std::uint32_t EVF_WAITMODE_AND = 0x01u;
constexpr std::uint32_t EVF_WAITMODE_OR = 0x02u;
constexpr std::uint32_t EVF_WAITMODE_CLEAR_ALL = 0x10u;
constexpr std::uint32_t EVF_WAITMODE_CLEAR_PAT = 0x20u;
constexpr std::uint32_t EVF_ATTR_SINGLE = 0x10u;
constexpr std::uint32_t EVF_ATTR_MASK = 0x33u;

struct KernelEventFlagPrivate {
    KernelEventFlagPrivate(std::string name, std::uint64_t pattern, bool single)
     : name(std::move(name)), pattern(pattern), single(single) {}

    bool satisfied(std::uint64_t bits, std::uint32_t mode) const {
        return (mode & EVF_WAITMODE_OR) != 0 ? (pattern & bits) != 0 : (pattern & bits) == bits;
    }

    // Applies the wake-time clear requested by the wait mode. Caller holds the mutex.
    void clearOnWake(std::uint64_t bits, std::uint32_t mode) {
        if ((mode & EVF_WAITMODE_CLEAR_ALL) != 0) pattern = 0;
        else if ((mode & EVF_WAITMODE_CLEAR_PAT) != 0) pattern &= ~bits;
    }

    std::mutex mutex;
    std::condition_variable condition;
    std::string name;
    std::uint64_t pattern;
    bool single;
    bool deleted = false;
    std::uint32_t waiters = 0;
    std::uint64_t cancelGeneration = 0;
};

namespace {
bool validWaitMode(std::uint32_t mode) {
    const bool both = (mode & EVF_WAITMODE_AND) != 0 && (mode & EVF_WAITMODE_OR) != 0;
    const bool neither = (mode & (EVF_WAITMODE_AND | EVF_WAITMODE_OR)) == 0;
    const bool bothClears = (mode & EVF_WAITMODE_CLEAR_ALL) != 0 && (mode & EVF_WAITMODE_CLEAR_PAT) != 0;
    return !both && !neither && !bothClears && (mode & ~(EVF_WAITMODE_AND | EVF_WAITMODE_OR | EVF_WAITMODE_CLEAR_ALL | EVF_WAITMODE_CLEAR_PAT)) == 0;
}
}  // namespace

extern "C" {

int APS5_VABI sceKernelCreateEventFlag(KernelEventFlag* ef, const char* name, uint32_t attr, uint64_t init_pattern, const void* param) {
    (void)param;
    if (ef == nullptr || (attr & ~EVF_ATTR_MASK) != 0) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    *ef = new KernelEventFlagPrivate(name != nullptr ? name : "", init_pattern, (attr & EVF_ATTR_SINGLE) != 0);
    return KERNEL_EFLAG_OK;
}

int APS5_VABI sceKernelDeleteEventFlag(KernelEventFlag ef) {
    if (ef == nullptr) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    {
        // Blocked waiters are woken with ECANCELED and must leave before the object is freed.
        std::unique_lock lock(ef->mutex);
        ef->deleted = true;
        ef->condition.notify_all();
        ef->condition.wait(lock, [&] { return ef->waiters == 0; });
    }
    delete ef;
    return KERNEL_EFLAG_OK;
}

int APS5_VABI sceKernelSetEventFlag(KernelEventFlag ef, uint64_t bit_pattern) {
    if (ef == nullptr) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    {
        std::lock_guard lock(ef->mutex);
        ef->pattern |= bit_pattern;
    }
    ef->condition.notify_all();
    return KERNEL_EFLAG_OK;
}

// SDK semantics: bit_pattern is a keep-mask, so bits that are 0 in it are cleared (games pass ~bits).
int APS5_VABI sceKernelClearEventFlag(KernelEventFlag ef, uint64_t bit_pattern) {
    if (ef == nullptr) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    std::lock_guard lock(ef->mutex);
    ef->pattern &= bit_pattern;
    return KERNEL_EFLAG_OK;
}

// Wakes every waiter with ECANCELED and replaces the pattern with set_pattern.
int APS5_VABI sceKernelCancelEventFlag(KernelEventFlag ef, uint64_t set_pattern, int* num_wait_threads) {
    if (ef == nullptr) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    {
        std::lock_guard lock(ef->mutex);
        if (num_wait_threads != nullptr) {
            *num_wait_threads = static_cast<int>(ef->waiters);
        }
        ef->pattern = set_pattern;
        ++ef->cancelGeneration;
    }
    ef->condition.notify_all();
    return KERNEL_EFLAG_OK;
}

int APS5_VABI sceKernelPollEventFlag(KernelEventFlag ef, uint64_t bit_pattern, uint32_t wait_mode, uint64_t* result_pat) {
    if (ef == nullptr || bit_pattern == 0 || !validWaitMode(wait_mode)) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    std::lock_guard lock(ef->mutex);
    if (!ef->satisfied(bit_pattern, wait_mode)) {
        if (result_pat != nullptr) {
            *result_pat = ef->pattern;
        }
        return KERNEL_EFLAG_ERROR_EBUSY;
    }
    if (result_pat != nullptr) {
        *result_pat = ef->pattern;
    }
    ef->clearOnWake(bit_pattern, wait_mode);
    return KERNEL_EFLAG_OK;
}

int APS5_VABI sceKernelWaitEventFlag(KernelEventFlag ef, uint64_t bit_pattern, uint32_t wait_mode, uint64_t* result_pat, KernelUseconds* timeout) {
    if (ef == nullptr || bit_pattern == 0 || !validWaitMode(wait_mode)) {
        return KERNEL_EFLAG_ERROR_EINVAL;
    }
    std::unique_lock lock(ef->mutex);
    if (ef->deleted) {
        return KERNEL_EFLAG_ERROR_ECANCELED;
    }
    if (ef->single && ef->waiters != 0) {
        return KERNEL_EFLAG_ERROR_EPERM;
    }
    const auto generation = ef->cancelGeneration;
    const auto ready = [&] { return ef->deleted || ef->cancelGeneration != generation || ef->satisfied(bit_pattern, wait_mode); };
    const auto start = std::chrono::steady_clock::now();
    ++ef->waiters;
    bool woke = true;
    if (timeout == nullptr) {
        ef->condition.wait(lock, ready);
    } else {
        woke = ef->condition.wait_for(lock, std::chrono::microseconds(*timeout), ready);
    }
    int result = KERNEL_EFLAG_OK;
    if (ef->deleted || ef->cancelGeneration != generation) {
        result = KERNEL_EFLAG_ERROR_ECANCELED;
    } else if (!woke) {
        result = KERNEL_EFLAG_ERROR_ETIMEDOUT;
    }
    if (result_pat != nullptr) {
        *result_pat = ef->pattern;
    }
    if (result == KERNEL_EFLAG_OK) {
        ef->clearOnWake(bit_pattern, wait_mode);
    }
    if (timeout != nullptr) {
        // The timeout is in/out: the remaining time is written back (zero on expiry).
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
        *timeout = result == KERNEL_EFLAG_ERROR_ETIMEDOUT || static_cast<std::uint64_t>(elapsed) >= *timeout ? 0u : static_cast<KernelUseconds>(*timeout - static_cast<std::uint64_t>(elapsed));
    }
    --ef->waiters;
    if (ef->deleted && ef->waiters == 0) {
        ef->condition.notify_all();
    }
    return result;
}

}
