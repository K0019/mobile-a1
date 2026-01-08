#pragma once
#include <array>
#include <filesystem>
#include <future>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include "animation_clip.h"
#include "morph_set.h"
#include "offsetAllocator.hpp"
#include "processed_assets.h"
#include "renderer/gpu_data.h"
#include "resource_pool.h"
#include "skeleton.h"
#include "resource/animation_ids.h"
#include "resource/font_types.h"
struct Context;

namespace Resource
{
  class ResourceManager
  {
    public:
      ResourceManager();

      void initialize(Context* context);

      ~ResourceManager();

      void shutdown();
      // Single asset creation
      MeshHandle createMesh(const ProcessedMesh& mesh);

      MaterialHandle createMaterial(const ProcessedMaterial& material);

    void postRendererInitialize();

    TextureHandle createTexture(const ProcessedTexture& texture);

    FontHandle createFont(const ProcessedFont& font);

    size_t getUsedVertexBytes() const;

    uint32_t getVertexBufferVersion() const;

    void bumpVertexBufferVersion();

    // Batch asset creation
    std::vector<MeshHandle> createMeshBatch(const std::vector<ProcessedMesh>& meshes);

    std::vector<MaterialHandle> createMaterialBatch(const std::vector<ProcessedMaterial>& materials);

    std::vector<TextureHandle> createTextureBatch(const std::vector<ProcessedTexture>& textures);

    // Animation assets
    ClipId createClip(const ProcessedAnimationClip& clip);

    const AnimationClip& Clip(ClipId id) const;

    const Skeleton& Skeleton(SkeletonId id) const;

    const MorphSet& Morph(MorphSetId id) const;

    // Asset data access
    const MeshGPUData* getMesh(MeshHandle handle) const;

    const ResourceTraits<MeshAsset>::ColdData* getMeshMetadata(MeshHandle handle) const;

    uint32_t getMaterialIndex(MaterialHandle handle) const;

    uint32_t getTextureBindlessIndex(TextureHandle handle) const;

    const Material* getMaterialCPU(MaterialHandle handle) const;

    const ResourceTraits<FontAsset>::HotData* getFont(FontHandle handle) const;

    const ResourceTraits<FontAsset>::ColdData* getFontMetadata(FontHandle handle) const;

    const FontGlyph* getFontGlyph(FontHandle handle, uint32_t codepoint) const;

    uint32_t getFontTextureBindlessIndex(FontHandle handle) const;

    FontHandle getDefaultUIFont() const;

    void setDefaultUIFont(FontHandle handle);

    bool isMaterialTransparent(MaterialHandle handle) const;

    // Asset management
    void freeMesh(MeshHandle handle);

    void freeMaterial(MaterialHandle handle);

    void freeTexture(TextureHandle handle);

    void freeFont(FontHandle handle);

    // Upload synchronization
    void FlushUploads();

  private:
    // Core systems
    Context* m_context;
    FontHandle m_defaultUIFont;

    void loadDefaultUIFont();

    uint32_t m_vertexBufferVersion = 1; // start at 1, 0 = uninitialized
    // Asset storage
    ResourcePool<MeshAsset> m_meshPool;
    ResourcePool<MaterialAsset> m_materialPool;
    ResourcePool<TextureAsset> m_texturePool;
    ResourcePool<FontAsset> m_fontPool;
    // Memory allocators with mutexes  
    mutable std::mutex m_meshAllocatorMutex;
    mutable std::mutex m_materialAllocatorMutex;
    OffsetAllocator::Allocator m_vertexAllocator;
    OffsetAllocator::Allocator m_indexAllocator;
    OffsetAllocator::Allocator m_materialAllocator;
    OffsetAllocator::Allocator m_meshDecompAllocator;
    OffsetAllocator::Allocator m_skinningAllocator;
    OffsetAllocator::Allocator m_morphDeltaAllocator;
    OffsetAllocator::Allocator m_morphBaseAllocator;
    OffsetAllocator::Allocator m_morphCountAllocator;

    // Pending upload structures
    struct PendingMeshUpload
    {
      std::vector<CompressedVertex> compressedVertices; // Compressed on CPU
      std::vector<uint32_t> indices;
      std::vector<GPUSkinningData> skinningData;
      std::vector<GPUMorphDelta> morphDeltas;
      std::vector<uint32_t> morphVertexStarts;
      std::vector<uint32_t> morphVertexCounts;
      MeshDecompressionData decompressionData; // Decompression parameters
      OffsetAllocator::Allocation vertexAlloc;
      OffsetAllocator::Allocation indexAlloc;
      OffsetAllocator::Allocation meshDecompAlloc; // Decompression data allocation
      OffsetAllocator::Allocation skinningAlloc;
      OffsetAllocator::Allocation morphDeltaAlloc;
      OffsetAllocator::Allocation morphVertexStartAlloc;
      OffsetAllocator::Allocation morphVertexCountAlloc;
    };

    struct PendingMaterialUpload
    {
      MaterialData data;
      OffsetAllocator::Allocation materialAlloc;
    };

    // Pending upload queues
    std::mutex m_pendingMeshesMutex;
    std::vector<PendingMeshUpload> m_pendingMeshes;
    std::mutex m_pendingMaterialsMutex;
    std::vector<PendingMaterialUpload> m_pendingMaterials;
    // Asset caches
    mutable std::shared_mutex m_cacheMutex;
    std::unordered_map<std::string, MeshHandle> m_meshCache;
    std::unordered_map<std::string, MaterialHandle> m_materialCache;
    std::unordered_map<std::string, TextureHandle> m_textureCache;
    std::unordered_map<std::string, FontHandle> m_fontCache;
    // Material-texture dependency tracking
    std::mutex m_dependencyMutex;
    std::unordered_map<std::string, std::vector<MaterialHandle>> m_unresolvedTextureToMaterials;

    MaterialData generateMaterialDataWithTextures_nolock(const Material& material);

    uint32_t resolveTextureIndex_nolock(const MaterialTexture& matTex);

    void updateMaterialTextureDependencies_nolock(MaterialHandle materialHandle, const ProcessedMaterial& material);

    void resolveWaitingMaterials_nolock(const std::string& textureCacheKey);

    // Upload batch processing
    void uploadMeshBatch(const std::vector<PendingMeshUpload>& meshes);

    void uploadMaterialBatch(std::vector<PendingMaterialUpload>& materials);

    SkeletonId registerSkeleton(const ProcessedSkeleton& skeleton);

    MorphSetId registerMorphSet(const std::vector<MeshMorphTargetInfo>& morphTargets);

    template <typename Container, typename AllocSelector, typename DataSelector>
    void uploadContiguousBatches(const Container& items, vk::BufferHandle targetBuffer, AllocSelector&& allocSelector,
                                 DataSelector&& dataSelector);

    void deduplicateMaterialUploads(std::vector<PendingMaterialUpload>& materials);

    // Cache key generation
    static std::string generateMeshCacheKey(const ProcessedMesh& mesh);

    static std::string generateMaterialCacheKey(const ProcessedMaterial& material);

    static std::string generateTextureCacheKey(const ProcessedTexture& texture);

    static std::string generateFontCacheKey(const ProcessedFont& font);

    std::vector<Resource::AnimationClip> m_clips;
    std::vector<Resource::Skeleton> m_skeletons;
    std::vector<Resource::MorphSet> m_morphSets;
  };
} // namespace AssetLoading
