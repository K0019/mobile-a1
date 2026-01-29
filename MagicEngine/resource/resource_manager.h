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

    // Batch asset creation
    std::vector<MeshHandle> createMeshBatch(const std::vector<ProcessedMesh>& meshes);

    std::vector<MaterialHandle> createMaterialBatch(const std::vector<ProcessedMaterial>& materials);

    std::vector<TextureHandle> createTextureBatch(const std::vector<ProcessedTexture>& textures);

    // Animation assets
    ClipId createClip(const ProcessedAnimationClip& clip);

    void freeClip(ClipId id);

    const AnimationClip& Clip(ClipId id) const;

    const Skeleton& Skeleton(SkeletonId id) const;

    const MorphSet& Morph(MorphSetId id) const;

    // Asset data access
    const MeshGPUData* getMesh(MeshHandle handle) const;

    const ResourceTraits<MeshAsset>::ColdData* getMeshMetadata(MeshHandle handle) const;

    uint32_t getTextureUIId(TextureHandle handle) const;

    const ResourceTraits<TextureAsset>::HotData* getTextureHotData(TextureHandle handle) const;

    const Material* getMaterialCPU(MaterialHandle handle) const;

    const ResourceTraits<MaterialAsset>::HotData* getMaterialHotData(MaterialHandle handle) const;

    const ResourceTraits<FontAsset>::HotData* getFont(FontHandle handle) const;

    const ResourceTraits<FontAsset>::ColdData* getFontMetadata(FontHandle handle) const;

    const FontGlyph* getFontGlyph(FontHandle handle, uint32_t codepoint) const;

    uint32_t getFontTextureUIId(FontHandle handle) const;

    FontHandle getDefaultUIFont() const;

    void setDefaultUIFont(FontHandle handle);

    bool isMaterialTransparent(MaterialHandle handle) const;

    // Asset management
    void freeMesh(MeshHandle handle);

    void freeMaterial(MaterialHandle handle);

    void freeTexture(TextureHandle handle);

    void freeFont(FontHandle handle);

  private:
    // Core systems
    Context* m_context;
    FontHandle m_defaultUIFont;

    void loadDefaultUIFont();

    // Asset storage
    ResourcePool<MeshAsset> m_meshPool;
    ResourcePool<MaterialAsset> m_materialPool;
    ResourcePool<TextureAsset> m_texturePool;
    ResourcePool<FontAsset> m_fontPool;
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

    SkeletonId registerSkeleton(const ProcessedSkeleton& skeleton);

    MorphSetId registerMorphSet(const std::vector<MeshMorphTargetInfo>& morphTargets);

    // Cache key generation
    static std::string generateMeshCacheKey(const ProcessedMesh& mesh);

    static std::string generateMaterialCacheKey(const ProcessedMaterial& material);

    static std::string generateTextureCacheKey(const ProcessedTexture& texture);

    static std::string generateFontCacheKey(const ProcessedFont& font);

    std::vector<Resource::AnimationClip> m_clips;
    std::vector<uint32_t> m_clipGenerations;  // Generation per slot for future validation
    std::vector<uint32_t> m_freeClipSlots;    // Free slot indices for reuse
    std::vector<Resource::Skeleton> m_skeletons;
    std::vector<Resource::MorphSet> m_morphSets;
  };
} // namespace AssetLoading
