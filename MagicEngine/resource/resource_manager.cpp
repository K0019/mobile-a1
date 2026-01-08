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

namespace
{
  constexpr float MORPH_DELTA_EPSILON = 1e-6f;
  constexpr float MORPH_DELTA_EPSILON_SQ = MORPH_DELTA_EPSILON * MORPH_DELTA_EPSILON;
  constexpr const char* kDefaultUIFontPath = "assets/fonts/DefaultUI.ttf";

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
    }

    // Queue upload
    {
      std::lock_guard pendingLock(m_pendingMeshesMutex);
      PendingMeshUpload pending;
      pending.compressedVertices = std::move(compressedVertices);
      pending.indices = mesh.indices;
      pending.skinningData = std::move(gpuSkinning);
      pending.morphDeltas = std::move(gpuMorphDeltas);
      pending.morphVertexStarts = std::move(morphVertexStarts);
      pending.morphVertexCounts = std::move(morphVertexCounts);
      pending.decompressionData = decompressionData;
      pending.vertexAlloc = vertexAlloc;
      pending.indexAlloc = indexAlloc;
      pending.meshDecompAlloc = meshDecompAlloc;
      pending.skinningAlloc = skinningAlloc;
      pending.morphDeltaAlloc = morphDeltaAlloc;
      pending.morphVertexStartAlloc = morphBaseAlloc;
      pending.morphVertexCountAlloc = morphCountAlloc;
      m_pendingMeshes.push_back(std::move(pending));
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
    atlasTexture.textureDesc.type = vk::TextureType::Tex2D;
    atlasTexture.textureDesc.format = vk::Format::RGBA_UN8;
    atlasTexture.textureDesc.dimensions.width = cpuData.atlasWidth;
    atlasTexture.textureDesc.dimensions.height = cpuData.atlasHeight;
    atlasTexture.textureDesc.dimensions.depth = 1u;
    atlasTexture.textureDesc.numMipLevels = 1;
    atlasTexture.textureDesc.usage = vk::TextureUsageBits_Sampled;
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
      .bindlessIndex = getTextureBindlessIndex(textureHandle),
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

  uint32_t ResourceManager::getTextureBindlessIndex(TextureHandle handle) const
  {
    const auto* hot = m_texturePool.getHotData(handle);
    return hot ? hot->bindlessIndex : 0;
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

  uint32_t ResourceManager::getFontTextureBindlessIndex(FontHandle handle) const
  {
    const auto* hot = getFont(handle);
    return hot ? hot->bindlessIndex : 0;
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

    // Upload compressed vertices
    uploadContiguousBatches(meshes, gpuBuffers.GetVertexBuffer(), [](const PendingMeshUpload& mesh) { return mesh.vertexAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.compressedVertices); });

    // Upload indices
    uploadContiguousBatches(meshes, gpuBuffers.GetIndexBuffer(), [](const PendingMeshUpload& mesh) { return mesh.indexAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.indices); });

    // Upload mesh decompression data
    uploadContiguousBatches(meshes, gpuBuffers.GetMeshDecompressionBuffer(), [](const PendingMeshUpload& mesh) { return mesh.meshDecompAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(&mesh.decompressionData, 1); });

    // Upload skinning data
    uploadContiguousBatches(meshes, gpuBuffers.GetSkinningBuffer(), [](const PendingMeshUpload& mesh) { return mesh.skinningAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.skinningData); });

    // Upload morph deltas
    uploadContiguousBatches(meshes, gpuBuffers.GetMorphDeltaBuffer(), [](const PendingMeshUpload& mesh) { return mesh.morphDeltaAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.morphDeltas); });

    // Upload morph vertex mapping (start offsets)
    uploadContiguousBatches(meshes, gpuBuffers.GetMorphVertexBaseBuffer(), [](const PendingMeshUpload& mesh) { return mesh.morphVertexStartAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.morphVertexStarts); });

    // Upload morph vertex counts
    uploadContiguousBatches(meshes, gpuBuffers.GetMorphVertexCountBuffer(), [](const PendingMeshUpload& mesh) { return mesh.morphVertexCountAlloc; }, [](const PendingMeshUpload& mesh) { return std::span(mesh.morphVertexCounts); });
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

    using ItemType = typename Container::value_type;
    std::vector<const ItemType*> validItems;
    validItems.reserve(items.size());
    for(const auto& item : items)
    {
      const auto alloc = allocSelector(item);
      const auto data = dataSelector(item);
      if(!alloc.isValid() || data.empty())
        continue;
      validItems.push_back(&item);
    }

    if(validItems.empty())
      return;

    std::sort(validItems.begin(), validItems.end(),
              [&](const ItemType* a, const ItemType* b)
    {
      return allocSelector(*a).offset < allocSelector(*b).offset;
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

    for(const ItemType* item : validItems)
    {
      const auto alloc = allocSelector(*item);
      const auto data = dataSelector(*item);
      const size_t dataSize = data.size_bytes();

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
