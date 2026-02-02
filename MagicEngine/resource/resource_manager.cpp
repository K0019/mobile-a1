#include "resource_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <numeric>
#include <span>
#include <stdexcept>
#include <stb_image.h>

#include "core/engine/engine.h"
#include "resource/animation_clip.h"
#include "resource/morph_set.h"
#include "resource/skeleton.h"
#include "renderer/ui/font_system.h"
#include "VFS/VFS.h"

// hina-vk integration
#include "renderer/gfx_renderer.h"
#include "renderer/gfx_mesh_storage.h"
#include "renderer/gfx_material_system.h"

namespace
{
  constexpr float MORPH_DELTA_EPSILON = 1e-6f;
  constexpr float MORPH_DELTA_EPSILON_SQ = MORPH_DELTA_EPSILON * MORPH_DELTA_EPSILON;
  constexpr const char* kDefaultUIFontPath = "assets/fonts/DefaultUI.ttf";

  // gfx::Format values match hina_format values, so just cast
  hina_format gfxFormatToHinaFormat(gfx::Format format)
  {
    return static_cast<hina_format>(format);
  }

  std::vector<uint8_t> LoadBinaryFile(const std::string& vfsPath)
  {
    std::vector<uint8_t> data;
    if(!VFS::ReadFile(vfsPath, data))
    {
      LOG_WARNING("Failed to read file via VFS: {}", vfsPath);
      return {};
    }
    return data;
  }

  bool isDeltaNonZero(const Resource::MorphTargetVertexDelta& delta)
  {
    const float pos = glm::dot(delta.deltaPosition, delta.deltaPosition);
    const float norm = glm::dot(delta.deltaNormal, delta.deltaNormal);
    const float tan = glm::dot(delta.deltaTangent, delta.deltaTangent);
    return pos > MORPH_DELTA_EPSILON_SQ || norm > MORPH_DELTA_EPSILON_SQ || tan > MORPH_DELTA_EPSILON_SQ;
  }

  std::vector<GPUSkinningData> buildSkinningBuffer(const std::vector<Resource::SkinningData>& skinning)
  {
    std::vector<GPUSkinningData> result;
    result.reserve(skinning.size());
    for(const auto& src : skinning)
    {
      GPUSkinningData dst;
      dst.indices = glm::uvec4(src.boneIndices[0], src.boneIndices[1], src.boneIndices[2], src.boneIndices[3]);
      dst.weights = glm::vec4(src.weights[0], src.weights[1], src.weights[2], src.weights[3]);
      result.push_back(dst);
    }
    return result;
  }

  // Convert GPUSkinningData (32 bytes) to VertexSkinning (8 bytes) for mobile-optimized rendering
  std::vector<gfx::VertexSkinning> buildCompactSkinningBuffer(const std::vector<GPUSkinningData>& gpuSkinning)
  {
    std::vector<gfx::VertexSkinning> result;
    result.reserve(gpuSkinning.size());
    for (const auto& src : gpuSkinning)
    {
      gfx::VertexSkinning dst;
      // Pack bone indices (clamp to 255, should be well under 64)
      dst.boneIndices[0] = static_cast<uint8_t>(std::min(src.indices.x, 255u));
      dst.boneIndices[1] = static_cast<uint8_t>(std::min(src.indices.y, 255u));
      dst.boneIndices[2] = static_cast<uint8_t>(std::min(src.indices.z, 255u));
      dst.boneIndices[3] = static_cast<uint8_t>(std::min(src.indices.w, 255u));
      // Pack weights as UNORM8 (0.0-1.0 -> 0-255)
      dst.weights[0] = static_cast<uint8_t>(std::clamp(src.weights.x * 255.0f + 0.5f, 0.0f, 255.0f));
      dst.weights[1] = static_cast<uint8_t>(std::clamp(src.weights.y * 255.0f + 0.5f, 0.0f, 255.0f));
      dst.weights[2] = static_cast<uint8_t>(std::clamp(src.weights.z * 255.0f + 0.5f, 0.0f, 255.0f));
      dst.weights[3] = static_cast<uint8_t>(std::clamp(src.weights.w * 255.0f + 0.5f, 0.0f, 255.0f));
      result.push_back(dst);
    }
    return result;
  }

  void buildMorphBuffers(const Resource::ProcessedMesh& mesh,
                         std::vector<GPUMorphDelta>& outDeltas,
                         std::vector<uint32_t>& outStarts,
                         std::vector<uint32_t>& outCounts,
                         std::vector<MeshMorphTargetInfo>& outInfos)
  {
    constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();
    const size_t vertexCount = mesh.vertices.size();
    outDeltas.clear();
    outStarts.assign(vertexCount, INVALID);
    outCounts.assign(vertexCount, INVALID);
    outInfos.clear();
    outInfos.reserve(mesh.morphTargets.size());

    std::vector<std::vector<GPUMorphDelta>> perVertexDeltas(vertexCount);

    for(uint32_t targetIdx = 0; targetIdx < mesh.morphTargets.size(); ++targetIdx)
    {
      const auto& target = mesh.morphTargets[targetIdx];
      outInfos.push_back({ target.name, targetIdx });

      if(target.deltas.size() != vertexCount)
        continue;

      for(uint32_t v = 0; v < vertexCount; ++v)
      {
        const auto& delta = target.deltas[v];
        if(!isDeltaNonZero(delta))
          continue;

        GPUMorphDelta gpuDelta;
        gpuDelta.morphTargetIndex = targetIdx;
        gpuDelta.vertexIndex = v;
        gpuDelta.deltaPosition = glm::vec4(delta.deltaPosition, 0.0f);
        gpuDelta.deltaNormal = glm::vec4(delta.deltaNormal, 0.0f);
        gpuDelta.deltaTangent = glm::vec4(delta.deltaTangent, 0.0f);
        perVertexDeltas[v].push_back(gpuDelta);
      }
    }

    size_t totalDeltaCount = 0;
    for(const auto& bucket : perVertexDeltas)
    {
      totalDeltaCount += bucket.size();
    }
    outDeltas.reserve(totalDeltaCount);

    for(uint32_t v = 0; v < vertexCount; ++v)
    {
      const auto& bucket = perVertexDeltas[v];
      if(bucket.empty())
        continue;

      outStarts[v] = static_cast<uint32_t>(outDeltas.size());
      outCounts[v] = static_cast<uint32_t>(bucket.size());
      outDeltas.insert(outDeltas.end(), bucket.begin(), bucket.end());
    }
  }
} // namespace

namespace Resource
{
  ResourceManager::ResourceManager()
  {
    m_clips.emplace_back();
    m_clipGenerations.push_back(1);  // Generation for reserved slot (index 0)
    m_skeletons.emplace_back();
    m_morphSets.emplace_back();
  }

  void ResourceManager::initialize(Context* context)
  {
    this->m_context = context;

    LOG_INFO("Resource Manager initialized - {}M vertices, {}M indices, {}K materials", ResourceLimits::MAX_VERTICES / 1'000'000, ResourceLimits::MAX_INDICES / 1'000'000, ResourceLimits::MAX_MATERIALS / 1000);
  }

  void ResourceManager::startUploadThread()
  {
    if (m_uploadThreadRunning.load()) return;

    m_uploadThreadStop.store(false);
    m_uploadThread = std::thread(&ResourceManager::uploadThreadFunc, this);
    m_uploadThreadRunning.store(true);
    LOG_INFO("Upload thread started");
  }

  void ResourceManager::uploadThreadFunc()
  {
    // Create thread-local hina context for GPU uploads
    hina_context* threadCtx = hina_create_thread_context();
    if (!threadCtx) {
      LOG_ERROR("Failed to create upload thread context — async uploads disabled");
      return;
    }
    LOG_INFO("Upload thread context created");

    while (!m_uploadThreadStop.load()) {
      UploadJob job;
      {
        std::unique_lock<std::mutex> lock(m_uploadQueueMutex);
        m_uploadQueueCV.wait(lock, [this] {
          return m_uploadThreadStop.load() || !m_uploadQueue.empty();
        });

        if (m_uploadThreadStop.load() && m_uploadQueue.empty())
          break;

        job = std::move(m_uploadQueue.front());
        m_uploadQueue.erase(m_uploadQueue.begin());
      }

      // Execute the job with our thread context
      job(threadCtx);
    }

    // Drain remaining jobs before exit
    {
      std::lock_guard<std::mutex> lock(m_uploadQueueMutex);
      for (auto& job : m_uploadQueue) {
        job(threadCtx);
      }
      m_uploadQueue.clear();
    }

    hina_destroy_thread_context(threadCtx);
    LOG_INFO("Upload thread exiting");
  }

  void ResourceManager::postRendererInitialize()
  {
    //loadDefaultUIFont();
  }

  void ResourceManager::loadDefaultUIFont()
  {
      //I legit swtg this entire filesystem and the working directory shit legit pisses me off holy shit how difficult is it to just hardcode the path,
      //leaving this out because legit i cannot trust that the file will be in that place.

    if(m_defaultUIFont.isValid())
      return;

    if(!m_context || !m_context->renderer)
    {
      throw std::runtime_error("ResourceManager: Renderer is not ready for default UI font creation");
    }

    std::vector<uint8_t> fontBytes = LoadBinaryFile(kDefaultUIFontPath);
    if(fontBytes.empty())
    {
      throw std::runtime_error("ResourceManager: Default UI font '" + std::string(kDefaultUIFontPath) + "' is missing");
    }

    ProcessedFont processed;
    processed.name = "DefaultUIFont";
    processed.fontFileData = std::move(fontBytes);
    processed.buildSettings.pixelHeight = 20.0f;
    processed.buildSettings.firstCodepoint = 32;
    processed.buildSettings.lastCodepoint = 255;
    processed.buildSettings.fallbackCodepoint = '?';
    processed.buildSettings.extraCodepoints.push_back(0x2026u);
    processed.sourceFile = kDefaultUIFontPath;

    m_defaultUIFont = createFont(processed);
    if(!m_defaultUIFont.isValid())
    {
      throw std::runtime_error("ResourceManager: Failed to create default UI font atlas");
    }

    LOG_INFO("Default UI font '{}' loaded", kDefaultUIFontPath);
  }

  ResourceManager::~ResourceManager() = default;

  void ResourceManager::shutdown()
  {
    // Stop upload thread (drains remaining jobs, destroys hina_context)
    if (m_uploadThreadRunning.load()) {
      m_uploadThreadStop.store(true);
      m_uploadQueueCV.notify_one();
      if (m_uploadThread.joinable()) {
        m_uploadThread.join();
      }
      m_uploadThreadRunning.store(false);
    }

    // Wait for all pending results to be consumed
    {
      std::lock_guard<std::mutex> lock(m_pendingUploadsMutex);
      for (auto& pending : m_pendingMeshUploads) {
        if (pending.future.valid()) pending.future.wait();
      }
      m_pendingMeshUploads.clear();
    }
    for (auto& pending : m_pendingTextureUploads) {
      if (pending.future.valid()) pending.future.wait();
    }
    m_pendingTextureUploads.clear();

    m_meshPool.clear();
    m_texturePool.clear();
    m_materialPool.clear();
    m_fontPool.clear();
    m_defaultUIFont = {};
    LOG_INFO("Resource Manager shutdown complete");
  }

  void ResourceManager::pollAsyncUploads()
  {
    // --- Meshes ---
    {
      std::lock_guard<std::mutex> lock(m_pendingUploadsMutex);
      auto it = m_pendingMeshUploads.begin();
      while (it != m_pendingMeshUploads.end()) {
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          MeshUploadResult result = it->future.get();
          if (result.valid) {
            if (auto* hot = m_meshPool.getHotData(it->handle)) {
              hot->gfxMeshIndex = result.gfxMeshIndex;
              hot->gfxMeshGeneration = result.gfxMeshGeneration;
              hot->hasGfxMesh = true;
              LOG_INFO("Async mesh upload completed '{}': index={} gen={} skinned={}",
                       it->meshName, result.gfxMeshIndex, result.gfxMeshGeneration, result.hasSkinning);
            }
            if (m_context && m_context->renderer) {
              m_context->renderer->recordMeshUpload(result.vertexCount, result.indexCount, result.totalBytes);
            }
          } else {
            LOG_WARNING("Async mesh upload failed for '{}'", it->meshName);
          }
          it = m_pendingMeshUploads.erase(it);
        } else {
          ++it;
        }
      }
    }

    // --- Textures ---
    {
      auto it = m_pendingTextureUploads.begin();
      while (it != m_pendingTextureUploads.end()) {
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          TextureUploadResult result = it->future.get();
          if (result.valid) {
            if (m_context && m_context->renderer) {
              auto* gfxRenderer = m_context->renderer;
              auto& materialSystem = gfxRenderer->getMaterialSystem();

              gfx::TextureHandle gfxTexture = materialSystem.registerPreCreatedTexture(
                {result.textureId}, {result.viewId}, result.width, result.height,
                static_cast<hina_format>(result.hinaFormat), result.isSRGB);

              if (auto* hot = m_texturePool.getHotData(it->handle)) {
                if (gfxTexture.isValid()) {
                  hot->hasGfxTexture = true;
                  hot->gfxTextureIndex = gfxTexture.index;
                  hot->gfxTextureGeneration = gfxTexture.generation;
                  LOG_INFO("Async texture upload completed '{}': gfx={}:{}",
                           it->textureName, gfxTexture.index, gfxTexture.generation);
                } else {
                  LOG_WARNING("Failed to register async texture '{}' in GfxMaterialSystem", it->textureName);
                }
              }
            }
            resolveWaitingMaterials_nolock(it->cacheKey);
          } else {
            LOG_WARNING("Async texture upload failed for '{}'", it->textureName);
          }
          it = m_pendingTextureUploads.erase(it);
        } else {
          ++it;
        }
      }
    }

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

    // Prepare animation GPU payloads on main thread (small data)
    std::vector<GPUSkinningData> gpuSkinning = buildSkinningBuffer(mesh.skinning);
    std::vector<GPUMorphDelta> gpuMorphDeltas;
    std::vector<uint32_t> morphVertexStarts;
    std::vector<uint32_t> morphVertexCounts;
    std::vector<MeshMorphTargetInfo> morphInfos;
    if(!mesh.morphTargets.empty())
    {
      buildMorphBuffers(mesh, gpuMorphDeltas, morphVertexStarts, morphVertexCounts, morphInfos);
    }
    if(gpuMorphDeltas.empty())
    {
      morphVertexStarts.clear();
      morphVertexCounts.clear();
    }

    // Create handle and populate cold + partial hot data (main thread)
    MeshHandle handle = m_meshPool.create();
    if(auto* cold = m_meshPool.getColdData(handle))
    {
      cold->meshName = mesh.name;
      cold->jointParentIndices = mesh.skeleton.parentIndices;
      cold->jointInverseBindMatrices = mesh.skeleton.inverseBindMatrices;
      cold->jointBindPoseMatrices = mesh.skeleton.bindPoseMatrices;
      cold->jointNames = mesh.skeleton.jointNames;
      cold->morphTargets = morphInfos;
      cold->skeletonId = registerSkeleton(mesh.skeleton);
      cold->morphSetId = registerMorphSet(morphInfos);
    }
    if(auto* hot = m_meshPool.getHotData(handle))
    {
      hot->vertexCount = static_cast<uint32_t>(mesh.vertices.size());
      hot->indexCount = static_cast<uint32_t>(mesh.indices.size());
      hot->bounds = mesh.bounds;

      auto& anim = hot->animation;
      anim.jointCount = static_cast<uint32_t>(mesh.skeleton.parentIndices.size());
      anim.skinningByteOffset = UINT32_MAX;
      anim.morphDeltaByteOffset = UINT32_MAX;
      anim.morphDeltaCount = static_cast<uint32_t>(gpuMorphDeltas.size());
      anim.morphVertexBaseOffset = UINT32_MAX;
      anim.morphVertexCountOffset = UINT32_MAX;
      anim.morphTargetCount = static_cast<uint32_t>(mesh.morphTargets.size());
      // hasGfxMesh stays false until async upload completes
    }

    m_meshCache[cacheKey] = handle;
    cacheLock.unlock();

    // Queue async upload (heavy CPU + GPU work on upload thread)
    if (m_context && m_context->renderer && m_uploadThreadRunning.load()) {
      auto vertices = mesh.vertices;
      auto indices = mesh.indices;
      std::string meshName = mesh.name;

      std::promise<MeshUploadResult> promise;
      auto future = promise.get_future();

      auto job = [
        this,
        vertices = std::move(vertices),
        indices = std::move(indices),
        gpuSkinning = std::move(gpuSkinning),
        promise = std::make_shared<std::promise<MeshUploadResult>>(std::move(promise))
      ](void* /*ctx*/) mutable {
        auto* gfxRenderer = m_context->renderer;
        auto& meshStorage = gfxRenderer->getMeshStorage();

        static_assert(sizeof(Vertex) == sizeof(gfx::FullVertex), "Vertex layouts must match");
        const auto* gfxVertices = reinterpret_cast<const gfx::FullVertex*>(vertices.data());

        gfx::MeshHandle gfxMesh;
        uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
        uint32_t indexCount = static_cast<uint32_t>(indices.size());
        bool hasSkinning = !gpuSkinning.empty();

        if (hasSkinning) {
          auto compactSkinning = buildCompactSkinningBuffer(gpuSkinning);
          std::vector<gfx::VertexPosition> positions(vertexCount);
          std::vector<gfx::VertexAttributes> attributes(vertexCount);
          glm::vec3 minPos(std::numeric_limits<float>::max());
          glm::vec3 maxPos(std::numeric_limits<float>::lowest());

          for (size_t i = 0; i < vertices.size(); ++i) {
            const auto& v = gfxVertices[i];
            positions[i] = gfx::VertexPosition(v.position);
            float bitangentSign = v.tangent.w;
            glm::vec3 tangent = glm::vec3(v.tangent);
            attributes[i].pack(v.normal, tangent, bitangentSign, v.getUV(), 0);
            minPos = glm::min(minPos, v.position);
            maxPos = glm::max(maxPos, v.position);
          }

          gfx::MeshBounds bounds;
          bounds.aabbMin = minPos;
          bounds.aabbMax = maxPos;
          bounds.center = (minPos + maxPos) * 0.5f;
          bounds.radius = glm::length(maxPos - bounds.center);

          gfxMesh = meshStorage.uploadPackedSkinned(
            positions.data(), attributes.data(), compactSkinning.data(),
            vertexCount, indices.data(), indexCount, bounds);
        } else {
          gfxMesh = meshStorage.upload(gfxVertices, vertexCount, indices.data(), indexCount);
        }

        uint32_t totalBytes = vertexCount * (sizeof(gfx::VertexPosition) + sizeof(gfx::VertexAttributes));
        totalBytes += indexCount * sizeof(uint32_t);
        if (hasSkinning) totalBytes += vertexCount * sizeof(gfx::VertexSkinning);

        MeshUploadResult result;
        result.gfxMeshIndex = gfxMesh.index;
        result.gfxMeshGeneration = gfxMesh.generation;
        result.vertexCount = vertexCount;
        result.indexCount = indexCount;
        result.totalBytes = totalBytes;
        result.hasSkinning = hasSkinning;
        result.valid = gfxMesh.isValid();
        promise->set_value(result);
      };

      {
        std::lock_guard<std::mutex> lock(m_pendingUploadsMutex);
        m_pendingMeshUploads.push_back({std::move(future), handle, meshName});
      }
      {
        std::lock_guard<std::mutex> lock(m_uploadQueueMutex);
        m_uploadQueue.push_back(std::move(job));
      }
      m_uploadQueueCV.notify_one();
    }

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

    // Create handle and populate data
    // Note: GPU memory is now managed by GfxMaterialSystem, not ResourceManager allocators
    Material mat = convertToStoredMaterial(material);
    MaterialHandle handle = m_materialPool.create();

    uint32_t alphaMode = material.flags & MaterialFlags::ALPHA_MODE_MASK;

    if(auto* cold = m_materialPool.getColdData(handle))
    {
      cold->material = mat;
    }
    if(auto* hot = m_materialPool.getHotData(handle))
    {
      hot->renderQueue = (alphaMode == ALPHA_MODE_BLEND)
        ? RenderQueue::Transparent
        : RenderQueue::Opaque;
    }
    // Generate GPU data and handle dependencies
    MaterialData gpuData = generateMaterialDataWithTextures_nolock(mat);
    updateMaterialTextureDependencies_nolock(handle, material);

    // Upload to GfxMaterialSystem directly (hina-vk path)
    if (m_context && m_context->renderer) {
      if (auto* gfxRenderer = m_context->renderer) {
        auto& materialSystem = gfxRenderer->getMaterialSystem();

        // Convert Material to gfx::GfxMaterial
        gfx::GfxMaterial gfxMat;
        gfxMat.baseColor = mat.baseColorFactor;
        gfxMat.roughness = mat.roughnessFactor;
        gfxMat.metallic = mat.metallicFactor;
        gfxMat.emissive = glm::length(mat.emissiveFactor) > 0.0f ? 1.0f : 0.0f;
        gfxMat.ao = mat.occlusionStrength;
        gfxMat.rim = 0.0f;

        // Set alpha mode
        switch (mat.alphaMode) {
          case AlphaMode::Opaque:
            gfxMat.alphaMode = gfx::GfxMaterial::AlphaMode::Opaque;
            break;
          case AlphaMode::Mask:
            gfxMat.alphaMode = gfx::GfxMaterial::AlphaMode::Mask;
            break;
          case AlphaMode::Blend:
            gfxMat.alphaMode = gfx::GfxMaterial::AlphaMode::Blend;
            break;
        }
        gfxMat.alphaCutoff = mat.alphaCutoff;
        gfxMat.doubleSided = (mat.flags & DOUBLE_SIDED) != 0;

        // Helper to resolve gfx::TextureHandle from MaterialTexture
        auto resolveGfxTexture = [this, &material](const MaterialTexture& matTex, const char* texType) -> gfx::TextureHandle {
          if (!matTex.hasTexture()) return {};
          std::string cacheKey = TextureCacheKeyGenerator::generateKey(matTex.source);
          auto it = m_textureCache.find(cacheKey);
          if (it != m_textureCache.end()) {
            if (const auto* hot = m_texturePool.getHotData(it->second)) {
              if (hot->hasGfxTexture) {
                gfx::TextureHandle gfxTex;
                gfxTex.index = hot->gfxTextureIndex;
                gfxTex.generation = hot->gfxTextureGeneration;
                return gfxTex;
              } else {
                LOG_WARNING("Material '{}' {} texture not in GfxMaterialSystem (key='{}')", material.name, texType, cacheKey);
              }
            }
          } else {
            LOG_WARNING("Material '{}' {} texture not in cache (key='{}')", material.name, texType, cacheKey);
          }
          return {};
        };

        // Look up texture handles
        gfxMat.albedoTexture = resolveGfxTexture(mat.baseColorTexture, "albedo");
        gfxMat.normalTexture = resolveGfxTexture(mat.normalTexture, "normal");
        gfxMat.metallicRoughnessTexture = resolveGfxTexture(mat.metallicRoughnessTexture, "metallicRoughness");
        gfxMat.emissiveTexture = resolveGfxTexture(mat.emissiveTexture, "emissive");
        gfxMat.occlusionTexture = resolveGfxTexture(mat.occlusionTexture, "occlusion");

        LOG_DEBUG("Material '{}' baseColor=({:.2f},{:.2f},{:.2f},{:.2f}), albedo={}:{}, normal={}:{}",
            material.name, gfxMat.baseColor.r, gfxMat.baseColor.g, gfxMat.baseColor.b, gfxMat.baseColor.a,
            gfxMat.albedoTexture.index, gfxMat.albedoTexture.generation,
            gfxMat.normalTexture.index, gfxMat.normalTexture.generation);

        gfx::MaterialHandle gfxMaterial = materialSystem.createMaterial(gfxMat);

        if (auto* hotData = m_materialPool.getHotData(handle)) {
          if (gfxMaterial.isValid()) {
            hotData->hasGfxMaterial = true;
            hotData->gfxMaterialIndex = gfxMaterial.index;
            hotData->gfxMaterialGeneration = gfxMaterial.generation;
            LOG_DEBUG("Uploaded material '{}' to GfxMaterialSystem: {}:{}",
                      material.name, gfxMaterial.index, gfxMaterial.generation);
          } else {
            LOG_WARNING("Failed to upload material '{}' to GfxMaterialSystem", material.name);
          }
        }
      }
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

    // Double-check pattern with unique lock
    std::unique_lock cacheLock(m_cacheMutex);
    if(auto it = m_textureCache.find(cacheKey); it != m_textureCache.end())
    {
      return it->second;
    }

    // Create handle and populate cold data (main thread)
    TextureHandle handle = m_texturePool.create();
    if(auto* cold = m_texturePool.getColdData(handle))
    {
      cold->sourceFile = texture.name;
      cold->cacheKey = cacheKey;
      cold->textureDesc = texture.textureDesc;
      cold->isSRGB = texture.sRGB;
    }

    m_textureCache[cacheKey] = handle;
    cacheLock.unlock();

    // Queue async GPU upload on the upload thread
    if (m_context && m_context->renderer && !texture.data.empty() && m_uploadThreadRunning.load()) {
      gfx::Format gfxFormat = texture.textureDesc.format;
      if (texture.sRGB && !gfx::isFormatSRGB(gfxFormat)) {
        gfx::Format srgbFormat = gfx::convertFormatSRGB(gfxFormat, true);
        if (srgbFormat != gfxFormat) {
          LOG_DEBUG("Texture '{}': Converting format {} -> {} (sRGB requested)",
              texture.name, static_cast<int>(gfxFormat), static_cast<int>(srgbFormat));
          gfxFormat = srgbFormat;
        }
      }
      hina_format hinaFormat = gfxFormatToHinaFormat(gfxFormat);

      LOG_INFO("Texture '{}' async upload queued: hinaFmt={}, size={}x{}, dataSize={}, sRGB={}",
          texture.name, static_cast<int>(hinaFormat),
          texture.width, texture.height, texture.data.size(), texture.sRGB);

      auto pixelData = texture.data;
      uint32_t width = texture.width;
      uint32_t height = texture.height;
      bool isSRGB = texture.sRGB;
      std::string textureName = texture.name;

      auto promise = std::make_shared<std::promise<TextureUploadResult>>();
      auto future = promise->get_future();

      auto job = [
        pixelData = std::move(pixelData),
        width, height, hinaFormat, isSRGB, textureName,
        promise
      ](void* ctx) mutable {
        auto* threadCtx = static_cast<hina_context*>(ctx);
        TextureUploadResult result;

        hina_texture_desc desc = hina_texture_desc_default();
        desc.width = width;
        desc.height = height;
        desc.format = hinaFormat;
        desc.mip_levels = 1;
        desc.usage = HINA_TEXTURE_SAMPLED_BIT;
        desc.initial_data = pixelData.data();
        desc.label = textureName.c_str();

        hina_texture tex = hina_ctx_make_texture(threadCtx, &desc);
        if (!hina_texture_is_valid(tex)) {
          LOG_WARNING("Async texture upload failed for '{}'", textureName);
          promise->set_value(result);
          return;
        }

        hina_ctx_flush_uploads(threadCtx);
        hina_texture_view view = hina_texture_get_default_view(tex);

        result.textureId = tex.id;
        result.viewId = view.id;
        result.width = width;
        result.height = height;
        result.hinaFormat = static_cast<uint32_t>(hinaFormat);
        result.isSRGB = isSRGB;
        result.valid = true;
        promise->set_value(result);
      };

      m_pendingTextureUploads.push_back({std::move(future), handle, textureName, cacheKey});
      {
        std::lock_guard<std::mutex> lock(m_uploadQueueMutex);
        m_uploadQueue.push_back(std::move(job));
      }
      m_uploadQueueCV.notify_one();
    }

    return handle;
  }

  FontHandle ResourceManager::createFont(const ProcessedFont& font)
  {
    if(!font.isValid())
    {
      LOG_ERROR("ResourceManager: Invalid font request");
      return {};
    }

    const std::string cacheKey = generateFontCacheKey(font);
    {
      std::shared_lock cacheRead(m_cacheMutex);
      auto it = m_fontCache.find(cacheKey);
      if(it != m_fontCache.end())
      {
        return it->second;
      }
    }

    // CPU rasterization (main thread — this is fast, GPU upload is what we want async)
    Resource::FontCPUData cpuData = ui::FontSystem::BuildFontCPUData(font.fontFileData, font.buildSettings,
                                                                     font.mergeSources, font.name);
    if(cpuData.atlasPixelsRGBA.empty() || cpuData.atlasWidth == 0 || cpuData.atlasHeight == 0)
    {
      LOG_ERROR("ResourceManager: Failed to build font atlas '{}'", font.name);
      return {};
    }

    // Create atlas texture (GPU upload is async via createTexture -> upload thread)
    ProcessedTexture atlasTexture;
    atlasTexture.name = font.name + "_FontAtlas";
    atlasTexture.textureDesc.type = gfx::TextureType::Tex2D;
    atlasTexture.textureDesc.format = gfx::Format::RGBA8_UNorm;
    atlasTexture.textureDesc.dimensions.width = cpuData.atlasWidth;
    atlasTexture.textureDesc.dimensions.height = cpuData.atlasHeight;
    atlasTexture.textureDesc.dimensions.depth = 1u;
    atlasTexture.textureDesc.numMipLevels = 1;
    atlasTexture.textureDesc.usage = gfx::TextureUsage::Sampled;
    atlasTexture.data = std::move(cpuData.atlasPixelsRGBA);
    atlasTexture.width = cpuData.atlasWidth;
    atlasTexture.height = cpuData.atlasHeight;
    atlasTexture.channels = 4;
    atlasTexture.sRGB = false;
    EmbeddedMemorySource atlasSource{};
    atlasSource.scene = nullptr;
    atlasSource.identifier = cacheKey;
    atlasSource.scenePath = "FontResource";
    atlasTexture.source = atlasSource;

    TextureHandle textureHandle = createTexture(atlasTexture);
    if(!textureHandle.isValid())
    {
      LOG_ERROR("ResourceManager: Failed to create font atlas texture '{}'", font.name);
      return {};
    }

    // Populate font data immediately
    ResourceTraits<FontAsset>::HotData hot{
      .atlasTexture = textureHandle,
      .ascent = cpuData.ascent,
      .descent = cpuData.descent,
      .lineGap = cpuData.lineGap,
      .fontSize = font.buildSettings.pixelHeight,
      .atlasWidth = cpuData.atlasWidth,
      .atlasHeight = cpuData.atlasHeight,
      .cpuData = std::move(cpuData)
    };

    ResourceTraits<FontAsset>::ColdData cold;
    cold.fontBinary = font.fontFileData;
    cold.cacheKey = cacheKey;
    cold.mergeSources = font.mergeSources;

    FontHandle handle = m_fontPool.create(hot, cold);

    {
      std::unique_lock cacheWrite(m_cacheMutex);
      m_fontCache.emplace(cacheKey, handle);
    }

    return handle;
  }

  ClipId ResourceManager::createClip(const ProcessedAnimationClip& clip)
  {
    if (!m_freeClipSlots.empty())
    {
      // Reuse a freed slot
      uint32_t index = m_freeClipSlots.back();
      m_freeClipSlots.pop_back();
      m_clips[index] = AnimationClip(clip);  // Construct AnimationClip from ProcessedAnimationClip
      return static_cast<ClipId>(index);
    }

    // Allocate new slot
    m_clips.emplace_back(clip);
    m_clipGenerations.push_back(1);
    return static_cast<ClipId>(m_clips.size() - 1);
  }

  void ResourceManager::freeClip(ClipId id)
  {
    if (id == INVALID_CLIP_ID || id >= static_cast<ClipId>(m_clips.size()))
      return;

    // Check if already freed (avoid double-free)
    for (uint32_t freeIdx : m_freeClipSlots)
    {
      if (freeIdx == id)
        return;
    }

    // Clear the clip data
    m_clips[id] = AnimationClip{};

    // Increment generation (for future validation if needed)
    if (++m_clipGenerations[id] == 0)
      m_clipGenerations[id] = 1;

    // Add to free list for reuse
    m_freeClipSlots.push_back(id);

    LOG_DEBUG("Freed animation clip at index {}", id);
  }

  const AnimationClip& ResourceManager::Clip(ClipId id) const
  {
    if(id == INVALID_CLIP_ID || id >= static_cast<ClipId>(m_clips.size()))
      return m_clips.front();
    return m_clips[id];
  }

  SkeletonId ResourceManager::registerSkeleton(const ProcessedSkeleton& skeleton)
  {
    if(skeleton.parentIndices.empty())
      return INVALID_SKELETON_ID;

    m_skeletons.emplace_back(skeleton);
    return static_cast<SkeletonId>(m_skeletons.size() - 1);
  }

  const Skeleton& ResourceManager::Skeleton(SkeletonId id) const
  {
    if(id == INVALID_SKELETON_ID || id >= static_cast<SkeletonId>(m_skeletons.size()))
      return m_skeletons.front();
    return m_skeletons[id];
  }

  MorphSetId ResourceManager::registerMorphSet(const std::vector<MeshMorphTargetInfo>& morphTargets)
  {
    if(morphTargets.empty())
      return INVALID_MORPH_SET_ID;

    m_morphSets.emplace_back(morphTargets);
    return static_cast<MorphSetId>(m_morphSets.size() - 1);
  }

  const MorphSet& ResourceManager::Morph(MorphSetId id) const
  {
    if(id == INVALID_MORPH_SET_ID || id >= static_cast<MorphSetId>(m_morphSets.size()))
      return m_morphSets.front();
    return m_morphSets[id];
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

  const ResourceTraits<MeshAsset>::ColdData* ResourceManager::getMeshMetadata(MeshHandle handle) const
  {
    return m_meshPool.getColdData(handle);
  }

  const ResourceTraits<TextureAsset>::HotData* ResourceManager::getTextureHotData(TextureHandle handle) const
  {
    return m_texturePool.getHotData(handle);
  }

  gfx::TextureView ResourceManager::resolveTextureView(TextureHandle handle) const
  {
    const auto* hot = m_texturePool.getHotData(handle);
    if (hot && hot->hasGfxTexture)
    {
      gfx::TextureHandle gfxHandle;
      gfxHandle.index = static_cast<uint16_t>(hot->gfxTextureIndex);
      gfxHandle.generation = static_cast<uint16_t>(hot->gfxTextureGeneration);
      gfx::TextureView view = m_context->renderer->getMaterialSystem().getTextureView(gfxHandle);
      if (hina_texture_view_is_valid(view))
      {
        return view;
      }
    }
    // Fallback: default white texture
    auto& matSys = m_context->renderer->getMaterialSystem();
    return matSys.getTextureView(matSys.getDefaultWhiteTexture());
  }

  TextureHandle ResourceManager::loadTextureFromFile(const std::string& path, bool sRGB)
  {
    // Check cache first
    {
      std::shared_lock lock(m_cacheMutex);
      if (auto it = m_textureCache.find(path); it != m_textureCache.end())
        return it->second;
    }

    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
      LOG_WARNING("[ResourceManager] Failed to load texture from file: {}", path);
      return TextureHandle{};
    }

    ProcessedTexture processed;
    processed.name = path;
    processed.source = FilePathSource{path};
    processed.width = static_cast<uint32_t>(width);
    processed.height = static_cast<uint32_t>(height);
    processed.channels = 4;
    processed.sRGB = sRGB;
    processed.data.assign(data, data + width * height * 4);
    processed.textureDesc.format = gfx::Format::RGBA8_UNorm;
    processed.textureDesc.dimensions = {processed.width, processed.height, 1};
    stbi_image_free(data);

    return createTexture(processed);
  }

  const ResourceTraits<FontAsset>::HotData* ResourceManager::getFont(FontHandle handle) const
  {
    return m_fontPool.getHotData(handle);
  }

  const ResourceTraits<FontAsset>::ColdData* ResourceManager::getFontMetadata(FontHandle handle) const
  {
    return m_fontPool.getColdData(handle);
  }

  const FontGlyph* ResourceManager::getFontGlyph(FontHandle handle, uint32_t codepoint) const
  {
    const auto* hot = getFont(handle);
    if(!hot)
      return nullptr;
    return hot->cpuData.findGlyph(codepoint);
  }

  FontHandle ResourceManager::getDefaultUIFont() const
  {
    return m_defaultUIFont;
  }

  void ResourceManager::setDefaultUIFont(FontHandle handle)
  {
    m_defaultUIFont = handle;
  }

  const Material* ResourceManager::getMaterialCPU(MaterialHandle handle) const
  {
    const auto* cold = m_materialPool.getColdData(handle);
    return cold ? &cold->material : nullptr;
  }

  const ResourceTraits<MaterialAsset>::HotData* ResourceManager::getMaterialHotData(MaterialHandle handle) const
  {
    return m_materialPool.getHotData(handle);
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

    // Destroy gfx mesh (hina-vk path)
    if (const auto* hot = m_meshPool.getHotData(handle)) {
      if (hot->hasGfxMesh && m_context && m_context->renderer) {
        if (auto* gfxRenderer = m_context->renderer) {
          gfx::MeshHandle gfxMesh;
          gfxMesh.index = hot->gfxMeshIndex;
          gfxMesh.generation = hot->gfxMeshGeneration;
          gfxRenderer->getMeshStorage().destroy(gfxMesh);
        }
      }
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

    m_meshPool.destroy(handle);
  }

  void ResourceManager::freeMaterial(MaterialHandle handle)
  {
    if(!m_materialPool.isValid(handle))
      return;

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

    // Destroy gfx material (hina-vk path)
    if (const auto* hot = m_materialPool.getHotData(handle)) {
      if (hot->hasGfxMaterial) {
        if (m_context && m_context->renderer) {
          if (auto* gfxRenderer = m_context->renderer) {
            gfx::MaterialHandle gfxMaterial;
            gfxMaterial.index = hot->gfxMaterialIndex;
            gfxMaterial.generation = hot->gfxMaterialGeneration;
            gfxRenderer->getMaterialSystem().destroyMaterial(gfxMaterial);
          }
        }
      }
    }

    m_materialPool.destroy(handle);
  }

  void ResourceManager::freeTexture(TextureHandle handle)
  {
    if(!m_texturePool.isValid(handle))
      return;

    std::string cacheKey;

    if(const auto* cold = m_texturePool.getColdData(handle))
    {
      cacheKey = cold->cacheKey;
    }

    // Destroy gfx texture (hina-vk path) - also cleans up UI bind groups
    if (const auto* hot = m_texturePool.getHotData(handle)) {
      if (hot->hasGfxTexture) {
        if (m_context && m_context->renderer) {
          if (auto* gfxRenderer = m_context->renderer) {
            gfx::TextureHandle gfxTexture;
            gfxTexture.index = hot->gfxTextureIndex;
            gfxTexture.generation = hot->gfxTextureGeneration;
            gfxRenderer->getMaterialSystem().destroyTexture(gfxTexture);
          }
        }
      }
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

  void ResourceManager::freeFont(FontHandle handle)
  {
    if(!m_fontPool.isValid(handle))
      return;

    TextureHandle atlasHandle;
    std::string cacheKey;

    if(const auto* hot = m_fontPool.getHotData(handle))
    {
      atlasHandle = hot->atlasTexture;
    }
    if(const auto* cold = m_fontPool.getColdData(handle))
    {
      cacheKey = cold->cacheKey;
    }

    if(atlasHandle.isValid())
    {
      freeTexture(atlasHandle);
    }

    {
      std::unique_lock lock(m_cacheMutex);
      if(!cacheKey.empty())
      {
        m_fontCache.erase(cacheKey);
      }
      else
      {
        std::erase_if(m_fontCache,
                      [handle](const auto& pair)
        {
          return pair.second == handle;
        });
      }
    }

    m_fontPool.destroy(handle);
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
      return it->second.getId();
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

    // Add dependencies for textures that haven't finished GPU upload yet
    auto addDependencyIfUnresolved = [&](const MaterialTexture& matTex)
    {
      if(!matTex.hasTexture())
        return;

      std::string textureCacheKey = TextureCacheKeyGenerator::generateKey(matTex.source);
      // Check if the texture has actually completed its GPU upload, not just whether
      // a cache entry exists (cache entries are created immediately, before async upload)
      bool textureReady = false;
      auto it = m_textureCache.find(textureCacheKey);
      if (it != m_textureCache.end()) {
        if (const auto* hot = m_texturePool.getHotData(it->second)) {
          textureReady = hot->hasGfxTexture;
        }
      }
      if (!textureReady) {
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

    // Update GfxMaterialSystem bind groups (hina-vk path)
    if (m_context && m_context->renderer) {
      if (auto* gfxRenderer = m_context->renderer) {
        auto& materialSystem = gfxRenderer->getMaterialSystem();

        // Helper to resolve gfx::TextureHandle from MaterialTexture
        auto resolveGfxTexture = [this](const MaterialTexture& matTex) -> gfx::TextureHandle {
          if (!matTex.hasTexture()) return {};
          std::string cacheKey = TextureCacheKeyGenerator::generateKey(matTex.source);
          auto it = m_textureCache.find(cacheKey);
          if (it != m_textureCache.end()) {
            if (const auto* hot = m_texturePool.getHotData(it->second)) {
              if (hot->hasGfxTexture) {
                gfx::TextureHandle gfxTex;
                gfxTex.index = hot->gfxTextureIndex;
                gfxTex.generation = hot->gfxTextureGeneration;
                return gfxTex;
              }
            }
          }
          return {};
        };

        for(MaterialHandle materialHandle : waitingMaterials)
        {
          auto* cold = m_materialPool.getColdData(materialHandle);
          auto* hot = m_materialPool.getHotData(materialHandle);
          if (!cold || !hot || !hot->hasGfxMaterial) continue;

          const Material& mat = cold->material;

          // Rebuild gfx::GfxMaterial with updated texture handles
          gfx::GfxMaterial gfxMat;
          gfxMat.baseColor = mat.baseColorFactor;
          gfxMat.roughness = mat.roughnessFactor;
          gfxMat.metallic = mat.metallicFactor;
          gfxMat.emissive = glm::length(mat.emissiveFactor) > 0.0f ? 1.0f : 0.0f;
          gfxMat.ao = mat.occlusionStrength;
          gfxMat.rim = 0.0f;

          switch (mat.alphaMode) {
            case AlphaMode::Opaque: gfxMat.alphaMode = gfx::GfxMaterial::AlphaMode::Opaque; break;
            case AlphaMode::Mask:   gfxMat.alphaMode = gfx::GfxMaterial::AlphaMode::Mask; break;
            case AlphaMode::Blend:  gfxMat.alphaMode = gfx::GfxMaterial::AlphaMode::Blend; break;
          }
          gfxMat.alphaCutoff = mat.alphaCutoff;
          gfxMat.doubleSided = (mat.flags & DOUBLE_SIDED) != 0;

          // Re-resolve all texture handles (now includes newly loaded texture)
          gfxMat.albedoTexture = resolveGfxTexture(mat.baseColorTexture);
          gfxMat.normalTexture = resolveGfxTexture(mat.normalTexture);
          gfxMat.metallicRoughnessTexture = resolveGfxTexture(mat.metallicRoughnessTexture);
          gfxMat.emissiveTexture = resolveGfxTexture(mat.emissiveTexture);
          gfxMat.occlusionTexture = resolveGfxTexture(mat.occlusionTexture);

          // Update the material (this rebuilds the bind group)
          gfx::MaterialHandle gfxHandle;
          gfxHandle.index = hot->gfxMaterialIndex;
          gfxHandle.generation = hot->gfxMaterialGeneration;
          materialSystem.updateMaterial(gfxHandle, gfxMat);

          LOG_DEBUG("Updated GfxMaterial {}:{} with resolved texture '{}'",
                    gfxHandle.index, gfxHandle.generation, textureCacheKey);
        }
      }
    }
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
    
    auto hash_combine = [&hash](size_t new_hash) {
        hash ^= new_hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    };

    auto hash_texture_slot = [&](const MaterialTexture& matTex) {
        std::string key = TextureCacheKeyGenerator::generateKey(matTex.source);
        hash_combine(std::hash<std::string>{}(key));
    };

    hash_combine(std::hash<float>{}(material.metallicFactor));
    hash_combine(std::hash<float>{}(material.roughnessFactor));

    hash_texture_slot(material.baseColorTexture);
    hash_texture_slot(material.metallicRoughnessTexture);
    hash_texture_slot(material.normalTexture);
    hash_texture_slot(material.emissiveTexture);
    hash_texture_slot(material.occlusionTexture);

    return material.name + "_" + std::to_string(hash);
  }

  std::string ResourceManager::generateTextureCacheKey(const ProcessedTexture& texture)
  {
    return TextureCacheKeyGenerator::generateKey(texture.source);
  }

  std::string ResourceManager::generateFontCacheKey(const ProcessedFont& font)
  {
    size_t hash = std::hash<std::string>{}(font.name);
    hash ^= std::hash<uint32_t>{}(static_cast<uint32_t>(font.buildSettings.pixelHeight * 100.0f) + font.buildSettings.firstCodepoint + (font.buildSettings.lastCodepoint << 16));
    hash ^= std::hash<int>{}(font.buildSettings.oversample);
    hash ^= std::hash<bool>{}(font.buildSettings.snapToPixel);
    hash ^= std::hash<uint32_t>{}(static_cast<uint32_t>(font.buildSettings.rasterizerMultiply * 1000.0f));
    auto hashCombine = [](size_t seed, size_t value)
    {
      return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    };
    if(!font.sourceFile.empty())
    {
      hash = hashCombine(hash, std::hash<std::string>{}(font.sourceFile.string()));
    }
    else
    {
      hash = hashCombine(hash, std::hash<std::string>{}(std::string(
        reinterpret_cast<const char*>(font.fontFileData.data()), font.fontFileData.size())));
    }
    for(const auto& merge : font.mergeSources)
    {
      size_t mergeHash = std::hash<std::string>{}(merge.name);
      mergeHash = hashCombine(mergeHash, std::hash<uint32_t>{}(static_cast<uint32_t>(merge.buildSettings.pixelHeight * 100.0f)));
      mergeHash = hashCombine(mergeHash, std::hash<uint32_t>{}(merge.buildSettings.firstCodepoint));
      mergeHash = hashCombine(mergeHash, std::hash<uint32_t>{}(merge.buildSettings.lastCodepoint));
      mergeHash = hashCombine(mergeHash, std::hash<int>{}(merge.buildSettings.oversample));
      mergeHash = hashCombine(mergeHash, std::hash<bool>{}(merge.buildSettings.snapToPixel));
      mergeHash = hashCombine(mergeHash, std::hash<uint32_t>{}(static_cast<uint32_t>(merge.buildSettings.rasterizerMultiply * 1000.0f)));
      mergeHash = hashCombine(mergeHash, std::hash<uint32_t>{}(static_cast<uint32_t>(merge.buildSettings.glyphMinAdvanceX * 100.0f)));
      if(!merge.sourceFile.empty())
      {
        mergeHash = hashCombine(mergeHash, std::hash<std::string>{}(merge.sourceFile.string()));
      }
      else
      {
        mergeHash = hashCombine(mergeHash, std::hash<std::string>{}(std::string(
          reinterpret_cast<const char*>(merge.fontFileData.data()), merge.fontFileData.size())));
      }
      hash = hashCombine(hash, mergeHash);
    }
    return font.name + "_" + std::to_string(hash);
  }
} // namespace AssetLoading
