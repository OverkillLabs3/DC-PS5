#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_TEXTURECACHE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_TEXTURECACHE_HPP

#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include <array>
#include <list>
#include <memory>
#include <vector>

namespace AgcDriver::Graphics {

class TextureCache {
public:
    explicit TextureCache(const Context& context);
    std::shared_ptr<Texture> Get(std::span<const std::uint32_t> words, const GuestTextureResource& resource, VkComponentMapping components);
    // Guest-memory textures are compared against their snapshot at most once per validation epoch. The driver advances the
    // epoch for every new submission and after every packet that can write guest memory, so a texture is never reused across a write.
    void SetValidationEpoch(std::uint64_t value) { epoch = value; }
    // Render targets of the draw being built: sampling one of them cannot alias its image (feedback loop), so it is copied.
    void SetActiveTargets(std::vector<const ResidentColor*> targets) { activeTargets = std::move(targets); }

private:
    struct Entry {
        std::array<std::uint32_t, 8> descriptor;
        std::vector<std::byte> snapshot;
        std::shared_ptr<Texture> texture;
        std::weak_ptr<ResidentColor> source;
        std::uint64_t generation = 0;
        std::uint64_t validatedEpoch = 0;
        // Adaptive backoff: a texture that keeps validating as unchanged is only re-compared every 2^stable epochs (capped).
        std::uint64_t skipUntilEpoch = 0;
        std::uint32_t stable = 0;
    };
    void trim();
    Context context;
    std::list<Entry> entries;
    std::uint64_t retainedBytes = 0;
    std::uint64_t epoch = 1;
    std::vector<const ResidentColor*> activeTargets;
    std::vector<std::shared_ptr<Texture>> uploading;
    static constexpr std::uint64_t budget = 2048ull * 1024 * 1024; // CPU snapshot + GPU copy of every cached guest texture
};

}

#endif
