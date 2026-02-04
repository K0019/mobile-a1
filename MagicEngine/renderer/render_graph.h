#pragma once
#include <deque>
#include <vector>
#include <functional>
#include <variant>
#include <bitset>
#include <array>
#include <string_view>
#include <unordered_map>
#include "frame_data.h"
#include "gfx_interface.h"
#include "logging/log.h"
#include "linear_color.h"
#include "render_feature.h"
#include "math/utils_math.h"

// Forward declaration for renderer access from features
class GfxRenderer;

/*
// Compile-time string hashing
consteval uint32_t fnv1a_32(const char* str)
{
  uint32_t hash = 2166136261u;
  while (*str)
  {
    hash ^= static_cast<uint32_t>(*str++);
    hash *= 16777619u;
  }
  return hash;
}
*/
namespace RenderResources
{
  // Fixed internal render resolution - avoids recompilation on window resize
  constexpr uint32_t INTERNAL_WIDTH = 1920;
  constexpr uint32_t INTERNAL_HEIGHT = 1080;
  constexpr gfx::Dimensions INTERNAL_RESOLUTION = {INTERNAL_WIDTH, INTERNAL_HEIGHT, 1};

  // UI viewport constants - UI is authored at 1920x1080 (16:9 reference)
  // At different aspect ratios, height stays 1080, width scales proportionally
  // This prevents stretching while maintaining consistent vertical scale
  constexpr float UI_REFERENCE_WIDTH = 1920.0f;
  constexpr float UI_REFERENCE_HEIGHT = 1080.0f;
  constexpr float UI_REFERENCE_ASPECT = UI_REFERENCE_WIDTH / UI_REFERENCE_HEIGHT;  // 16:9

  // Calculate UI viewport width for a given window aspect ratio
  // At 16:9: returns 1920 (reference)
  // At 21:9: returns ~2520 (wider)
  // At 4:3:  returns 1440 (narrower)
  inline float GetUIViewportWidth(float windowAspect) {
    return UI_REFERENCE_HEIGHT * windowAspect;
  }

  // Calculate horizontal offset for centered UI elements
  // At 16:9: returns 0 (no offset needed)
  // At 21:9: returns 300 (extra space on each side)
  // At 4:3:  returns -240 (less space, elements shift inward)
  inline float GetUICenterOffset(float windowAspect) {
    return (GetUIViewportWidth(windowAspect) - UI_REFERENCE_WIDTH) / 2.0f;
  }

  constexpr const char* SWAPCHAIN_IMAGE = "SwapchainImage";
  constexpr const char* SCENE_COLOR = "SceneColor";
  constexpr const char* SCENE_DEPTH = "SceneDepth";
  constexpr const char* VIEW_OUTPUT = "ViewOutput";  // External texture for ImGui viewport display
  constexpr const char* MATERIAL_BUFFER = "MaterialBuffer";
  constexpr const char* VERTEX_BUFFER = "VertexBuffer";
  constexpr const char* SKINNED_VERTEX_BUFFER = "SkinnedVertexBuffer";
  constexpr const char* INDEX_BUFFER = "IndexBuffer";
  constexpr const char* MESH_DECOMPRESSION_BUFFER = "MeshDecompressionBuffer";
  constexpr const char* FRAME_CONSTANTS = "FrameConstants";
  constexpr const char* SKINNING_BUFFER = "SkinningBuffer";
  constexpr const char* MORPH_DELTA_BUFFER = "MorphDeltaBuffer";
  constexpr const char* MORPH_VERTEX_BASE_BUFFER = "MorphVertexBaseBuffer";
  constexpr const char* MORPH_VERTEX_COUNT_BUFFER = "MorphVertexCountBuffer";
  constexpr const char* ANIM_BONE_BUFFER = "AnimationBoneBuffer";
  constexpr const char* ANIM_MORPH_BUFFER = "AnimationMorphBuffer";
  constexpr const char* ANIM_INSTANCE_BUFFER = "AnimationInstanceBuffer";
  // Point light shadow map resources
  constexpr const char* POINT_SHADOW_CUBE_COLOR_0 = "PointShadowCubeColor0";
  constexpr const char* POINT_SHADOW_CUBE_COLOR_1 = "PointShadowCubeColor1";
  constexpr const char* POINT_SHADOW_CUBE_COLOR_2 = "PointShadowCubeColor2";
  constexpr const char* POINT_SHADOW_CUBE_COLOR_3 = "PointShadowCubeColor3";
  constexpr const char* POINT_SHADOW_CUBE_DEPTH_0 = "PointShadowCubeDepth0";
  constexpr const char* POINT_SHADOW_CUBE_DEPTH_1 = "PointShadowCubeDepth1";
  constexpr const char* POINT_SHADOW_CUBE_DEPTH_2 = "PointShadowCubeDepth2";
  constexpr const char* POINT_SHADOW_CUBE_DEPTH_3 = "PointShadowCubeDepth3";
  /*// Pre-computed hashes using compile-time function
  consteval uint32_t SWAPCHAIN_HASH = fnv1a_32(SWAPCHAIN_IMAGE);
  consteval uint32_t SCENE_COLOR_HASH = fnv1a_32(SCENE_COLOR);
  consteval uint32_t SCENE_DEPTH_HASH = fnv1a_32(SCENE_DEPTH);
  consteval uint32_t MATERIAL_HASH = fnv1a_32(MATERIAL_BUFFER);
  consteval uint32_t VERTEX_HASH = fnv1a_32(VERTEX_BUFFER);
  consteval uint32_t INDEX_HASH = fnv1a_32(INDEX_BUFFER);*/
} // namespace RenderResources
namespace internal
{
  using NameID = uint32_t;
  constexpr NameID INVALID_NAME_ID = 0;
  constexpr size_t MAX_RESOURCES = 256;
  constexpr size_t MAX_PASSES = 128;
  constexpr size_t MAX_NAME_IDS = 512;
  class ExecutionContext;
}

// Hash specialization for gfx handle types (ID-based)
template<>
struct std::hash<gfx::Texture> {
    size_t operator()(const gfx::Texture& t) const { return std::hash<uint32_t>()(t.id); }
};

template<>
struct std::hash<gfx::Buffer> {
    size_t operator()(const gfx::Buffer& b) const { return std::hash<uint32_t>()(b.id); }
};

class RenderGraph;
using ExecuteLambda = std::function<void(internal::ExecutionContext& context)>;
using LogicalResourceName = const char*;

enum class AccessType : uint8_t
{
  Read = 0,
  Write = 1,
  ReadWrite = 2
};

// Output configuration for render graph execution
// Contains external textures that the render graph will write to
struct ViewOutputConfig
{
  gfx::Texture resolvedColor;  // LDR output texture for ImGui viewport display
  gfx::Texture swapchainImage; // Optional swapchain image for presentation
};

enum class ResourceType : uint8_t
{
  None = 0,
  Texture = 1,
  Buffer = 2
};

// Packed resource handle - uses gfx types (opaque pointers)
struct ResourceHandle
{
  union
  {
    gfx::Texture texture;
    gfx::Buffer buffer;
    void* packed;
  };

  ResourceType type;

  ResourceHandle() : packed(nullptr), type(ResourceType::None)
  {
  }

  explicit ResourceHandle(gfx::Texture tex) : texture(tex), type(ResourceType::Texture)
  {
  }

  explicit ResourceHandle(gfx::Buffer buf) : buffer(buf), type(ResourceType::Buffer)
  {
  }

  bool isValid() const
  {
    return type != ResourceType::None && packed != nullptr;
  }

  bool operator==(const ResourceHandle& other) const
  {
    return type == other.type && packed == other.packed;
  }

  bool operator<(const ResourceHandle& other) const
  {
    return packed < other.packed;
  }
};

struct ResourceProperties
{
  std::variant<gfx::TextureDesc, gfx::BufferDesc> desc;
  ResourceType type;
  bool persistent = false;
  static constexpr gfx::Dimensions SWAPCHAIN_RELATIVE_DIMENSIONS = {0, 0, 0};
  // Fixed internal resolution - use this for scene rendering to avoid recompilation on resize
  static constexpr gfx::Dimensions INTERNAL_RESOLUTION_DIMENSIONS = RenderResources::INTERNAL_RESOLUTION;

  static ResourceProperties FromDesc(const gfx::TextureDesc& textureDesc, bool isPersistent = false);

  static ResourceProperties FromDesc(const gfx::BufferDesc& bufferDesc, bool isPersistent = false);
};

struct AttachmentDesc
{
  const char* textureName = nullptr;
  const char* resolveTextureName = nullptr;
  gfx::LoadOp loadOp = gfx::LoadOp::DontCare;  // DontCare = invalid/not specified
  gfx::StoreOp storeOp = gfx::StoreOp::Store;
  uint8_t layer = 0;
  uint8_t level = 0;
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepth = 1.0f;
  uint32_t clearStencil = 0;
};

struct PassDeclarationInfo
{
  AttachmentDesc colorAttachments[gfx::MAX_COLOR_ATTACHMENTS];
  AttachmentDesc depthAttachment;
  AttachmentDesc stencilAttachment;
  uint32_t layerCount = 1;
  uint32_t viewMask = 0;
  const char* framebufferDebugName = "";
};

// Use NameIDs instead of string pointers for faster comparisons
struct GraphAttachmentDescription
{
  internal::NameID textureNameID = internal::INVALID_NAME_ID;
  internal::NameID resolveTextureNameID = internal::INVALID_NAME_ID;
  gfx::LoadOp loadOp = gfx::LoadOp::DontCare;  // DontCare = invalid/not specified
  gfx::StoreOp storeOp = gfx::StoreOp::Store;
  uint8_t layer = 0;
  uint8_t level = 0;
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepth = 1.0f;
  uint32_t clearStencil = 0;

  // Helper to check if this attachment slot is used (has a valid texture and a load op other than DontCare)
  bool IsActive() const
  {
    return textureNameID != internal::INVALID_NAME_ID;
  }

  bool operator==(const GraphAttachmentDescription& other) const;
};

struct GraphicsPassDeclaration
{
  GraphAttachmentDescription colorAttachments[gfx::MAX_COLOR_ATTACHMENTS];
  GraphAttachmentDescription depthAttachment;
  GraphAttachmentDescription stencilAttachment;
  uint32_t layerCount = 1;
  uint32_t viewMask = 0;
  const char* framebufferDebugName = "";
};

static_assert(std::is_standard_layout_v<GraphicsPassDeclaration>,
              "GraphicsPassDeclaration must be a POD-like type for memcmp to be safe.");

namespace internal
{
  class RenderPassBuilder
  {
  public:
    enum class PassPriority : int
    {
      EarlySetup = 0, // Resource initialization, clearing
      ShadowMap = 100, // Shadow map generation
      ZPrepass = 200, // Depth pre-pass for early-z
      LightCulling = 250, // Light culling / clustering
      Opaque = 300, // Opaque geometry rendering
      Transparent = 400, // Transparent geometry collection
      OITResolve = 450, // Order-independent transparency resolve
      PostProcess = 500, // Post-processing effects (SSAO, bloom, etc.)
      UI = 600, // User interface elements
      Present = 700 // Final output (tone mapping, gamma correction)
    };

    struct DeclaredResourceUsage
    {
      NameID id;
      AccessType accessType;
    };

    class PassBuilder
    {
    public:
      PassBuilder& DeclareTransientResource(LogicalResourceName name, const gfx::TextureDesc& desc);

      PassBuilder& DeclareTransientResource(LogicalResourceName name, const gfx::BufferDesc& desc);

      PassBuilder& UseResource(LogicalResourceName name, AccessType accessType);

      PassBuilder& SetPriority(PassPriority priority);

      PassBuilder& ExecuteAfter(const char* passName);

      RenderPassBuilder& AddGraphicsPass(const char* passName, const PassDeclarationInfo& declaration,
                                         ExecuteLambda&& executeLambda);

      RenderPassBuilder& AddGenericPass(const char* passName, ExecuteLambda&& executeLambda);

    private:
      friend class RenderPassBuilder;

      explicit PassBuilder(RenderPassBuilder* parent);

      RenderPassBuilder* m_parent;
      std::vector<DeclaredResourceUsage> m_resourceDeclarations;
      std::vector<NameID> m_executeAfterTargets;
      PassPriority m_priority = PassPriority::Opaque;
    };

    PassBuilder CreatePass();

  private:
    friend class ::RenderGraph;
    friend class PassBuilder;

    RenderPassBuilder(RenderGraph* graph, IRenderFeature* feature);

    void SubmitGraphicsPass(PassBuilder& builder, const char* passName, const PassDeclarationInfo& declaration,
                            ExecuteLambda&& executeLambda);

    void SubmitGenericPass(PassBuilder& builder, const char* passName, ExecuteLambda&& executeLambda);

    RenderGraph* m_pRenderGraph;
    IRenderFeature* m_pCurrentFeature;
  };
} // namespace internal
// Hot execution data - cache-line aligned
struct alignas(16) PassExecutionData
{
  internal::NameID passID;
  uint8_t passType; // 0=Graphics, 1=Generic
  uint8_t handleCount;
  uint16_t handleOffset; // Offset into handle storage
  ExecuteLambda* executeLambda; // Heap-allocated to avoid inline storage
};

// Cold metadata - separate allocation
struct PassMetadata
{
  LogicalResourceName debugName;
  std::vector<internal::RenderPassBuilder::DeclaredResourceUsage> declaredResourceAccesses;
  internal::RenderPassBuilder::PassPriority priority;
  std::vector<internal::NameID> executeAfterTargets;
  GraphicsPassDeclaration graphicsDeclaration;
  FeatureMask requiredFeatureMask = 0;  // Which features must be enabled for this pass
  IRenderFeature* featureOwner;
};

namespace internal
{
  // ============================================================================
  // UniformRing - Triple-buffered uniform allocation for per-frame data
  // ============================================================================

  /**
   * @brief Result of a uniform allocation from the ring buffer.
   */
  struct UniformAllocation
  {
    gfx::Buffer buffer;    // Buffer handle for binding
    uint64_t offset;       // Offset within buffer (aligned)
    void* mapped;          // Mapped pointer to write data

    bool isValid() const { return mapped != nullptr; }
  };

  /**
   * @brief Triple-buffered ring allocator for per-frame uniform data.
   *
   * Owned by RenderGraph, exposed to passes via ExecutionContext.
   * Handles alignment automatically based on device limits.
   */
  class UniformRing
  {
  public:
    static constexpr uint64_t DEFAULT_SIZE = 4 * 1024 * 1024; // 4MB per frame

    bool init(uint64_t sizePerFrame = DEFAULT_SIZE);
    void shutdown();

    // Called by RenderGraph at frame start
    void beginFrame(uint32_t frameIndex);

    // Allocate space and return mapped pointer (does NOT copy data)
    UniformAllocation allocate(uint64_t size);

    // Allocate and copy data in one call
    UniformAllocation write(const void* data, uint64_t size);

    uint32_t getAlignment() const { return m_alignment; }

  private:
    std::array<gfx::Holder<gfx::Buffer>, MAX_FRAMES_IN_FLIGHT> m_buffers;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mapped{};
    uint64_t m_sizePerFrame = 0;
    uint64_t m_currentOffset = 0;
    uint32_t m_currentFrame = 0;
    uint32_t m_alignment = 256; // Default, updated from device limits
    bool m_initialized = false;
  };

  // Fast resource cache with perfect hashing for small sets
  struct FastResourceCache
  {
    static constexpr size_t CACHE_SIZE = 16;

    struct Entry
    {
      uint32_t nameHash;
      uint8_t index;
    };

    std::array<Entry, CACHE_SIZE> entries{};
    uint8_t count = 0;

    void Insert(uint32_t hash, uint8_t index);

    uint8_t Find(uint32_t hash) const;

    void Clear();
  };

  class ExecutionContext
  {
  public:
    ExecutionContext(gfx::Cmd* cmd, const FrameData& frameData, RenderGraph* graph,
                     size_t passIndex);

    gfx::Texture GetTexture(LogicalResourceName name) const;

    gfx::Buffer GetBuffer(LogicalResourceName name) const;

    void ResizeBuffer(LogicalResourceName name, uint64_t newSize) const;

    uint64_t GetBufferSize(LogicalResourceName name) const;

    // Fast indexed access
    gfx::Texture GetTextureByIndex(size_t index) const;

    gfx::Buffer GetBufferByIndex(size_t index) const;

    // Get the raw command buffer pointer
    gfx::Cmd* GetCmd() const { return m_cmd; }

    // Get command buffer wrapper
    gfx::CommandBuffer& GetCommandBuffer() const { return const_cast<gfx::CommandBuffer&>(m_cmdWrapper); }

    const FrameData& GetFrameData() const
    {
      return m_frameData;
    }

    // Get GfxRenderer for feature rendering
    GfxRenderer* GetGfxRenderer() const;

    // Get HinaContext directly (preferred for new code)
    HinaContext* GetHinaContext() const;

    // ========================================================================
    // Uniform Allocation API
    // ========================================================================

    /**
     * @brief Allocate uniform buffer space for this frame.
     *
     * Returns a mapped pointer where data can be written directly.
     * The buffer and offset can be used for binding.
     *
     * @param size Size in bytes to allocate
     * @return UniformAllocation with buffer, offset, and mapped pointer
     */
    UniformAllocation AllocateUniform(uint64_t size);

    /**
     * @brief Allocate and write uniform data in one call.
     *
     * @param data Pointer to data to copy
     * @param size Size in bytes
     * @return UniformAllocation with buffer, offset (mapped pointer points to copied data)
     */
    UniformAllocation WriteUniform(const void* data, uint64_t size);

  private:
    gfx::Cmd* m_cmd;
    gfx::CommandBuffer m_cmdWrapper;
    const FrameData& m_frameData;
    RenderGraph* m_pGraph;
    size_t m_passIndex;
    mutable FastResourceCache m_cache;

    ResourceHandle GetResourceCached(LogicalResourceName name) const;

    static uint32_t HashString(const char* str);
  };

  // Efficient frame buffer manager with direct indexing
  struct FrameBufferManager
  {
    struct Entry
    {
      std::array<gfx::Holder<gfx::Buffer>, MAX_FRAMES_IN_FLIGHT> buffers;
      gfx::BufferDesc desc;
      uint64_t currentSize = 0;
      bool active = false;
    };

    std::array<Entry, MAX_RESOURCES> m_resources{};
    std::array<uint16_t, MAX_NAME_IDS> m_nameToIndex{}; // NameID -> resource index
    uint16_t m_nextIndex = 0;

    uint16_t AllocateEntry(NameID id);

    Entry* GetEntry(NameID id);

    gfx::Buffer GetBuffer(NameID id, uint32_t frameIdx) const;
  };
} // namespace internal
class RenderGraph
{
public:
  RenderGraph();

  ~RenderGraph();

  // Feature management
  void AddFeature(IRenderFeature* feature);

  void RemoveFeature(IRenderFeature* feature);

  void ClearFeaturesAndPasses();

  // Feature mask system - assigns unique IDs to features and enables filtering
  FeatureMask RegisterFeature(IRenderFeature* feature);
  FeatureMask RegisterFeatureWithId(IRenderFeature* feature, RenderFeatureId explicitId);
  void UnregisterFeature(IRenderFeature* feature);
  FeatureMask GetFeatureMask(IRenderFeature* feature) const;
  FeatureMask GetActiveFeatureMask() const { return m_activeFeatureMask; }

  // External resource import
  internal::NameID ImportExternalTexture(LogicalResourceName name, gfx::Texture textureHandle,
                                         const gfx::TextureDesc& desc);

  internal::NameID ImportExternalBuffer(LogicalResourceName name, gfx::Buffer bufferHandle,
                                        const gfx::BufferDesc& desc);

  // Execution
  void Execute(gfx::Cmd* cmd, const FrameData& frameData, const ViewOutputConfig& outputConfig);

  void BeginFrame()
  {
    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    m_uniformRing.beginFrame(m_currentFrameIndex);
  }

  uint32_t GetCurrentFrameIndex() const
  {
    return m_currentFrameIndex;
  }

  void ResizeTransientBuffer(LogicalResourceName name, uint64_t newSize);

  // State management
  void Invalidate()
  {
    m_isCompiled = false;
  }

  bool IsCompiled() const
  {
    return m_isCompiled;
  }

  // Check if graph can compile (needs swapchain)
  bool canCompile() const;

  // Attempt compilation, returns true if successful
  bool tryCompile();

  LinearColorSystem& GetLinearColorSystem()
  {
    return linearColorSystem;
  }

  // GfxRenderer access for features
  void SetGfxRenderer(GfxRenderer* renderer);
  GfxRenderer* GetGfxRenderer() const { return m_gfxRenderer; }

  // Uniform ring access (for ExecutionContext)
  internal::UniformRing& GetUniformRing() { return m_uniformRing; }

private:
  friend class internal::RenderPassBuilder;
  friend class internal::ExecutionContext;
  friend class LinearColorSystem;

  // Batched execution step
  struct BatchedGraphicsPassExecution
  {
    gfx::RenderPass renderPassInfo;
    gfx::Dependencies accumulatedDependencies;
    gfx::Framebuffer framebuffer;
    std::array<size_t, 8> passIndices; // Indices into m_passExecData
    uint8_t passCount = 0;
  };

  struct CompiledExecutionStep
  {
    enum class Type : uint8_t
    {
      Generic,
      Graphics
    };

    Type type;

    union
    {
      size_t genericPassIndex;
      BatchedGraphicsPassExecution graphicsBatch;
    };

    explicit CompiledExecutionStep(size_t passIdx) : type(Type::Generic), genericPassIndex(passIdx)
    {
    }

    explicit CompiledExecutionStep(BatchedGraphicsPassExecution&& batch) : type(Type::Graphics),
                                                                           graphicsBatch(std::move(batch))
    {
    }
  };

  struct ResolvedResourceInfo
  {
    internal::NameID id = internal::INVALID_NAME_ID;
    ResourceHandle concreteHandle;
    ResourceProperties definition;
    bool isExternal = false;
    uint32_t firstUsePassIndex = ~0u;
    uint32_t lastUsePassIndex = 0;
  };

  // Core state
  GfxRenderer* m_gfxRenderer = nullptr;
  LinearColorSystem linearColorSystem;
  internal::UniformRing m_uniformRing;
  bool m_isCompiled = false;
  bool m_compilationDeferred = false; // Track deferred state
  uint32_t m_currentFrameIndex = 0;
  gfx::Dimensions m_lastCompileDimensions = {0, 0, 0};

  // Blit pipeline for scaling fixed-resolution scene to swapchain (LDR to LDR)
  gfx::Holder<gfx::Pipeline> m_blitPipeline;
  gfx::Holder<gfx::Sampler> m_blitSampler;
  gfx::Holder<gfx::BindGroupLayout> m_blitBindGroupLayout;
  bool m_blitPipelineCreated = false;

  void EnsureBlitPipelineCreated();

  // Resolve output pipeline for HDR to LDR tone mapping
  gfx::Holder<gfx::Pipeline> m_resolveOutputPipeline;
  gfx::Holder<gfx::Sampler> m_resolveOutputSampler;
  gfx::Holder<gfx::BindGroupLayout> m_resolveOutputBindGroupLayout;
  bool m_resolveOutputPipelineCreated = false;

  void EnsureResolveOutputPipelineCreated();
  // String interning - optimized with sorted vector for better cache locality
  std::deque<std::string> m_stringStorage;
  std::vector<std::pair<std::string_view, internal::NameID>> m_nameRegistry;
  std::vector<std::string_view> m_idToStringMap; // Reverse lookup
  internal::NameID m_nextNameID = 1;
  internal::NameID m_swapchainID = internal::INVALID_NAME_ID;
  internal::NameID m_viewOutputID = internal::INVALID_NAME_ID;

  // Feature mask system
  RenderFeatureId m_nextFeatureId = 0;
  std::unordered_map<IRenderFeature*, RenderFeatureId> m_featureIdMap;
  FeatureMask m_activeFeatureMask = ~FeatureMask(0);  // All features enabled by default

  // Features and passes - hot/cold split
  std::vector<IRenderFeature*> m_features;
  std::vector<PassExecutionData> m_passExecData;
  std::vector<PassMetadata> m_passMetadata;
  std::vector<ExecuteLambda> m_executionLambdas;
  // Resource handle storage
  std::vector<ResourceHandle> m_handleStorage;
  // Compilation results
  std::vector<size_t> m_compiledPassOrder;
  std::vector<CompiledExecutionStep> m_executionSteps;
  // Resources - direct indexing instead of map
  std::vector<ResolvedResourceInfo> m_resolvedResourceInfo; // Indexed by NameID - 1
  internal::FrameBufferManager m_frameBuffers;
  std::vector<gfx::Holder<gfx::Texture>> m_storedTextures;
  // Internal methods
  internal::NameID InternName(LogicalResourceName name);

  internal::NameID FindNameID(LogicalResourceName name) const;

  LogicalResourceName GetNameString(internal::NameID id) const;

  void RegisterTransientResource(LogicalResourceName name, const ResourceProperties& properties);

  void RegisterTransientResource(LogicalResourceName name, const gfx::TextureDesc& desc);

  void RegisterTransientResource(LogicalResourceName name, const gfx::BufferDesc& desc);

  void RegisterPass(PassExecutionData&& execData, PassMetadata&& metadata, ExecuteLambda&& lambda);

  void ExecuteFinalBlit(const internal::ExecutionContext& ctx);
  void ExecuteResolveViewOutput(const internal::ExecutionContext& ctx);
  void UpdateViewOutputResource(gfx::Texture outputHandle);

  ResourceHandle GetResolvedHandle(internal::NameID id) const;

  ResolvedResourceInfo* GetResolvedResource(internal::NameID id);

  const ResolvedResourceInfo* GetResolvedResource(internal::NameID id) const;

  // Compilation pipeline
  void Compile();

  // Helper to validate minimum resources
  bool hasMinimumResources() const;

  void BuildAdjacencyListAndSort();

  void UpdateSwapchainResource();

  // Pass batching
  static bool AreGraphicsDeclarationsEqual(const GraphicsPassDeclaration& decl1, const GraphicsPassDeclaration& decl2);

  static void FillRenderPassFromDeclaration(const GraphicsPassDeclaration& graphDecl, gfx::RenderPass& rp);

  void FillFramebufferFromDeclaration(const GraphicsPassDeclaration& graphDecl, gfx::Framebuffer& fb);

  void AggregateDependenciesForPass(size_t passIndex, std::vector<gfx::Texture>& textures,
                                    std::vector<gfx::Buffer>& buffers) const;

  void EnsureBufferCapacity(internal::FrameBufferManager::Entry& resource, uint64_t requiredSize);
};
