#include "resource_manager.h"
#include <algorithm>
#include <span>

#include "core/engine/engine.h"
namespace Resource
{
  ResourceManager::ResourceManager() : m_vertexAllocator(ResourceLimits::VERTEX_BUFFER_SIZE), m_indexAllocator(ResourceLimits::INDEX_BUFFER_SIZE), m_materialAllocator(ResourceLimits::MATERIAL_BUFFER_SIZE)
  {

  }

  void ResourceManager::initialize(Context* context)
  {
    this->m_context = context;
    LOG_INFO("Resource Manager initialized - {}M vertices, {}M indices, {}K materials", ResourceLimits::MAX_VERTICES / 1'000'000, ResourceLimits::MAX_INDICES / 1'000'000, ResourceLimits::MAX_MATERIALS / 1000);

  }

  ResourceManager::~ResourceManager() = default;

  void ResourceManager::shutdown()
  {
    m_meshPool.clear();
    m_texturePool.clear();
    m_materialPool.clear();
    LOG_INFO("Resource Manager shutdown complete");
  }

  MeshHandle ResourceManager::createMesh(const ProcessedMesh& mesh)
  {
    const std::string cacheKey = generateMeshCacheKey(mesh);

    // Check cache with shared lock first
    {
      std::shared_lock lock(m_cacheMutex);
      if(auto it = m_meshCache.find(cacheKey); it != m_meshCache.end())
      {
        return it->second;
      }
    }

    // Double-check pattern with unique lock
    std::unique_lock cacheLock(m_cacheMutex);
    if(auto it = m_meshCache.find(cacheKey); it != m_meshCache.end())
    {
      return it->second;
    }

    // Allocate GPU memory
    OffsetAllocator::Allocation vertexAlloc, indexAlloc;
    {
      std::lock_guard allocatorLock(m_meshAllocatorMutex);
      vertexAlloc = m_vertexAllocator.allocate(static_cast<uint32_t>(mesh.vertices.size() * sizeof(Vertex)));
      indexAlloc = m_indexAllocator.allocate(static_cast<uint32_t>(mesh.indices.size() * sizeof(uint32_t)));

      if(!vertexAlloc.isValid() || !indexAlloc.isValid())
      {
        if(vertexAlloc.isValid())
          m_vertexAllocator.free(vertexAlloc);
        if(indexAlloc.isValid())
          m_indexAllocator.free(indexAlloc);
        LOG_ERROR("Out of GPU memory for mesh '{}'", mesh.name);
        return {};
      }
    }

    // Create handle and populate data
    MeshHandle handle = m_meshPool.create();
    if(auto* cold = m_meshPool.getColdData(handle))
    {
      cold->meshName = mesh.name;
      cold->vertexMetadata = vertexAlloc;
      cold->indexMetadata = indexAlloc;
    }
    if(auto* hot = m_meshPool.getHotData(handle))
    {
      hot->vertexByteOffset = vertexAlloc.offset;
      hot->indexByteOffset = indexAlloc.offset;
      hot->vertexCount = static_cast<uint32_t>(mesh.vertices.size());
      hot->indexCount = static_cast<uint32_t>(mesh.indices.size());
      hot->bounds = mesh.bounds;
    }
    // Queue upload
    {
      std::lock_guard pendingLock(m_pendingMeshesMutex);
      m_pendingMeshes.push_back({ mesh, vertexAlloc, indexAlloc });
    }

    m_meshCache[cacheKey] = handle;
    return handle;
  }

  MaterialHandle ResourceManager::createMaterial(const ProcessedMaterial& material)
  {
    const std::string cacheKey = generateMaterialCacheKey(material);

    // Check cache with shared lock first
    {
      std::shared_lock lock(m_cacheMutex);
      if(auto it = m_materialCache.find(cacheKey); it != m_materialCache.end())
      {
        return it->second;
      }
    }

    // Double-check pattern with unique lock
    std::unique_lock cacheLock(m_cacheMutex);
    if(auto it = m_materialCache.find(cacheKey); it != m_materialCache.end())
    {
      return it->second;
    }

    // Allocate GPU memory
    OffsetAllocator::Allocation materialAlloc;
    {
      std::lock_guard allocatorLock(m_materialAllocatorMutex);
      materialAlloc = m_materialAllocator.allocate(sizeof(MaterialData));
      if(!materialAlloc.isValid())
      {
        LOG_ERROR("Out of GPU memory for material '{}'", material.name);
        return {};
      }
    }

    // Create handle and populate data
    Material mat = convertToStoredMaterial(material);
    MaterialHandle handle = m_materialPool.create();

    uint32_t alphaMode = material.flags & MaterialFlags::ALPHA_MODE_MASK;

    if(auto* cold = m_materialPool.getColdData(handle))
    {
      cold->material = mat;
      cold->metadata = materialAlloc;
    }
    if(auto* hot = m_materialPool.getHotData(handle))
    {
      hot->materialOffset = materialAlloc.offset;
      hot->renderQueue = (alphaMode == ALPHA_MODE_BLEND)
        ? RenderQueue::Transparent
        : RenderQueue::Opaque;
    }
    // Generate GPU data and handle dependencies
    MaterialData gpuData = generateMaterialDataWithTextures_nolock(mat);
    updateMaterialTextureDependencies_nolock(handle, material);
    // Queue upload
    {
      std::lock_guard pendingLock(m_pendingMaterialsMutex);
      m_pendingMaterials.push_back({ gpuData, materialAlloc });
    }

    m_materialCache[cacheKey] = handle;
    return handle;
  }

  TextureHandle ResourceManager::createTexture(const ProcessedTexture& texture)
  {
    const std::string cacheKey = generateTextureCacheKey(texture);

    // Check cache with shared lock first  
    {
      std::shared_lock lock(m_cacheMutex);
      if(auto it = m_textureCache.find(cacheKey); it != m_textureCache.end())
      {
        return it->second;
      }
    }

    // Upload texture to GPU immediately
    uint32_t bindlessIndex = m_context->renderer->getGPUBuffers().uploadTexture(texture.textureDesc, texture.data);

    if(bindlessIndex == 0)
    {
      LOG_ERROR("Failed to upload texture '{}'", texture.name);
      return {};
    }

    // Double-check pattern with unique lock
    std::unique_lock cacheLock(m_cacheMutex);
    if(auto it = m_textureCache.find(cacheKey); it != m_textureCache.end())
    {
      // Another thread created this texture, clean up our upload
      m_context->renderer->getGPUBuffers().deleteTexture(bindlessIndex);
      return it->second;
    }

    // Create handle and populate data
    TextureHandle handle = m_texturePool.create();
    if(auto* cold = m_texturePool.getColdData(handle))
    {
      cold->sourceFile = texture.name;
      cold->cacheKey = cacheKey;
      cold->textureDesc = texture.textureDesc;
    }
    if(auto* hot = m_texturePool.getHotData(handle))
    {
      hot->bindlessIndex = bindlessIndex;
    }

    m_textureCache[cacheKey] = handle;

    // Resolve materials waiting for this texture
    resolveWaitingMaterials_nolock(cacheKey);

    LOG_INFO("Texture '{}' created successfully (bindless: {})", texture.name, bindlessIndex);
    return handle;
  }

  std::vector<MeshHandle> ResourceManager::createMeshBatch(const std::vector<ProcessedMesh>& meshes)
  {
    std::vector<MeshHandle> handles;
    handles.reserve(meshes.size());

    for(const auto& mesh : meshes)
    {
      handles.push_back(createMesh(mesh));
    }

    return handles;
  }

  std::vector<MaterialHandle> ResourceManager::createMaterialBatch(const std::vector<ProcessedMaterial>& materials)
  {
    std::vector<MaterialHandle> handles;
    handles.reserve(materials.size());

    for(const auto& material : materials)
    {
      handles.push_back(createMaterial(material));
    }

    return handles;
  }

  std::vector<TextureHandle> ResourceManager::createTextureBatch(const std::vector<ProcessedTexture>& textures)
  {
    std::vector<TextureHandle> handles;
    handles.reserve(textures.size());

    for(const auto& texture : textures)
    {
      handles.push_back(createTexture(texture));
    }

    return handles;
  }

  // Asset data access methods
  const MeshGPUData* ResourceManager::getMesh(MeshHandle handle) const
  {
    return m_meshPool.getHotData(handle);
  }

  uint32_t ResourceManager::getMaterialIndex(MaterialHandle handle) const
  {
    const auto* hot = m_materialPool.getHotData(handle);
    return hot ? hot->materialOffset / sizeof(MaterialData) : 0;
  }

  uint32_t ResourceManager::getTextureBindlessIndex(TextureHandle handle) const
  {
    const auto* hot = m_texturePool.getHotData(handle);
    return hot ? hot->bindlessIndex : 0;
  }

  const Material* ResourceManager::getMaterialCPU(MaterialHandle handle) const
  {
    const auto* cold = m_materialPool.getColdData(handle);
    return cold ? &cold->material : nullptr;
  }

  bool ResourceManager::isMaterialTransparent(MaterialHandle handle) const
  {
    const auto* hot = m_materialPool.getHotData(handle);
    return hot ? hot->renderQueue == RenderQueue::Transparent : false;
  }

  // Asset management methods
  void ResourceManager::freeMesh(MeshHandle handle)
  {
    if(!m_meshPool.isValid(handle))
      return;

    // Get allocation data before destroying handle
    OffsetAllocator::Allocation vertexAlloc, indexAlloc;
    if(const auto* cold = m_meshPool.getColdData(handle))
    {
      vertexAlloc = cold->vertexMetadata;
      indexAlloc = cold->indexMetadata;
    }

    // Remove from cache
    {
      std::unique_lock lock(m_cacheMutex);
      std::erase_if(m_meshCache,
                    [handle](const auto& pair)
      {
        return pair.second == handle;
      });
    }

    // Free GPU memory
    {
      std::lock_guard meshLock(m_meshAllocatorMutex);
      if(vertexAlloc.isValid())
        m_vertexAllocator.free(vertexAlloc);
      if(indexAlloc.isValid())
        m_indexAllocator.free(indexAlloc);
    }

    m_meshPool.destroy(handle);
  }

  void ResourceManager::freeMaterial(MaterialHandle handle)
  {
    if(!m_materialPool.isValid(handle))
      return;

    // Get allocation data before destroying handle
    OffsetAllocator::Allocation materialAlloc;
    if(const auto* cold = m_materialPool.getColdData(handle))
    {
      materialAlloc = cold->metadata;
    }

    // Remove dependencies
    {
      std::lock_guard lock(m_dependencyMutex);
      for(auto& [cacheKey, materials] : m_unresolvedTextureToMaterials)
      {
        std::erase(materials, handle);
      }
      std::erase_if(m_unresolvedTextureToMaterials, [](const auto& pair) { return pair.second.empty(); });
    }

    // Remove from cache
    {
      std::unique_lock lock(m_cacheMutex);
      std::erase_if(m_materialCache,
                    [handle](const auto& pair)
      {
        return pair.second == handle;
      });
    }

    // Free GPU memory
    {
      std::lock_guard materialLock(m_materialAllocatorMutex);
      if(materialAlloc.isValid())
        m_materialAllocator.free(materialAlloc);
    }

    m_materialPool.destroy(handle);
  }

  void ResourceManager::freeTexture(TextureHandle handle)
  {
    if(!m_texturePool.isValid(handle))
      return;

    uint32_t bindlessIndex = getTextureBindlessIndex(handle);
    std::string cacheKey;

    if(const auto* cold = m_texturePool.getColdData(handle))
    {
      cacheKey = cold->cacheKey;
    }

    // Free GPU texture
    if(bindlessIndex != 0)
    {
      m_context->renderer->getGPUBuffers().deleteTexture(bindlessIndex);
    }

    // Remove from cache and dependencies
    {
      std::unique_lock lock(m_cacheMutex);
      if(!cacheKey.empty())
      {
        m_textureCache.erase(cacheKey);
      }
    }

    {
      std::lock_guard lock(m_dependencyMutex);
      m_unresolvedTextureToMaterials.erase(cacheKey);
    }

    m_texturePool.destroy(handle);
  }

  void ResourceManager::FlushUploads()
  {
    // Atomically acquire pending uploads
    std::vector<PendingMeshUpload> meshesToUpload;
    std::vector<PendingMaterialUpload> materialsToUpload;

    {
      std::lock_guard meshLock(m_pendingMeshesMutex);
      meshesToUpload.swap(m_pendingMeshes);
    }
    {
      std::lock_guard materialLock(m_pendingMaterialsMutex);
      materialsToUpload.swap(m_pendingMaterials);
    }

    // Early exit if nothing to upload
    if(meshesToUpload.empty() && materialsToUpload.empty())
    {
      return;
    }

    // Process uploads
    if(!meshesToUpload.empty())
    {
      LOG_INFO("Flushing {} meshes...", meshesToUpload.size());
      uploadMeshBatch(meshesToUpload);
    }

    if(!materialsToUpload.empty())
    {
      LOG_INFO("Flushing {} materials...", materialsToUpload.size());
      uploadMaterialBatch(materialsToUpload);
    }
  }

  // Private helper methods
  MaterialData ResourceManager::generateMaterialDataWithTextures_nolock(const Material& material)
  {
    MaterialData data;

    data.baseColorFactor = material.baseColorFactor;
    data.metallicRoughnessNormalOcclusion = vec4(material.metallicFactor, material.roughnessFactor, material.normalScale, material.occlusionStrength);
    data.emissiveFactorAlphaCutoff = vec4(material.emissiveFactor.x, material.emissiveFactor.y, material.emissiveFactor.z, material.alphaCutoff);

    // Resolve texture indices
    data.baseColorTexture = resolveTextureIndex_nolock(material.baseColorTexture);
    data.metallicRoughnessTexture = resolveTextureIndex_nolock(material.metallicRoughnessTexture);
    data.normalTexture = resolveTextureIndex_nolock(material.normalTexture);
    data.emissiveTexture = resolveTextureIndex_nolock(material.emissiveTexture);
    data.occlusionTexture = resolveTextureIndex_nolock(material.occlusionTexture);

    data.materialTypeFlags = material.materialTypeFlags;
    data.flags = material.flags;

    return data;
  }

  uint32_t ResourceManager::resolveTextureIndex_nolock(const MaterialTexture& matTex)
  {
    if(!matTex.hasTexture())
      return 0;

    std::string cacheKey = TextureCacheKeyGenerator::generateKey(matTex.source);
    if(auto it = m_textureCache.find(cacheKey); it != m_textureCache.end())
    {
      return getTextureBindlessIndex(it->second);
    }
    return 0;
  }

  void ResourceManager::updateMaterialTextureDependencies_nolock(MaterialHandle materialHandle, const ProcessedMaterial& material)
  {
    std::lock_guard depLock(m_dependencyMutex);

    // Remove old dependencies for this material
    for(auto& [cacheKey, materials] : m_unresolvedTextureToMaterials)
    {
      std::erase(materials, materialHandle);
    }
    std::erase_if(m_unresolvedTextureToMaterials, [](const auto& pair) { return pair.second.empty(); });

    // Add dependencies for unresolved textures
    auto addDependencyIfUnresolved = [&](const MaterialTexture& matTex)
    {
      if(!matTex.hasTexture())
        return;

      std::string textureCacheKey = TextureCacheKeyGenerator::generateKey(matTex.source);
      if(m_textureCache.find(textureCacheKey) == m_textureCache.end())
      {
        m_unresolvedTextureToMaterials[textureCacheKey].push_back(materialHandle);
      }
    };

    addDependencyIfUnresolved(material.baseColorTexture);
    addDependencyIfUnresolved(material.metallicRoughnessTexture);
    addDependencyIfUnresolved(material.normalTexture);
    addDependencyIfUnresolved(material.emissiveTexture);
    addDependencyIfUnresolved(material.occlusionTexture);
  }

  void ResourceManager::resolveWaitingMaterials_nolock(const std::string& textureCacheKey)
  {
    std::vector<MaterialHandle> waitingMaterials;

    {
      std::lock_guard depLock(m_dependencyMutex);
      if(auto it = m_unresolvedTextureToMaterials.find(textureCacheKey); it != m_unresolvedTextureToMaterials.end())
      {
        waitingMaterials = std::move(it->second);
        m_unresolvedTextureToMaterials.erase(it);
      }
    }

    if(waitingMaterials.empty())
      return;

    LOG_INFO("Resolving {} waiting materials for texture '{}'", waitingMaterials.size(), textureCacheKey);

    // Re-generate GPU data and queue uploads
    {
      std::lock_guard pendingLock(m_pendingMaterialsMutex);
      for(MaterialHandle materialHandle : waitingMaterials)
      {
        if(auto* cold = m_materialPool.getColdData(materialHandle))
        {
          MaterialData newGpuData = generateMaterialDataWithTextures_nolock(cold->material);
          m_pendingMaterials.push_back({ newGpuData, cold->metadata });
        }
      }
    }
  }

  void ResourceManager::uploadMeshBatch(const std::vector<PendingMeshUpload>& meshes)
  {
    auto& gpuBuffers = m_context->renderer->getGPUBuffers();

    // Upload vertices
    uploadContiguousBatches(meshes, gpuBuffers.GetVertexBuffer(), [](const PendingMeshUpload& mesh) { return mesh.vertexAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.data.vertices); });

    // Upload indices
    uploadContiguousBatches(meshes, gpuBuffers.GetIndexBuffer(), [](const PendingMeshUpload& mesh) { return mesh.indexAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.data.indices); });
  }

  void ResourceManager::uploadMaterialBatch(std::vector<PendingMaterialUpload>& materials)
  {
    auto& gpuBuffers = m_context->renderer->getGPUBuffers();

    deduplicateMaterialUploads(materials);

    uploadContiguousBatches(materials, gpuBuffers.GetMaterialBuffer(), [](const PendingMaterialUpload& mat) { return mat.materialAlloc; }, [](const PendingMaterialUpload& mat) { return std::span(&mat.data, 1); });
  }

  template <typename Container, typename AllocSelector, typename DataSelector>
  void ResourceManager::uploadContiguousBatches(const Container& items, vk::BufferHandle targetBuffer, AllocSelector&& allocSelector, DataSelector&& dataSelector)
  {
    if(items.empty())
      return;

    auto sortedItems = items;
    std::sort(sortedItems.begin(), sortedItems.end(),
              [&](const auto& a, const auto& b)
    {
      return allocSelector(a).offset < allocSelector(b).offset;
    });

    constexpr size_t MAX_BATCH_SIZE = 16 * 1024 * 1024; // 16MB
    std::vector<uint8_t> batchBuffer;
    batchBuffer.reserve(4 * 1024 * 1024);

    auto& gpuBuffers = m_context->renderer->getGPUBuffers();
    uint32_t batchStartOffset = 0;
    bool batchActive = false;

    auto flushBatch = [&]()
    {
      if(!batchBuffer.empty() && batchActive)
      {
        if(!gpuBuffers.uploadtoBuffer(targetBuffer, batchStartOffset, batchBuffer.data(), batchBuffer.size()))
        {
          LOG_ERROR("Failed to upload batch at offset {}, size {}", batchStartOffset, batchBuffer.size());
        }
        batchBuffer.clear();
        batchActive = false;
      }
    };

    for(const auto& item : sortedItems)
    {
      const auto alloc = allocSelector(item);
      const auto data = dataSelector(item);
      const size_t dataSize = data.size() * sizeof(typename decltype(data)::element_type);

      const bool isContiguous = batchActive && (alloc.offset == batchStartOffset + batchBuffer.size());

      if(!isContiguous || batchBuffer.size() + dataSize > MAX_BATCH_SIZE)
      {
        flushBatch();
        batchStartOffset = alloc.offset;
        batchActive = true;
      }

      const uint8_t* byteData = reinterpret_cast<const uint8_t*>(data.data());
      batchBuffer.insert(batchBuffer.end(), byteData, byteData + dataSize);
    }

    flushBatch();
  }

  void ResourceManager::deduplicateMaterialUploads(std::vector<PendingMaterialUpload>& materials)
  {
    if(materials.size() <= 1)
      return;

    std::sort(materials.begin(), materials.end(),
              [](const auto& a, const auto& b)
    {
      return a.materialAlloc.offset < b.materialAlloc.offset;
    });

    // Remove duplicates, keeping the last (most recent) update for each offset
    auto newEnd = std::unique(materials.begin(), materials.end(),
                              [](const auto& a, const auto& b)
    {
      return a.materialAlloc.offset == b.materialAlloc.offset;
    });
    materials.erase(newEnd, materials.end());
  }

  // Cache key generation
  std::string ResourceManager::generateMeshCacheKey(const ProcessedMesh& mesh)
  {
    size_t hash = std::hash<std::string>{}(mesh.name);
    hash ^= std::hash<size_t>{}(mesh.vertices.size()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= std::hash<size_t>{}(mesh.indices.size()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return mesh.name + "_" + std::to_string(hash);
  }

  std::string ResourceManager::generateMaterialCacheKey(const ProcessedMaterial& material)
  {
    size_t hash = std::hash<std::string>{}(material.name);
    hash ^= std::hash<float>{}(material.metallicFactor) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= std::hash<float>{}(material.roughnessFactor) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return material.name + "_" + std::to_string(hash);
  }

  std::string ResourceManager::generateTextureCacheKey(const ProcessedTexture& texture)
  {
    return TextureCacheKeyGenerator::generateKey(texture.source);
  }
} // namespace AssetLoading
