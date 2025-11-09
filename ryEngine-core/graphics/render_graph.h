#pragma once

#include <deque>
#include <vector>
#include <functional>
#include <variant>
#include <bitset>
#include <array>
#include <string_view>

#include "frame_data.h"
#include "interface.h"
#include "i_graph_observer.h"
#include "linear_color.h"
#include "OIT.h"
#include "renderer_resources.h"
#include "render_feature.h"
#include "math/utils_math.h"

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
  constexpr const char* SWAPCHAIN_IMAGE = "SwapchainImage";
  constexpr const char* SCENE_COLOR = "SceneColor";
  constexpr const char* SCENE_DEPTH = "SceneDepth";
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

// Hash specialization for vk::Handle
template <typename ObjectType>
struct std::hash<vk::Handle<ObjectType>>
{
  size_t operator()(const vk::Handle<ObjectType>& h) const
  {
    return std::hash<uint64_t>()((uint64_t(h.index()) << 32) | h.gen());
  }
};

class RenderGraph;
using ExecuteLambda = std::function<void(internal::ExecutionContext& context)>;
using LogicalResourceName = const char*;

enum class AccessType : uint8_t
{
  Read      = 0,
  Write     = 1,
  ReadWrite = 2
};

enum class ResourceType : uint8_t
{
  None    = 0,
  Texture = 1,
  Buffer  = 2
};

// Packed resource handle - 8 bytes
struct ResourceHandle
{
  union
  {
    vk::TextureHandle texture;
    vk::BufferHandle buffer;
    uint64_t packed;
  };

  ResourceType type;

  ResourceHandle() : packed(0), type(ResourceType::None)
  {}

  explicit ResourceHandle(vk::TextureHandle tex) : texture(tex), type(ResourceType::Texture)
  {}

  explicit ResourceHandle(vk::BufferHandle buf) : buffer(buf), type(ResourceType::Buffer)
  {}

  bool isValid() const
  {
    return type != ResourceType::None && packed != 0;
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
  std::variant<vk::TextureDesc, vk::BufferDesc> desc;
  ResourceType type;
  bool persistent = false;

  static constexpr vk::Dimensions SWAPCHAIN_RELATIVE_DIMENSIONS = {0, 0};

  static ResourceProperties FromDesc(const vk::TextureDesc& textureDesc, bool isPersistent = false);

  static ResourceProperties FromDesc(const vk::BufferDesc& bufferDesc, bool isPersistent = false);
};

struct AttachmentDesc
{
  const char* textureName = nullptr;
  const char* resolveTextureName = nullptr;
  vk::LoadOp loadOp = vk::LoadOp::Invalid;
  vk::StoreOp storeOp = vk::StoreOp::Store;
  uint8_t layer = 0;
  uint8_t level = 0;
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepth = 1.0f;
  uint32_t clearStencil = 0;
};

struct PassDeclarationInfo
{
  AttachmentDesc colorAttachments[vk::MAX_COLOR_ATTACHMENTS];
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

  vk::LoadOp loadOp = vk::LoadOp::Invalid;
  vk::StoreOp storeOp = vk::StoreOp::Store;
  uint8_t layer = 0;
  uint8_t level = 0;
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepth = 1.0f;
  uint32_t clearStencil = 0;

  bool IsActive() const
  {
    return textureNameID != internal::INVALID_NAME_ID && loadOp != vk::LoadOp::Invalid;
  }

  bool operator==(const GraphAttachmentDescription& other) const;
};

struct GraphicsPassDeclaration
{
  GraphAttachmentDescription colorAttachments[vk::MAX_COLOR_ATTACHMENTS];
  GraphAttachmentDescription depthAttachment;
  GraphAttachmentDescription stencilAttachment;
  uint32_t layerCount = 1;
  uint32_t viewMask = 0;
  const char* framebufferDebugName = "";
};

static_assert(std::is_standard_layout_v<GraphicsPassDeclaration>, "GraphicsPassDeclaration must be a POD-like type for memcmp to be safe.");

namespace internal
{
  class RenderPassBuilder
  {
    public:
      enum class PassPriority : int
      {
        EarlySetup    = 0,   // Resource initialization, clearing
        ShadowMap     = 100, // Shadow map generation
        ZPrepass      = 200, // Depth pre-pass for early-z
        LightCulling  = 250, // Light culling / clustering
        Opaque        = 300, // Opaque geometry rendering
        Transparent   = 400, // Transparent geometry collection
        OITResolve    = 450, // Order-independent transparency resolve
        PostProcess   = 500, // Post-processing effects (SSAO, bloom, etc.)
        UI            = 600, // User interface elements
        Present       = 700  // Final output (tone mapping, gamma correction)
      };

      struct DeclaredResourceUsage
      {
        NameID id;
        AccessType accessType;
      };

      class PassBuilder
      {
        public:
          PassBuilder& DeclareTransientResource(LogicalResourceName name, const vk::TextureDesc& desc);

          PassBuilder& DeclareTransientResource(LogicalResourceName name, const vk::BufferDesc& desc);

          PassBuilder& UseResource(LogicalResourceName name, AccessType accessType);

          PassBuilder& SetPriority(PassPriority priority);

          PassBuilder& ExecuteAfter(const char* passName);

          RenderPassBuilder& AddGraphicsPass(const char* passName, const PassDeclarationInfo& declaration, ExecuteLambda&& executeLambda);

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

      void SubmitGraphicsPass(PassBuilder& builder, const char* passName, const PassDeclarationInfo& declaration, ExecuteLambda&& executeLambda);

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
  IRenderFeature* featureOwner;
};

namespace internal
{
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
      ExecutionContext(vk::ICommandBuffer& commandBuffer, const FrameData& frameData, RenderGraph* graph, size_t passIndex);

      vk::TextureHandle GetTexture(LogicalResourceName name) const;

      vk::BufferHandle GetBuffer(LogicalResourceName name) const;

      void ResizeBuffer(LogicalResourceName name, uint64_t newSize) const;

      uint64_t GetBufferSize(LogicalResourceName name) const;

      // Fast indexed access
      vk::TextureHandle GetTextureByIndex(size_t index) const;

      vk::BufferHandle GetBufferByIndex(size_t index) const;

      vk::ICommandBuffer& GetvkCommandBuffer() const
      {
        return m_vkCommandBuffer;
      }

      vk::IContext& GetvkContext() const;

      const FrameData& GetFrameData() const
      {
        return m_frameData;
      }

    private:
      vk::ICommandBuffer& m_vkCommandBuffer;
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
      std::array<vk::Holder<vk::BufferHandle>, MAX_FRAMES_IN_FLIGHT> buffers;
      vk::BufferDesc desc;
      uint64_t currentSize = 0;
      bool active = false;
    };

    std::array<Entry, MAX_RESOURCES> m_resources{};
    std::array<uint16_t, MAX_NAME_IDS> m_nameToIndex{}; // NameID -> resource index
    uint16_t m_nextIndex = 0;

    uint16_t AllocateEntry(NameID id);

    Entry* GetEntry(NameID id);

    vk::BufferHandle GetBuffer(NameID id, uint32_t frameIdx) const;
  };
} // namespace internal

class RenderGraph
{
  public:
    explicit RenderGraph(vk::IContext& vkContext, GPUBuffers& gpu_buffers);

    ~RenderGraph();

    // Feature management
    void AddFeature(IRenderFeature* feature);

    void RemoveFeature(IRenderFeature* feature);

    void ClearFeaturesAndPasses();

    // External resource import
    internal::NameID ImportExternalTexture(LogicalResourceName name, vk::TextureHandle textureHandle, const vk::TextureDesc& desc);

    internal::NameID ImportExternalBuffer(LogicalResourceName name, vk::BufferHandle bufferHandle, const vk::BufferDesc& desc);

    // Execution
    void Execute(vk::ICommandBuffer& vkCommandBuffer, const FrameData& frameData);

    void BeginFrame()
    {
      m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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

    void AddTransientObserver(internal::ITransientResourceObserver* observer);

    void RemoveTransientObserver(internal::ITransientResourceObserver* observer);

    LinearColorSystem& GetLinearColorSystem()
    {
      return linearColorSystem;
    }

  private:
    friend class internal::RenderPassBuilder;
    friend class internal::ExecutionContext;
    friend class LinearColorSystem;
    // Batched execution step
    struct BatchedGraphicsPassExecution
    {
      vk::RenderPass renderPassInfo;
      vk::Dependencies accumulatedDependencies;
      vk::Framebuffer framebuffer;
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
      {}

      explicit CompiledExecutionStep(BatchedGraphicsPassExecution&& batch) : type(Type::Graphics), graphicsBatch(std::move(batch))
      {}
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
    vk::IContext& m_vkContext;
    GPUBuffers& m_gpu_buffers;
    OITSystem oit;
    LinearColorSystem linearColorSystem;
    bool m_isCompiled = false;
    bool m_compilationDeferred = false;  // Track deferred state
    uint32_t m_currentFrameIndex = 0;
    vk::Dimensions m_lastCompileDimensions = {0, 0};

    // String interning - optimized with sorted vector for better cache locality
    std::deque<std::string> m_stringStorage;
    std::vector<std::pair<std::string_view, internal::NameID>> m_nameRegistry;
    std::vector<std::string_view> m_idToStringMap; // Reverse lookup
    internal::NameID m_nextNameID = 1;
    internal::NameID m_swapchainID = internal::INVALID_NAME_ID;

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
    std::vector<vk::Holder<vk::TextureHandle>> m_storedTextures;
    // Observers
    std::vector<internal::ITransientResourceObserver*> m_transientObservers;

    // Internal methods
    internal::NameID InternName(LogicalResourceName name);

    internal::NameID FindNameID(LogicalResourceName name) const;

    LogicalResourceName GetNameString(internal::NameID id) const;

    void RegisterTransientResource(LogicalResourceName name, const ResourceProperties& properties);

    void RegisterTransientResource(LogicalResourceName name, const vk::TextureDesc& desc);

    void RegisterTransientResource(LogicalResourceName name, const vk::BufferDesc& desc);

    void RegisterPass(PassExecutionData&& execData, PassMetadata&& metadata, ExecuteLambda&& lambda);

    void ExecuteFinalBlit(const internal::ExecutionContext& ctx);

    ResourceHandle GetResolvedHandle(internal::NameID id) const;

    ResolvedResourceInfo* GetResolvedResource(internal::NameID id);

    const ResolvedResourceInfo* GetResolvedResource(internal::NameID id) const;

    // Compilation pipeline
    void Compile();

    // Helper to validate minimum resources
    bool hasMinimumResources() const;

    void BuildAdjacencyListAndSort();

    void UpdateSwapchainResource();

    void NotifyTransientObservers();

    // Pass batching
    static bool AreGraphicsDeclarationsEqual(const GraphicsPassDeclaration& decl1, const GraphicsPassDeclaration& decl2);

    static void FillVkRenderPassFromDeclaration(const GraphicsPassDeclaration& graphDecl, vk::RenderPass& vkRp);

    void FillVkFramebufferFromDeclaration(const GraphicsPassDeclaration& graphDecl, vk::Framebuffer& vkFb);

    void AggregateDependenciesForPass(size_t passIndex, std::vector<vk::TextureHandle>& textures, std::vector<vk::BufferHandle>& buffers) const;

    void EnsureBufferCapacity(internal::FrameBufferManager::Entry& resource, uint64_t requiredSize);
};
