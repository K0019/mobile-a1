#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <thread>
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
      void startUploadThread();
      void pollAsyncUploads();
      // Single asset creation
      MeshHandle createMesh(const ProcessedMesh& mesh);

      MaterialHandle createMaterial(const ProcessedMaterial& material);

    void postRendererInitialize();

    TextureHandle createTexture(ProcessedTexture texture);

    FontHandle createFont(const ProcessedFont& font);

    // Batch asset creation
    std::vector<MeshHandle> createMeshBatch(const std::vector<ProcessedMesh>& meshes);

    std::vector<MaterialHandle> createMaterialBatch(const std::vector<ProcessedMaterial>& materials);

    std::vector<TextureHandle> createTextureBatch(std::vector<ProcessedTexture> textures);

    // Animation assets
    ClipId createClip(const ProcessedAnimationClip& clip);

    void freeClip(ClipId id);

    const AnimationClip& Clip(ClipId id) const;

    const Skeleton& Skeleton(SkeletonId id) const;

    const MorphSet& Morph(MorphSetId id) const;

    // Asset data access
    const MeshGPUData* getMesh(MeshHandle handle) const;

    const ResourceTraits<MeshAsset>::ColdData* getMeshMetadata(MeshHandle handle) const;

    const ResourceTraits<TextureAsset>::HotData* getTextureHotData(TextureHandle handle) const;

    const Material* getMaterialCPU(MaterialHandle handle) const;

    const ResourceTraits<MaterialAsset>::HotData* getMaterialHotData(MaterialHandle handle) const;

    const ResourceTraits<FontAsset>::HotData* getFont(FontHandle handle) const;

    const ResourceTraits<FontAsset>::ColdData* getFontMetadata(FontHandle handle) const;

    const FontGlyph* getFontGlyph(FontHandle handle, uint32_t codepoint) const;

    FontHandle getDefaultUIFont() const;

    // Default/placeholder resources for missing or unloaded assets.
    // These are visually obvious (magenta) so missing assets are easy to spot.
    MeshHandle getDefaultMesh() const;
    TextureHandle getDefaultTexture() const;
    MaterialHandle getDefaultMaterial() const;

    // Load a texture from a file path (stb_image) and return an engine handle.
    // The GPU upload is async — resolveTextureView returns default white until ready.
    TextureHandle loadTextureFromFile(const std::string& path, bool sRGB = true);

    // Transparent texture resolution — consumers use these instead of checking upload state
    gfx::TextureView resolveTextureView(TextureHandle handle) const;

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

    // Placeholder resources (created in postRendererInitialize)
    MeshHandle m_defaultMesh;
    TextureHandle m_defaultTexture;  // magenta checkerboard
    MaterialHandle m_defaultMaterial;

    void loadDefaultUIFont();
    void createDefaultResources();

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

    // ========================================================================
    // Upload thread — single background thread for all async GPU uploads
    // ========================================================================
    void uploadThreadFunc();

    // Upload job queue (type-erased via std::function, executed on upload thread)
    using UploadJob = std::function<void(void* /*hina_context**/)>;
    std::mutex m_uploadQueueMutex;
    std::condition_variable m_uploadQueueCV;
    std::vector<UploadJob> m_uploadQueue;
    std::thread m_uploadThread;
    std::atomic<bool> m_uploadThreadStop{false};
    std::atomic<bool> m_uploadThreadRunning{false};

    // Async result tracking — futures fulfilled by upload thread via promises
    struct MeshUploadResult {
        uint16_t gfxMeshIndex = 0;
        uint16_t gfxMeshGeneration = 0;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t totalBytes = 0;
        bool hasSkinning = false;
        bool valid = false;
    };
    struct PendingMeshUpload {
        std::future<MeshUploadResult> future;
        MeshHandle handle;
        std::string meshName;
    };
    std::mutex m_pendingUploadsMutex;
    std::vector<PendingMeshUpload> m_pendingMeshUploads;

    struct TextureUploadResult {
        uint32_t textureId = 0;
        uint32_t viewId = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t hinaFormat = 0;
        bool isSRGB = false;
        bool valid = false;
    };
    struct PendingTextureUpload {
        std::future<TextureUploadResult> future;
        TextureHandle handle;
        std::string textureName;
        std::string cacheKey;
    };
    std::vector<PendingTextureUpload> m_pendingTextureUploads;

  };
} // namespace AssetLoading
