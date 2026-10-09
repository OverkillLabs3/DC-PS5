#include "prx/libSceAgcDriver/Graphics/include/TextureCache.hpp"
#include "prx/libSceAgcDriver/Execution/include/PerformanceTimer.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureTiling.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/RenderCache.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureFormat.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

namespace AgcDriver::Graphics {
namespace {
std::uint64_t guestCreates = 0, changed = 0, evictions = 0;
}

TextureCache::TextureCache(const Context& context) : context(context) {
    Require(context.detiler != nullptr, "texture cache requires a device detiler");
}

void TextureCache::trim() {
    for (auto it = entries.begin(); it != entries.end() && (retainedBytes > budget || entries.size() > 64);) {
        if (it->texture.use_count() != 1) {
            ++it;
            continue;
        }
        retainedBytes -= it->snapshot.size() + it->texture->AllocationBytes();
        ++evictions;
        it = entries.erase(it);
    }
}

std::shared_ptr<Texture> TextureCache::Get(std::span<const std::uint32_t> words, const GuestTextureResource& resource, VkComponentMapping components) {
    Require(words.size() == 8, "texture cache descriptor must contain eight DWORDs");
    // Retire finished asynchronous uploads; once none is in flight the detiler can safely reuse its descriptor pools.
    uploading.erase(std::remove_if(uploading.begin(), uploading.end(), [](const std::shared_ptr<Texture>& texture) { return !texture->UploadPending(); }), uploading.end());
    if (uploading.empty()) context.detiler->Recycle();
    trim();
    std::array<std::uint32_t, 8> key;
    std::copy(words.begin(), words.end(), key.begin());
    auto source = context.renderCache ? context.renderCache->Find(resource.baseAddress) : nullptr;
    // TEMPORARY MEASUREMENT: why a render-target-tiled texture failed to alias a resident render target.
    static int aliasDiag = 0;
    if (resource.tileMode == TextureTileMode::RenderTarget64KB && aliasDiag < 40) {
        if (!source) {
            ++aliasDiag;
            std::fprintf(stderr, "[ALIAS] no resident target for addr=%llx %ux%u fmt=%u cache:%s\n", static_cast<unsigned long long>(resource.baseAddress), resource.width, resource.height, resource.format, context.renderCache ? context.renderCache->DebugDump(resource.baseAddress).c_str() : "none");
        } else {
            const auto& c = source->Description();
            if (resource.width != c.extent.width || resource.height != c.extent.height || resource.mipCount != 1 || BytesPerElement(resource.format) != (c.gpuOnly ? 8u : 4u) || c.tileMode != ColorTileMode::RenderTarget) {
                ++aliasDiag;
                std::fprintf(stderr, "[ALIAS] mismatch addr=%llx tex=%ux%u fmt=%u mips=%u rt=%ux%u gpuOnly=%d tile=%u\n", static_cast<unsigned long long>(resource.baseAddress), resource.width, resource.height, resource.format, resource.mipCount, c.extent.width, c.extent.height, static_cast<int>(c.gpuOnly), static_cast<unsigned>(c.tileMode));
            }
        }
    }
    if (source) {
        const auto& color = source->Description();
        const auto compatibleTiling = (color.tileMode == ColorTileMode::RenderTarget && resource.tileMode == TextureTileMode::RenderTarget64KB) || (color.tileMode == ColorTileMode::Linear && resource.tileMode == TextureTileMode::kLinear);
        if (!compatibleTiling || resource.width != color.extent.width || resource.height != color.extent.height || resource.dimension != TextureDimension::k2D || resource.mipCount != 1 || resource.baseLevel != 0 || resource.baseArray != 0 || IsBlockCompressed(resource.format) || BytesPerElement(resource.format) != (color.gpuOnly ? 8u : 4u)) source.reset();
    }
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->descriptor != key) continue;
        if (source || it->generation != 0) {
            if (source && it->source.lock() == source && (it->texture->Aliased() || it->generation == source->Generation())) {
                auto result = it->texture;
                entries.splice(entries.end(), entries, it);
                return result;
            }
            retainedBytes -= it->snapshot.size() + it->texture->AllocationBytes();
            entries.erase(it);
            break;
        }
        if (it->validatedEpoch == epoch || epoch < it->skipUntilEpoch) {
            auto result = it->texture;
            entries.splice(entries.end(), entries, it);
            return result;
        }
        PerformanceTimer compareTiming("Graphics.TextureCache.validate");
        GuestMemory::CheckRange(reinterpret_cast<const void*>(resource.baseAddress), it->snapshot.size(), 1);
        compareTiming.Mark("range_check", it->snapshot.size());
        const bool unchanged = std::memcmp(reinterpret_cast<const void*>(resource.baseAddress), it->snapshot.data(), it->snapshot.size()) == 0;
        compareTiming.Mark(unchanged ? "unchanged" : "changed", it->snapshot.size());
        if (unchanged) {
            it->validatedEpoch = epoch;
            if (it->stable < 10 && resource.tileMode != TextureTileMode::RenderTarget64KB) ++it->stable;
            it->skipUntilEpoch = epoch + (1ull << it->stable);
            auto result = it->texture;
            entries.splice(entries.end(), entries, it);
            return result;
        }
        retainedBytes -= it->snapshot.size() + it->texture->AllocationBytes();
        entries.erase(it);
        ++changed;
        break;
    }
    if (source) {
        PerformanceTimer residentTiming("Graphics.TextureCache.create_resident");
        const bool feedback = std::find(activeTargets.begin(), activeTargets.end(), source.get()) != activeTargets.end();
        auto texture = std::make_shared<Texture>(context, source, resource, components, !feedback);
        entries.push_back({key, {}, texture, source, source->Generation()});
        retainedBytes += texture->AllocationBytes();
        trim();
        return texture;
    }
    PerformanceTimer createTiming("Graphics.TextureCache.create_guest");
    CountEvent(FrameTiming::TexCreates);
    ++guestCreates;
    if (guestCreates % 600 == 0) {
        std::fprintf(stderr, "[TEXCACHE] guest creates=%llu of which content-changed=%llu, evictions=%llu, entries=%zu, retained=%llu MB\n",
            static_cast<unsigned long long>(guestCreates), static_cast<unsigned long long>(changed), static_cast<unsigned long long>(evictions), entries.size(), static_cast<unsigned long long>(retainedBytes >> 20u));
        std::fflush(stderr);
    }
    const auto mips = ComputeMipLayout(resource.tileMode, resource.format, resource.width, resource.height, resource.mipCount);
    const auto layers = resource.dimension == TextureDimension::k2DArray || resource.dimension == TextureDimension::kCube ? resource.depthOrLastArray + 1u : 1u;
    const auto bytes = ComputeSurfaceSize(mips, layers);
    Require(bytes != 0 && bytes <= std::numeric_limits<std::size_t>::max(), "texture cache surface size overflow");
    std::vector<std::byte> snapshot(static_cast<std::size_t>(bytes));
    GuestMemory::Read(resource.baseAddress, snapshot, 1);
    auto texture = std::make_shared<Texture>(context, *context.detiler, resource, components, snapshot);
    uploading.push_back(texture);
    const auto retained = snapshot.size() + texture->AllocationBytes();
    entries.push_back({key, std::move(snapshot), texture});
    entries.back().validatedEpoch = epoch;
    retainedBytes += retained;
    trim();
    return texture;
}

}
