#ifndef CORE_SHADER_RECOMPILIER_INCLUDE_SHADER_RECOMPILIER_RECOMPILER_HPP
#define CORE_SHADER_RECOMPILIER_INCLUDE_SHADER_RECOMPILIER_RECOMPILER_HPP

#include <atomic>
#include <memory>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ShaderRecompiler {

enum class ShaderStage {
    Compute,
    Vertex,
    TessellationControl,
    TessellationEvaluation,
    Geometry,
    Fragment,
    Local,
    Mesh
};

struct MemoryRegion {
    std::uint64_t guestAddress;
    std::span<const std::byte> bytes;
};

struct ShaderBinary {
    ShaderStage stage;
    std::uint64_t codeAddress;
    std::span<const std::uint32_t> code;
    std::uint64_t headerAddress;
    std::span<const std::byte> header;
};

struct ShaderComputeStageInfo {
    std::array<std::uint32_t, 3> numThreads;
    std::uint32_t ldsSizeDwords;
    std::array<bool, 3> groupIdEnable;
    bool tgSizeEnable;
    std::uint32_t threadIdComponentCount;
};

struct ShaderPixelStageInfo {
    std::uint32_t interpolatorCount;
    std::array<std::uint32_t, 32> interpolatorSettings;
    bool wave32;
    std::uint32_t perspectiveCenterVgpr;
    bool hasPerspectiveCenterVgpr;
    bool posX;
    bool posY;
    bool posZ;
    bool posW;
    bool frontFace;
    bool ancillary;
    bool sampleShading;
    bool noPerspective;
    bool pixelKillEnable;
    bool depthExportEnable;
    bool sampleMaskExportEnable;
    bool earlyZ;
    bool executeOnNoop;
    std::array<std::uint8_t, 8> targetOutputMode;
    std::array<std::uint8_t, 8> targetExportMapping;
};

struct ShaderVertexBufferResource {
    std::array<std::uint32_t, 4> fields;
};

struct ShaderVertexResourceDestination {
    std::int32_t registerStart;
    std::int32_t registersNum;
    std::int32_t attrId;
    std::uint32_t fetchIndex;
};

struct ShaderVertexStageInfo {
    static constexpr std::uint32_t MaxResources = 32;
    std::array<ShaderVertexBufferResource, MaxResources> resources;
    std::array<ShaderVertexResourceDestination, MaxResources> resourcesDst;
    std::uint32_t resourcesNum;
    std::uint32_t fetchAttribReg;
    std::uint32_t fetchBufferReg;
    bool fetchEmbedded;
};

struct GuestContext {
    std::uint32_t waveSize;
    std::uint32_t userDataBaseRegister;
    std::span<const std::uint32_t> userData;
    std::optional<ShaderComputeStageInfo> compute;
    std::optional<ShaderPixelStageInfo> pixel;
    std::optional<ShaderVertexStageInfo> vertex;
    std::span<const MemoryRegion> memory;
};

struct MeshTargetLimits {
    std::array<std::uint32_t, 3> maxWorkgroupSize;
    std::uint32_t maxWorkgroupInvocations;
    std::uint32_t maxSharedMemoryBytes;
    std::uint32_t maxOutputVertices;
    std::uint32_t maxOutputPrimitives;
    std::uint32_t maxOutputComponents;
    std::uint32_t maxOutputMemoryBytes;
    std::uint32_t outputPerVertexGranularity;
    std::uint32_t outputPerPrimitiveGranularity;
};

struct TessellationTargetLimits {
    std::uint32_t maxPatchSize;
    std::uint32_t maxControlPerVertexInputComponents;
    std::uint32_t maxControlPerVertexOutputComponents;
    std::uint32_t maxControlPerPatchOutputComponents;
    std::uint32_t maxControlTotalOutputComponents;
    std::uint32_t maxEvaluationInputComponents;
    std::uint32_t maxEvaluationOutputComponents;
};

struct SpirvTarget {
    std::uint32_t vulkanVersion;
    std::uint32_t spirvVersion;
    std::uint32_t subgroupSize;
    std::uint32_t bdaAbiVersion;
    std::span<const std::uint32_t> supportedCapabilities;
    std::span<const std::string_view> supportedExtensions;
    bool fragmentShaderBarycentricEnabled;
    std::array<std::uint32_t, 3> maxWorkgroupSize;
    std::uint32_t maxWorkgroupInvocations;
    std::uint32_t maxWorkgroupSharedMemoryBytes;
    std::optional<MeshTargetLimits> mesh;
    std::optional<TessellationTargetLimits> tessellation;
};

struct BindingLayout {
    std::uint32_t descriptorSet;
    std::uint32_t firstBinding;
    std::uint32_t pushConstantOffsetBytes;
    std::uint32_t pushConstantSizeBytes;
};

enum class ProgramRole {
    Main,
    GeometryBack,
    Local,
    Hull,
    Domain,
    Fragment
};

struct LinkedProgram {
    ProgramRole role;
    ShaderBinary binary;
    std::uint32_t userDataBaseRegister;
    std::uint32_t firstUserSgpr;
    std::span<const std::uint32_t> userData;
};

struct MeshConfiguration {
    std::uint32_t inputPrimitive;
    std::uint32_t primitivesPerGroup;
    std::uint32_t verticesPerGroup;
    std::uint32_t maxVertices;
    std::uint32_t maxPrimitives;
    std::uint32_t threadsPerGroup;
    std::uint32_t ldsSizeDwords;
    std::uint32_t provokingVertex;
};

struct TessellationConfiguration {
    std::uint32_t inputControlPoints;
    std::uint32_t outputControlPoints;
    std::uint32_t domain;
    std::uint32_t partitioning;
    std::uint32_t outputTopology;
};

struct GraphicsDrawParameters {
    std::uint64_t indexAddress;
    std::uint32_t indexCount;
    std::uint32_t indexElementBytes;
    std::uint32_t instanceCount;
};

struct GraphicsCompileContext {
    std::uint32_t firstUserSgpr;
    std::span<const LinkedProgram> linkedPrograms;
    std::optional<MeshConfiguration> mesh;
    std::optional<TessellationConfiguration> tessellation;
    GraphicsDrawParameters draw;
};

struct RecompileRequest {
    ShaderBinary shader;
    GuestContext context;
    SpirvTarget target;
    BindingLayout layout;
    std::optional<GraphicsCompileContext> graphics;
    bool useCache = true;
};

enum class DescriptorKind {
    UniformBuffer,
    StorageBuffer,
    UniformTexelBuffer,
    StorageTexelBuffer,
    SampledImage,
    StorageImage,
    Sampler
};

enum class DescriptorImageShape {
    Image1D,
    Image2D,
    Image2DArray,
    ImageCube,
    Image3D
};

enum class DescriptorRole {
    GuestBuffers,
    GuestImages,
    GuestSamplers,
    Gds,
    BdaPagetable,
    FaultBuffer,
    FlattenedSrt,
    ShaderData
};

struct DescriptorBinding {
    DescriptorKind kind;
    DescriptorRole role;
    std::uint32_t descriptorSet;
    std::uint32_t binding;
    std::uint32_t count;
    std::vector<std::uint32_t> guestDescriptor;
    bool readOnly = false;
    std::optional<DescriptorImageShape> imageShape;
    std::vector<bool> samplerDepthCompare;
};

struct VertexAttribute {
    std::uint32_t location;
    std::uint32_t components;
    ShaderVertexBufferResource resource;
    std::uint32_t fetchIndex;
};

struct FragmentParameter {
    std::uint32_t location;
    std::uint32_t sourceLocation;
    bool flat;
    bool perVertex;
};

// Immutable SPIR-V words shared by every copy of a RecompileResult. The shader cache hands out a copy of the compiled result for every draw (two
// shaders each), and a deep copy of the module dominated the copy and allocation cost of the draw path. Copies now share one storage object, which
// also memoises what is derived from the words: a content hash (replacing per-draw hashing of the whole module for the driver's cache keys) and the
// set of storage-buffer bindings the module may write (an analysis that used to be recomputed per draw).
class SpirvWords {
public:
    SpirvWords() = default;
    SpirvWords(std::vector<std::uint32_t> words) : storage(std::make_shared<Storage>(std::move(words))) {}
    SpirvWords& operator=(std::vector<std::uint32_t> words) {
        storage = std::make_shared<Storage>(std::move(words));
        return *this;
    }
    const std::vector<std::uint32_t>& vec() const { return storage ? storage->words : Empty(); }
    operator const std::vector<std::uint32_t>&() const { return vec(); }
    std::size_t size() const { return vec().size(); }
    bool empty() const { return vec().empty(); }
    const std::uint32_t* data() const { return vec().data(); }
    const std::uint32_t& operator[](std::size_t index) const { return vec()[index]; }
    const std::uint32_t* begin() const { return vec().data(); }
    const std::uint32_t* end() const { return vec().data() + vec().size(); }
    // 64-bit FNV-1a over the words, computed once per module.
    std::uint64_t ContentHash() const {
        if (!storage) return 0;
        auto hash = storage->hash.load(std::memory_order_relaxed);
        if (hash == 0) {
            hash = 1469598103934665603ull;
            for (const auto word : storage->words) hash = (hash ^ word) * 1099511628211ull;
            hash ^= storage->words.size();
            if (hash == 0) hash = 1;
            storage->hash.store(hash, std::memory_order_relaxed);
        }
        return hash;
    }
    // Memoised list of storage-buffer bindings the module may write ({UINT32_MAX} = unknown). Filled lazily by the driver.
    std::shared_ptr<const std::vector<std::uint32_t>>& WrittenBindingsSlot() const {
        static thread_local std::shared_ptr<const std::vector<std::uint32_t>> none;
        if (!storage) {
            none.reset();
            return none;
        }
        return storage->written;
    }

private:
    struct Storage {
        explicit Storage(std::vector<std::uint32_t> words) : words(std::move(words)) {}
        std::vector<std::uint32_t> words;
        mutable std::atomic<std::uint64_t> hash{0};
        mutable std::shared_ptr<const std::vector<std::uint32_t>> written;
    };
    static const std::vector<std::uint32_t>& Empty() {
        static const std::vector<std::uint32_t> none;
        return none;
    }
    std::shared_ptr<Storage> storage;
};

struct RecompileResult {
    SpirvWords spirv;
    std::vector<DescriptorBinding> bindings;
    std::vector<std::byte> pushConstants;
    std::uint32_t bdaAbiVersion = 0;
    std::vector<VertexAttribute> vertexAttributes;
    std::int32_t vertexOffsetSgpr = -1;
    std::int32_t instanceOffsetSgpr = -1;
    std::vector<std::uint32_t> parameterExports;
    std::vector<FragmentParameter> fragmentParameters;
    bool cacheHit = false;
};

[[nodiscard]] RecompileResult Recompile(const RecompileRequest& request);

struct RectListShaders {
    RecompileResult control;
    RecompileResult evaluation;
};

[[nodiscard]] RectListShaders BuildRectListShaders(const RecompileResult& vertex, const RecompileResult& fragment, const SpirvTarget& target);

}

#endif
