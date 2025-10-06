#pragma once
#include <filesystem>
#include <future>
#include <shared_mutex>
#include <unordered_map>
#include "offsetAllocator.hpp"
#include "processed_assets.h"
#include "resource_pool.h"

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

      TextureHandle createTexture(const ProcessedTexture& texture);

      // Batch asset creation
      std::vector<MeshHandle> createMeshBatch(const std::vector<ProcessedMesh>& meshes);

      std::vector<MaterialHandle> createMaterialBatch(const std::vector<ProcessedMaterial>& materials);

      std::vector<TextureHandle> createTextureBatch(const std::vector<ProcessedTexture>& textures);

      // Asset data access
      const MeshGPUData* getMesh(MeshHandle handle) const;

      uint32_t getMaterialIndex(MaterialHandle handle) const;

      uint32_t getTextureBindlessIndex(TextureHandle handle) const;

      const Material* getMaterialCPU(MaterialHandle handle) const;

      bool isMaterialTransparent(MaterialHandle handle) const;

      // Asset management
      void freeMesh(MeshHandle handle);

      void freeMaterial(MaterialHandle handle);

      void freeTexture(TextureHandle handle);

      // Upload synchronization
      void FlushUploads();


    private:
      // Core systems
      Context* m_context;

      // Asset storage
      ResourcePool<MeshAsset> m_meshPool;
      ResourcePool<MaterialAsset> m_materialPool;
      ResourcePool<TextureAsset> m_texturePool;

      // Memory allocators with mutexes  
      mutable std::mutex m_meshAllocatorMutex;
      mutable std::mutex m_materialAllocatorMutex;

      OffsetAllocator::Allocator m_vertexAllocator;
      OffsetAllocator::Allocator m_indexAllocator;
      OffsetAllocator::Allocator m_materialAllocator;

      // Pending upload structures
      struct PendingMeshUpload
      {
        ProcessedMesh data;
        OffsetAllocator::Allocation vertexAlloc;
        OffsetAllocator::Allocation indexAlloc;
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

      // Material-texture dependency tracking
      std::mutex m_dependencyMutex;
      std::unordered_map<std::string, std::vector<MaterialHandle>> m_unresolvedTextureToMaterials;

      // Internal helper methods
      template <typename T>
      std::vector<T> createAssetBatch(const std::vector<typename T::ProcessedType>& assets, T (ResourceManager::*createFunc)(const typename T::ProcessedType&));

      MaterialData generateMaterialDataWithTextures_nolock(const Material& material);

      uint32_t resolveTextureIndex_nolock(const MaterialTexture& matTex);

      void updateMaterialTextureDependencies_nolock(MaterialHandle materialHandle, const ProcessedMaterial& material);

      void resolveWaitingMaterials_nolock(const std::string& textureCacheKey);

      // Upload batch processing
      void uploadMeshBatch(const std::vector<PendingMeshUpload>& meshes);

      void uploadMaterialBatch(std::vector<PendingMaterialUpload>& materials);

      template <typename Container, typename AllocSelector, typename DataSelector>
      void uploadContiguousBatches(const Container& items, vk::BufferHandle targetBuffer, AllocSelector&& allocSelector, DataSelector&& dataSelector);

      void deduplicateMaterialUploads(std::vector<PendingMaterialUpload>& materials);

      // Cache key generation
      static std::string generateMeshCacheKey(const ProcessedMesh& mesh);

      static std::string generateMaterialCacheKey(const ProcessedMaterial& material);

      static std::string generateTextureCacheKey(const ProcessedTexture& texture);
  };
} // namespace AssetLoading
