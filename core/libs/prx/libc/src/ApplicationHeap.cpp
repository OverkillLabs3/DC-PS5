#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>

namespace {

using Allocate = void* (APS5_VABI *)(std::size_t);
using Free = void (APS5_VABI *)(void*);
using Reallocate = void* (APS5_VABI *)(void*, std::size_t);
using Calloc = void* (APS5_VABI *)(std::size_t, std::size_t);
using Align = void* (APS5_VABI *)(std::size_t, std::size_t);
using PosixAlign = int (APS5_VABI *)(void**, std::size_t, std::size_t);
using Initialize = void (APS5_VABI *)();

std::mutex heapMutex;
std::array<void*, 10> heapApi{};
std::once_flag heapInitialization;
std::exception_ptr heapFailure;
Initialize heapFinalize = nullptr;
bool heapFinalized = false;
thread_local bool heapCallbackActive = false;

class CallbackScope {
public:
    CallbackScope() {
        if (heapCallbackActive) throw std::runtime_error("application heap: recursive libc allocator callback");
        heapCallbackActive = true;
    }
    ~CallbackScope() { heapCallbackActive = false; }
    CallbackScope(const CallbackScope&) = delete;
    CallbackScope& operator=(const CallbackScope&) = delete;
};

void* requireAllocation(void* pointer) {
    if (pointer == nullptr) throw std::bad_alloc();
    return pointer;
}

void requireAlignment(std::size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) throw std::invalid_argument("application heap: invalid alignment");
}

// The PS5 kernel resolves the replacement table function slots against the application's own libc at load time; retail packages ship the table zeroed and a relinked Windows image has no such resolver. A fully or partially empty table therefore means "no external allocator": fall back to the C runtime, matching the internal default libSceLibcInternal uses when no replacement is provided. Every block handed to the guest goes through the same aligned family so that the free slot accepts them all.
// These defaults are invoked through the APS5_VABI function pointers above, so they must carry the same SysV calling convention; under MinGW a plain function defaults to the Microsoft ABI and would receive its arguments in the wrong registers.
#ifdef _WIN32
#include <malloc.h>

void* APS5_VABI hostAllocate(std::size_t bytes) { return _aligned_malloc(bytes != 0 ? bytes : 1, 16); }
void APS5_VABI hostFree(void* pointer) { _aligned_free(pointer); }
void* APS5_VABI hostReallocate(void* pointer, std::size_t bytes) { return _aligned_realloc(pointer, bytes != 0 ? bytes : 1, 16); }
void* APS5_VABI hostAlign(std::size_t alignment, std::size_t bytes) { return _aligned_malloc(bytes != 0 ? bytes : 1, alignment); }
#else

void* hostAllocate(std::size_t bytes) { return std::malloc(bytes != 0 ? bytes : 1); }
void hostFree(void* pointer) { std::free(pointer); }
void* hostReallocate(void* pointer, std::size_t bytes) { return std::realloc(pointer, bytes != 0 ? bytes : 1); }
void* hostAlign(std::size_t alignment, std::size_t bytes) { return std::aligned_alloc(alignment, (bytes == 0 ? alignment : bytes + alignment - 1) & ~(alignment - 1)); }

#endif

void* APS5_VABI defaultAllocate(std::size_t bytes) { return hostAllocate(bytes); }
void APS5_VABI defaultFree(void* pointer) { hostFree(pointer); }
void* APS5_VABI defaultReallocate(void* pointer, std::size_t bytes) { return pointer == nullptr ? hostAllocate(bytes) : hostReallocate(pointer, bytes); }
void* APS5_VABI defaultCalloc(std::size_t count, std::size_t bytes) {
    const auto overflow = bytes != 0 && count > std::numeric_limits<std::size_t>::max() / bytes;
    if (overflow) return nullptr;
    const auto total = count * bytes;
    const auto pointer = hostAllocate(total);
    if (pointer != nullptr) std::memset(pointer, 0, total);
    return pointer;
}
int APS5_VABI defaultPosixAlign(void** pointer, std::size_t alignment, std::size_t bytes) {
    const auto result = hostAlign(alignment, bytes);
    if (result == nullptr) return 12;
    *pointer = result;
    return 0;
}
void* APS5_VABI defaultMallocR(void*, std::size_t bytes) { return hostAllocate(bytes); }
void APS5_VABI defaultFreeR(void*, void* pointer) { hostFree(pointer); }
void* APS5_VABI defaultReallocR(void*, void* pointer, std::size_t bytes) { return defaultReallocate(pointer, bytes); }

const std::array<void*, 10> hostApi = {reinterpret_cast<void*>(&defaultAllocate), reinterpret_cast<void*>(&defaultFree), reinterpret_cast<void*>(&defaultCalloc), reinterpret_cast<void*>(&defaultReallocate), reinterpret_cast<void*>(&hostAlign), reinterpret_cast<void*>(&defaultFree), reinterpret_cast<void*>(&defaultPosixAlign), reinterpret_cast<void*>(&defaultMallocR), reinterpret_cast<void*>(&defaultFreeR), reinterpret_cast<void*>(&defaultReallocR)};

template<typename TCallback>
TCallback registeredCallback(std::size_t index) {
    std::lock_guard lock(heapMutex);
    if (heapFailure) std::rethrow_exception(heapFailure);
    if (heapFinalized) throw std::runtime_error("application heap: allocator has been finalized");
    const auto pointer = heapApi[index] != nullptr ? heapApi[index] : hostApi[index];
    static_assert(sizeof(TCallback) == sizeof(void*));
    TCallback result;
    std::memcpy(&result, &pointer, sizeof(result));
    return result;
}

void finalize() {
    Initialize finalizeCallback;
    {
        std::lock_guard lock(heapMutex);
        if (heapFailure) std::rethrow_exception(heapFailure);
        if (heapFinalized) throw std::runtime_error("application heap: duplicate finalization");
        finalizeCallback = heapFinalize;
    }
    if (finalizeCallback != nullptr) finalizeCallback();
    std::lock_guard lock(heapMutex);
    heapFinalized = true;
}

template<typename TValue>
TValue read(const void* pointer, std::size_t offset) {
    if (pointer == nullptr) throw std::invalid_argument("application heap: null metadata");
    TValue value;
    std::memcpy(&value, static_cast<const std::byte*>(pointer) + offset, sizeof(value));
    return value;
}

}

void ApplicationHeapRegister_nid_no_patch(void* const* api) {
    if (api == nullptr) throw std::invalid_argument("application heap: null allocator API");
    std::array<void*, 10> replacement;
    std::memcpy(replacement.data(), api, sizeof(replacement));
    for (std::size_t index = 0; index < replacement.size(); ++index) {
        if (replacement[index] == nullptr) replacement[index] = hostApi[index];
    }
    std::lock_guard lock(heapMutex);
    if (heapFailure) std::rethrow_exception(heapFailure);
    if (heapFinalized) throw std::runtime_error("application heap: allocator has been finalized");
    if (heapApi[0] != nullptr && heapApi != replacement) throw std::runtime_error("application heap: cannot replace an active allocator");
    heapApi = replacement;
}

void ApplicationHeapInitialize_nid_no_patch(const void* processParameters) {
    std::call_once(heapInitialization, [processParameters] {
        try {
            if (read<std::uint64_t>(processParameters, 0) < 0x40 || read<std::uint32_t>(processParameters, 8) != 0x4942524f) throw std::runtime_error("application heap: invalid process parameters");
            const auto* libcParameters = read<const void*>(processParameters, 0x38);
            if (read<std::uint64_t>(libcParameters, 0) < 0x38) throw std::runtime_error("application heap: invalid libc parameters");
            const auto* replacement = read<const void*>(libcParameters, 0x30);
            if (read<std::uint64_t>(replacement, 0) != 0x78 || read<std::uint64_t>(replacement, 8) != 2) throw std::runtime_error("application heap: unsupported allocator replacement table");
            std::array<void*, 10> api;
            std::memcpy(api.data(), static_cast<const std::byte*>(replacement) + 0x20, sizeof(api));
            ApplicationHeapRegister_nid_no_patch(api.data());
            const auto initialize = read<Initialize>(replacement, 0x10);
            if (initialize != nullptr) initialize();
            {
                std::lock_guard lock(heapMutex);
                heapFinalize = read<Initialize>(replacement, 0x18);
            }
            if (std::atexit(finalize) != 0) throw std::runtime_error("application heap: cannot register finalization");
        } catch (...) {
            std::lock_guard lock(heapMutex);
            heapFailure = std::current_exception();
        }
    });
    std::lock_guard lock(heapMutex);
    if (heapFailure) std::rethrow_exception(heapFailure);
}

void* ApplicationHeapAllocate_nid_no_patch(std::size_t bytes) {
    const auto allocate = registeredCallback<Allocate>(0);
    CallbackScope scope;
    return requireAllocation(allocate(bytes));
}

void ApplicationHeapFree_nid_no_patch(void* pointer) {
    if (pointer == nullptr) return;
    const auto free = registeredCallback<Free>(1);
    CallbackScope scope;
    free(pointer);
}

void* ApplicationHeapReallocate_nid_no_patch(void* pointer, std::size_t bytes) {
    if (bytes == 0) {
        ApplicationHeapFree_nid_no_patch(pointer);
        return nullptr;
    }
    const auto reallocate = registeredCallback<Reallocate>(3);
    CallbackScope scope;
    return requireAllocation(reallocate(pointer, bytes));
}

void* ApplicationHeapAlign_nid_no_patch(std::size_t alignment, std::size_t bytes) {
    requireAlignment(alignment);
    const auto align = registeredCallback<Align>(4);
    CallbackScope scope;
    void* pointer = requireAllocation(align(alignment, bytes));
    if (reinterpret_cast<std::uintptr_t>(pointer) % alignment != 0) throw std::runtime_error("application heap: allocator returned a misaligned pointer");
    return pointer;
}

void* ApplicationHeapCalloc_nid_no_patch(std::size_t count, std::size_t bytes) {
    if (bytes != 0 && count > std::numeric_limits<std::size_t>::max() / bytes) throw std::length_error("application heap: calloc size overflow");
    const auto calloc = registeredCallback<Calloc>(2);
    CallbackScope scope;
    return requireAllocation(calloc(count, bytes));
}

int ApplicationHeapPosixAlign_nid_no_patch(void** pointer, std::size_t alignment, std::size_t bytes) {
    if (pointer == nullptr) throw std::invalid_argument("application heap: null allocation output");
    requireAlignment(alignment);
    if (alignment < sizeof(void*)) throw std::invalid_argument("application heap: invalid POSIX alignment");
    const auto align = registeredCallback<PosixAlign>(6);
    CallbackScope scope;
    void* result = nullptr;
    if (align(&result, alignment, bytes) != 0) throw std::runtime_error("application heap: posix_memalign failed");
    requireAllocation(result);
    if (reinterpret_cast<std::uintptr_t>(result) % alignment != 0) throw std::runtime_error("application heap: allocator returned a misaligned pointer");
    *pointer = result;
    return 0;
}
