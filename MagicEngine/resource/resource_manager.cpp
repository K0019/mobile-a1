#include "resource_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <numeric>
#include <span>
#include <stdexcept>

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
    : m_vertexAllocator(ResourceLimits::VERTEX_BUFFER_SIZE),
      m_indexAllocator(ResourceLimits::INDEX_BUFFER_SIZE),
      m_materialAllocator(ResourceLimits::MATERIAL_BUFFER_SIZE),
      m_meshDecompAllocator(ResourceLimits::MESH_DECOMPRESSION_BUFFER_SIZE),
      m_skinningAllocator(ResourceLimits::SKINNING_BUFFER_SIZE),
      m_morphDeltaAllocator(ResourceLimits::MORPH_DELTA_BUFFER_SIZE),
      m_morphBaseAllocator(ResourceLimits::MORPH_VERTEX_BASE_BUFFER_SIZE),
      m_morphCountAllocator(ResourceLimits::MORPH_VERTEX_COUNT_BUFFER_SIZE)
  {
    m_clips.emplace_back();
    m_skeletons.emplace_back();
    m_morphSets.emplace_back();

  }

  void ResourceManager::initialize(Context* context)
  {
    this->m_context = context;
    LOG_INFO("Resource Manager initialized - {}M vertices, {}M indices, {}K materials", ResourceLimits::MAX_VERTICES / 1'000'000, ResourceLimits::MAX_INDICES / 1'000'000, ResourceLimits::MAX_MATERIALS / 1000);

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
    m_meshPool.clear();
    m_texturePool.clear();
    m_materialPool.clear();
    m_defaultUIFont = {};
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

    // Compute decompression data and compress vertices on CPU
    MeshDecompressionData decompressionData = VertexCompression::computeDecompressionData(mesh.vertices.data(), mesh.vertices.size());

    std::vector<CompressedVertex> compressedVertices;
    compressedVertices.reserve(mesh.vertices.size());
    for(const auto& v : mesh.vertices)
    {
      compressedVertices.push_back(VertexCompression::compress(v, decompressionData));
    }

    //VertexCompression::validateCompression(mesh.vertices, compressedVertices, decompressionData);

    // Prepare animation GPU payloads
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

    // Allocate GPU memory
    OffsetAllocator::Allocation vertexAlloc;
    OffsetAllocator::Allocation indexAlloc;
    OffsetAllocator::Allocation meshDecompAlloc;
    OffsetAllocator::Allocation skinningAlloc;
    OffsetAllocator::Allocation morphDeltaAlloc;
    OffsetAllocator::Allocation morphBaseAlloc;
    OffsetAllocator::Allocation morphCountAlloc;

    auto releaseAllocations = [&]()
    {
      if(vertexAlloc.isValid())
        m_vertexAllocator.free(vertexAlloc);
      if(indexAlloc.isValid())
        m_indexAllocator.free(indexAlloc);
      if(meshDecompAlloc.isValid())
        m_meshDecompAllocator.free(meshDecompAlloc);
      if(skinningAlloc.isValid())
        m_skinningAllocator.free(skinningAlloc);
      if(morphDeltaAlloc.isValid())
        m_morphDeltaAllocator.free(morphDeltaAlloc);
      if(morphBaseAlloc.isValid())
        m_morphBaseAllocator.free(morphBaseAlloc);
      if(morphCountAlloc.isValid())
        m_morphCountAllocator.free(morphCountAlloc);
    };

    {
      std::lock_guard allocatorLock(m_meshAllocatorMutex);
      vertexAlloc = m_vertexAllocator.allocate(static_cast<uint32_t>(compressedVertices.size() * sizeof(CompressedVertex)));
      indexAlloc = m_indexAllocator.allocate(static_cast<uint32_t>(mesh.indices.size() * sizeof(uint32_t)));
      meshDecompAlloc = m_meshDecompAllocator.allocate(sizeof(MeshDecompressionData));

      if(!gpuSkinning.empty())
      {
        skinningAlloc = m_skinningAllocator.allocate(static_cast<uint32_t>(gpuSkinning.size() * sizeof(GPUSkinningData)));
      }
      if(!gpuMorphDeltas.empty())
      {
        morphDeltaAlloc = m_morphDeltaAllocator.allocate(static_cast<uint32_t>(gpuMorphDeltas.size() * sizeof(GPUMorphDelta)));
        morphBaseAlloc = m_morphBaseAllocator.allocate(static_cast<uint32_t>(morphVertexStarts.size() * sizeof(uint32_t)));
        morphCountAlloc = m_morphCountAllocator.allocate(static_cast<uint32_t>(morphVertexCounts.size() * sizeof(uint32_t)));
      }
    }

      const bool allocationsOk =
        vertexAlloc.isValid() &&
        indexAlloc.isValid() &&
        meshDecompAlloc.isValid() &&
        (gpuSkinning.empty() || skinningAlloc.isValid()) &&
        (gpuMorphDeltas.empty() || (morphDeltaAlloc.isValid() && morphBaseAlloc.isValid() && morphCountAlloc.isValid()));

      if(!allocationsOk)
      {
        releaseAllocations();
        LOG_ERROR("Out of GPU memory for mesh '{}'", mesh.name);
        return {};
      }
    

    // Create handle and populate data
    MeshHandle handle = m_meshPool.create();
      if(auto* cold = m_meshPool.getColdData(handle))
      {
        cold->meshName = mesh.name;
        cold->vertexMetadata = vertexAlloc;
        cold->indexMetadata = indexAlloc;
      cold->meshDecompressionMetadata = meshDecompAlloc;
      cold->skinningMetadata = skinningAlloc;
      cold->morphDeltaMetadata = morphDeltaAlloc;
      cold->morphVertexBaseMetadata = morphBaseAlloc;
      cold->morphVertexCountMetadata = morphCountAlloc;
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
      hot->vertexByteOffset = vertexAlloc.offset;
      hot->indexByteOffset = indexAlloc.offset;
      hot->vertexCount = static_cast<uint32_t>(mesh.vertices.size());
      hot->indexCount = static_cast<uint32_t>(mesh.indices.size());
      hot->bounds = mesh.bounds;
      hot->decompressionByteOffset = meshDecompAlloc.offset;

      auto& anim = hot->animation;
      anim.jointCount = static_cast<uint32_t>(mesh.skeleton.parentIndices.size());
      anim.skinningByteOffset = skinningAlloc.isValid() ? skinningAlloc.offset : UINT32_MAX;
      anim.morphDeltaByteOffset = morphDeltaAlloc.isValid() ? morphDeltaAlloc.offset : UINT32_MAX;
      anim.morphDeltaCount = morphDeltaAlloc.isValid() ? static_cast<uint32_t>(gpuMorphDeltas.size()) : 0;
      anim.morphVertexBaseOffset = morphBaseAlloc.isValid() ? morphBaseAlloc.offset : UINT32_MAX;
      anim.morphVertexCountOffset = morphCountAlloc.isValid() ? morphCountAlloc.offset : UINT32_MAX;
      anim.morphTargetCount = static_cast<uint32_t>(mesh.morphTargets.size());

      // Upload to GfxMeshStorage directly (hina-vk path)
      if (m_context && m_context->renderer) {
        if (auto* gfxRenderer = m_context->renderer) {
          auto& meshStorage = gfxRenderer->getMeshStorage();

          // Convert Vertex to gfx::FullVertex (they have matching layout)
          static_assert(sizeof(Vertex) == sizeof(gfx::FullVertex), "Vertex layouts must match");
          const auto* gfxVertices = reinterpret_cast<const gfx::FullVertex*>(mesh.vertices.data());

          gfx::MeshHandle gfxMesh;

          // Check if mesh has skinning data for skeletal animation
          if (!gpuSkinning.empty()) {
            // Convert to compact skinning format (8 bytes vs 32 bytes)
            auto compactSkinning = buildCompactSkinningBuffer(gpuSkinning);

            // Pack vertices into position/attribute streams
            std::vector<gfx::VertexPosition> positions;
            std::vector<gfx::VertexAttributes> attributes;
            gfx::MeshBounds bounds;

            positions.resize(mesh.vertices.size());
            attributes.resize(mesh.vertices.size());
            glm::vec3 minPos(std::numeric_limits<float>::max());
            glm::vec3 maxPos(std::numeric_limits<float>::lowest());

            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
              const auto& v = gfxVertices[i];
              positions[i] = gfx::VertexPosition(v.position);
              float bitangentSign = v.tangent.w;
              glm::vec3 tangent = glm::vec3(v.tangent);
              attributes[i].pack(v.normal, tangent, bitangentSign, v.getUV(), 0);
              minPos = glm::min(minPos, v.position);
              maxPos = glm::max(maxPos, v.position);
            }

            bounds.aabbMin = minPos;
            bounds.aabbMax = maxPos;
            bounds.center = (minPos + maxPos) * 0.5f;
            bounds.radius = glm::length(maxPos - bounds.center);

            LOG_INFO("Uploading skinned mesh '{}' to GfxMeshStorage ({} verts, {} indices, {} joints)",
                     mesh.name, mesh.vertices.size(), mesh.indices.size(), hot->animation.jointCount);

            gfxMesh = meshStorage.uploadPackedSkinned(
              positions.data(),
              attributes.data(),
              compactSkinning.data(),
              static_cast<uint32_t>(mesh.vertices.size()),
              mesh.indices.data(),
              static_cast<uint32_t>(mesh.indices.size()),
              bounds
            );
          } else {
            // Static mesh - use standard upload
            LOG_INFO("Uploading static mesh '{}' to GfxMeshStorage ({} verts, {} indices)",
                     mesh.name, mesh.vertices.size(), mesh.indices.size());

            gfxMesh = meshStorage.upload(
              gfxVertices,
              static_cast<uint32_t>(mesh.vertices.size()),
              mesh.indices.data(),
              static_cast<uint32_t>(mesh.indices.size())
            );
          }

          if (gfxMesh.isValid()) {
            hot->hasGfxMesh = true;
            hot->gfxMeshIndex = gfxMesh.index;
            hot->gfxMeshGeneration = gfxMesh.generation;
            LOG_INFO("Uploaded mesh '{}' to GfxMeshStorage: index={} gen={} skinned={}",
                      mesh.name, gfxMesh.index, gfxMesh.generation, !gpuSkinning.empty());
          } else {
            LOG_WARNING("Failed to upload mesh '{}' to GfxMeshStorage", mesh.name);
          }
        } else {
          LOG_WARNING("GfxRenderer not available - mesh '{}' not uploaded to GPU", mesh.name);
        }
      } else {
        LOG_WARNING("Renderer context not available - mesh '{}' not uploaded to GPU", mesh.name);
      }
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

    // Create handle and populate data
    TextureHandle handle = m_texturePool.create();
    if(auto* cold = m_texturePool.getColdData(handle))
    {
      cold->sourceFile = texture.name;
      cold->cacheKey = cacheKey;
      cold->textureDesc = texture.textureDesc;
      cold->isSRGB = texture.sRGB;
    }

    // Upload to GfxMaterialSystem directly (hina-vk path)
    if (m_context && m_context->renderer && !texture.data.empty()) {
      if (auto* gfxRenderer = m_context->renderer) {
        auto& materialSystem = gfxRenderer->getMaterialSystem();

        // Use actual format from texture descriptor (captured from KTX2 file)
        // Convert to sRGB format if needed (color textures like albedo/emissive need sRGB)
        gfx::Format gfxFormat = texture.textureDesc.format;
        if (texture.sRGB && !gfx::isFormatSRGB(gfxFormat)) {
          gfx::Format srgbFormat = gfx::convertFormatSRGB(gfxFormat, true);
          if (srgbFormat != gfxFormat) {
            LOG_DEBUG("Texture '{}': Converting format {} -> {} (sRGB requested)",
                texture.name, static_cast<int>(gfxFormat), static_cast<int>(srgbFormat));
            gfxFormat = srgbFormat;
          }
        }
        hina_format format = gfxFormatToHinaFormat(gfxFormat);

        LOG_INFO("Texture '{}' upload: gfxFmt={}, hinaFmt={}, size={}x{}, dataSize={}, sRGB={}",
            texture.name, static_cast<int>(gfxFormat), static_cast<int>(format),
            texture.width, texture.height, texture.data.size(), texture.sRGB);

        gfx::TextureCreateInfo createInfo;
        createInfo.data = texture.data.data();
        createInfo.width = texture.width;
        createInfo.height = texture.height;
        createInfo.format = format;
        createInfo.generateMips = false;  // TODO: Enable mip generation
        createInfo.isSRGB = texture.sRGB;
        createInfo.label = texture.name.c_str();  // Debug label for RenderDoc

        gfx::TextureHandle gfxTexture = materialSystem.createTexture(createInfo);

        if (auto* hotData = m_texturePool.getHotData(handle)) {
          if (gfxTexture.isValid()) {
            hotData->hasGfxTexture = true;
            hotData->gfxTextureIndex = gfxTexture.index;
            hotData->gfxTextureGeneration = gfxTexture.generation;

            // Register texture for UI/ImGui use - now works early since UI support
            // is initialized before ImGui, and bind groups come from material system
            uint64_t uiTexId = gfxRenderer->registerUITexture(gfxTexture);
            hotData->uiTextureId = static_cast<uint32_t>(uiTexId);

            LOG_DEBUG("Uploaded texture '{}' to GfxMaterialSystem: {}:{} ({}x{}, fmt={}, {}) uiId={}",
                      texture.name, gfxTexture.index, gfxTexture.generation,
                      texture.width, texture.height, static_cast<int>(format), texture.sRGB ? "sRGB" : "linear",
                      hotData->uiTextureId);
          } else {
            LOG_WARNING("Failed to upload texture '{}' to GfxMaterialSystem", texture.name);
          }
        }
      }
    }

    m_textureCache[cacheKey] = handle;

    // Resolve materials waiting for this texture
    resolveWaitingMaterials_nolock(cacheKey);

    LOG_INFO("Texture '{}' created successfully ({}x{})", texture.name, texture.width, texture.height);
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

    Resource::FontCPUData cpuData = ui::FontSystem::BuildFontCPUData(font.fontFileData, font.buildSettings,
                                                                     font.mergeSources, font.name);
    if(cpuData.atlasPixelsRGBA.empty() || cpuData.atlasWidth == 0 || cpuData.atlasHeight == 0)
    {
      LOG_ERROR("ResourceManager: Failed to build font atlas '{}'", font.name);
      return {};
    }

    ProcessedTexture atlasTexture;
    atlasTexture.name = font.name + "_FontAtlas";
    atlasTexture.textureDesc.type = gfx::TextureType::Tex2D;
    atlasTexture.textureDesc.format = gfx::Format::RGBA8_UNorm;
    atlasTexture.textureDesc.dimensions.width = cpuData.atlasWidth;
    atlasTexture.textureDesc.dimensions.height = cpuData.atlasHeight;
    atlasTexture.textureDesc.dimensions.depth = 1u;
    atlasTexture.textureDesc.numMipLevels = 1;
    atlasTexture.textureDesc.usage = gfx::TextureUsage::Sampled;
    atlasTexture.data = cpuData.atlasPixelsRGBA;
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
      LOG_ERROR("ResourceManager: Failed to upload font atlas '{}'", font.name);
      return {};
    }

    ResourceTraits<FontAsset>::HotData hot{
      .atlasTexture = textureHandle,
      .uiTextureId = getTextureUIId(textureHandle),
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
    m_clips.emplace_back(clip);
    return static_cast<ClipId>(m_clips.size() - 1);
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

  size_t ResourceManager::getUsedVertexBytes() const
  {
      return m_vertexAllocator.storageReport().highWatermark;
  }

  uint32_t ResourceManager::getVertexBufferVersion() const
  {
    return m_vertexBufferVersion;
  }

  void ResourceManager::bumpVertexBufferVersion()
  {
    ++m_vertexBufferVersion;
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

  uint32_t ResourceManager::getMaterialIndex(MaterialHandle handle) const
  {
    const auto* hot = m_materialPool.getHotData(handle);
    return hot ? hot->materialOffset / sizeof(MaterialData) : 0;
  }

  uint32_t ResourceManager::getTextureUIId(TextureHandle handle) const
  {
    const auto* hot = m_texturePool.getHotData(handle);
    return hot ? hot->uiTextureId : 0;
  }

  const ResourceTraits<TextureAsset>::HotData* ResourceManager::getTextureHotData(TextureHandle handle) const
  {
    return m_texturePool.getHotData(handle);
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

  uint32_t ResourceManager::getFontTextureUIId(FontHandle handle) const
  {
    const auto* hot = getFont(handle);
    return hot ? hot->uiTextureId : 0;
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

    // Get allocation data before destroying handle
    OffsetAllocator::Allocation vertexAlloc, indexAlloc, meshDecompAlloc, skinningAlloc, morphDeltaAlloc, morphBaseAlloc, morphCountAlloc;
    if(const auto* cold = m_meshPool.getColdData(handle))
    {
      vertexAlloc = cold->vertexMetadata;
      indexAlloc = cold->indexMetadata;
      meshDecompAlloc = cold->meshDecompressionMetadata;
      skinningAlloc = cold->skinningMetadata;
      morphDeltaAlloc = cold->morphDeltaMetadata;
      morphBaseAlloc = cold->morphVertexBaseMetadata;
      morphCountAlloc = cold->morphVertexCountMetadata;
    }

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
    bumpVertexBufferVersion();
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
      bumpVertexBufferVersion();
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
      return getTextureUIId(it->second);
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

    // Re-generate GPU data and queue uploads (legacy path)
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

  void ResourceManager::uploadMeshBatch([[maybe_unused]] const std::vector<PendingMeshUpload>& meshes)
  {
    // Legacy batch upload - meshes now uploaded directly via GfxMeshStorage in loadMesh()
    // This function is kept for compatibility but does nothing with hina-vk
  }

  void ResourceManager::uploadMaterialBatch([[maybe_unused]] std::vector<PendingMaterialUpload>& materials)
  {
    // TODO: Implement material upload via hina-vk when material system is complete
    // For now, materials are tracked but not GPU-uploaded
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
