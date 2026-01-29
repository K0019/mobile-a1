#include "render_graph.h"
#include "gfx_renderer.h"  // For swapchain access
#include "hina_context.h"  // For HinaContext
#include "VFS/VFS.h"       // For shader file loading
#include <map>
#include <queue>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <iostream>

// Push constants for resolve output (HDR->LDR tone mapping)
struct ResolveOutputPushConstants
{
  int32_t toneMappingMode;    // 0=None, 1=Reinhard, 2=Uchimura, 3=ACES, 4=KhronosPBR, 5=Uncharted2, 6=Unreal, 7=Passthrough
  float exposure;
  float maxWhite;             // Reinhard parameter
  float P;                    // Uchimura parameters
  float a;
  float m;
  float l;
  float c;
  float b;
};
static_assert(sizeof(ResolveOutputPushConstants) == 36, "ResolveOutputPushConstants must be 36 bytes");

void RenderGraph::EnsureBufferCapacity(internal::FrameBufferManager::Entry& resource, uint64_t requiredSize)
{
  if (resource.currentSize >= requiredSize && gfx::isValid(resource.buffers[0].get()))
  {
    return;
  }
  if (requiredSize == 0)
  {
    return;
  }
  resource.desc.size = requiredSize;
  resource.currentSize = requiredSize;
  // Recreate all frame buffers with new size
  for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
  {
    resource.buffers[frame].reset(gfx::createBuffer(resource.desc));
  }
}

void RenderGraph::ResizeTransientBuffer(LogicalResourceName name, uint64_t newSize)
{
  internal::NameID id = FindNameID(name);
  if (id == internal::INVALID_NAME_ID) return;
  if (const auto* info = GetResolvedResource(id))
  {
    if (info->isExternal || info->definition.persistent) return;
  }
  if (auto* entry = m_frameBuffers.GetEntry(id))
  {
    EnsureBufferCapacity(*entry, newSize);
    if (auto* info = GetResolvedResource(id))
    {
      info->concreteHandle = ResourceHandle(entry->buffers[m_currentFrameIndex]);
    }
  }
}

bool RenderGraph::AreGraphicsDeclarationsEqual(const GraphicsPassDeclaration& decl1,
                                               const GraphicsPassDeclaration& decl2)
{
  // Fast comparison with memcmp since we now use NameIDs
  return std::memcmp(&decl1, &decl2, sizeof(GraphicsPassDeclaration)) == 0;
}

void RenderGraph::FillRenderPassFromDeclaration(const GraphicsPassDeclaration& graphDecl, gfx::RenderPass& rp)
{
  rp = {};
  // Copy multiview settings
  rp.layerCount = graphDecl.layerCount;
  rp.viewMask = graphDecl.viewMask;
  // Unroll loop for better performance
  for (unsigned i = 0; i < gfx::MAX_COLOR_ATTACHMENTS; ++i)
  {
    const auto& graphAtt = graphDecl.colorAttachments[i];
    if (graphAtt.IsActive())
    {
      rp.color[i].loadOp = graphAtt.loadOp;
      rp.color[i].storeOp = graphAtt.storeOp;
      rp.color[i].layer = graphAtt.layer;
      rp.color[i].level = graphAtt.level;
      std::memcpy(&rp.color[i].clearColor, &graphAtt.clearColor, sizeof(rp.color[i].clearColor));
    }
    else
    {
      rp.color[i].loadOp = gfx::LoadOp::DontCare;
    }
  }
  if (graphDecl.depthAttachment.IsActive())
  {
    rp.depth.loadOp = graphDecl.depthAttachment.loadOp;
    rp.depth.storeOp = graphDecl.depthAttachment.storeOp;
    rp.depth.layer = graphDecl.depthAttachment.layer;
    rp.depth.level = graphDecl.depthAttachment.level;
    rp.depth.clearDepth = graphDecl.depthAttachment.clearDepth;
    rp.depth.clearStencil = graphDecl.depthAttachment.clearStencil;
  }
  else
  {
    rp.depth.loadOp = gfx::LoadOp::DontCare;
  }
  if (graphDecl.stencilAttachment.IsActive())
  {
    if (!graphDecl.depthAttachment.IsActive() || graphDecl.depthAttachment.textureNameID != graphDecl.stencilAttachment.
                                                                                                      textureNameID)
    {
      rp.stencil.loadOp = graphDecl.stencilAttachment.loadOp;
      rp.stencil.storeOp = graphDecl.stencilAttachment.storeOp;
      rp.stencil.layer = graphDecl.stencilAttachment.layer;
      rp.stencil.level = graphDecl.stencilAttachment.level;
      rp.stencil.clearStencil = graphDecl.stencilAttachment.clearStencil;
    }
    else
    {
      rp.depth.clearStencil = graphDecl.stencilAttachment.clearStencil;
    }
  }
  else
  {
    rp.stencil.loadOp = gfx::LoadOp::DontCare;
  }
}

void RenderGraph::FillFramebufferFromDeclaration(const GraphicsPassDeclaration& graphDecl, gfx::Framebuffer& fb)
{
  fb = {};
  fb.debugName = graphDecl.framebufferDebugName;
  auto getTextureHandle = [this](internal::NameID nameID) -> gfx::Texture
  {
    if (nameID == internal::INVALID_NAME_ID)
    {
      LOG_ERROR("GetTexture: FAILED. No valid handle found");
      return gfx::Texture{0};
    }
    if (const auto* info = GetResolvedResource(nameID))
    {
      if (info->concreteHandle.type == ResourceType::Texture)
      {
        return info->concreteHandle.texture;
      }
    }
    LOG_ERROR("GetTexture: FAILED. No valid handle found");
    return gfx::Texture{0};
  };
  // Fill color attachments
  for (unsigned i = 0; i < gfx::MAX_COLOR_ATTACHMENTS; ++i)
  {
    const auto& graphAtt = graphDecl.colorAttachments[i];
    if (graphAtt.IsActive())
    {
      fb.color[i].texture = getTextureHandle(graphAtt.textureNameID);
      if (graphAtt.resolveTextureNameID != internal::INVALID_NAME_ID)
      {
        fb.color[i].resolveTexture = getTextureHandle(graphAtt.resolveTextureNameID);
      }
    }
  }
  // Fill depth/stencil attachments
  if (graphDecl.depthAttachment.IsActive())
  {
    fb.depthStencil.texture = getTextureHandle(graphDecl.depthAttachment.textureNameID);
    if (graphDecl.depthAttachment.resolveTextureNameID != internal::INVALID_NAME_ID)
    {
      fb.depthStencil.resolveTexture = getTextureHandle(graphDecl.depthAttachment.resolveTextureNameID);
    }
  }
  if (graphDecl.stencilAttachment.IsActive())
  {
    if (!graphDecl.depthAttachment.IsActive() || graphDecl.depthAttachment.textureNameID != graphDecl.stencilAttachment.
                                                                                                      textureNameID)
    {
      fb.depthStencil.texture = getTextureHandle(graphDecl.stencilAttachment.textureNameID);
      if (graphDecl.stencilAttachment.resolveTextureNameID != internal::INVALID_NAME_ID)
      {
        fb.depthStencil.resolveTexture = getTextureHandle(graphDecl.stencilAttachment.resolveTextureNameID);
      }
    }
  }
}

void RenderGraph::AggregateDependenciesForPass(size_t passIndex, std::vector<gfx::Texture>& textures,
                                               std::vector<gfx::Buffer>& buffers) const
{
  const auto& execData = m_passExecData[passIndex];
  const auto& metadata = m_passMetadata[passIndex];
  // Helper to check if resource is an output attachment
  auto isOutputAttachment = [&](internal::NameID resourceId) -> bool
  {
    if (execData.passType != 0) return false; // Not graphics
    const auto& gd = metadata.graphicsDeclaration;
    // Check all attachments
    for (const auto& att : gd.colorAttachments)
    {
      if (!att.IsActive()) continue;
      if (att.textureNameID == resourceId || att.resolveTextureNameID == resourceId)
      {
        return true;
      }
    }
    if (gd.depthAttachment.IsActive())
    {
      if (gd.depthAttachment.textureNameID == resourceId || gd.depthAttachment.resolveTextureNameID == resourceId)
      {
        return true;
      }
    }
    if (gd.stencilAttachment.IsActive())
    {
      if (gd.stencilAttachment.textureNameID == resourceId || gd.stencilAttachment.resolveTextureNameID == resourceId)
      {
        return true;
      }
    }
    return false;
  };
  // Process resource accesses
  for (size_t i = 0; i < execData.handleCount; ++i)
  {
    const auto& usage = metadata.declaredResourceAccesses[i];
    if (usage.id == internal::INVALID_NAME_ID || usage.id == m_swapchainID) continue;
    // Only include resources that are read by shaders
    if (usage.accessType == AccessType::Read || usage.accessType == AccessType::ReadWrite)
    {
      if (isOutputAttachment(usage.id)) continue;
      const ResourceHandle& handle = m_handleStorage[execData.handleOffset + i];
      if (handle.type == ResourceType::Texture && gfx::isValid(handle.texture))
      {
        textures.push_back(handle.texture);
      }
      else if (handle.type == ResourceType::Buffer && gfx::isValid(handle.buffer))
      {
        buffers.push_back(handle.buffer);
      }
    }
  }
}

// ResourceProperties implementation
ResourceProperties ResourceProperties::FromDesc(const gfx::TextureDesc& textureDesc, bool isPersistent)
{
  return {.desc = textureDesc, .type = ResourceType::Texture, .persistent = isPersistent};
}

ResourceProperties ResourceProperties::FromDesc(const gfx::BufferDesc& bufferDesc, bool isPersistent)
{
  return {.desc = bufferDesc, .type = ResourceType::Buffer, .persistent = isPersistent};
}

// GraphAttachmentDescription implementation
bool GraphAttachmentDescription::operator==(const GraphAttachmentDescription& other) const
{
  // Now using NameIDs, comparison is much faster
  return textureNameID == other.textureNameID && resolveTextureNameID == other.resolveTextureNameID && loadOp == other.
    loadOp && storeOp == other.storeOp && layer == other.layer && level == other.level &&
    std::memcmp(&clearColor, &other.clearColor, sizeof(clearColor)) == 0 && clearDepth == other.clearDepth &&
    clearStencil == other.clearStencil;
}

// Internal namespace implementations
namespace internal
{
  void FastResourceCache::Insert(uint32_t hash, uint8_t index)
  {
    if (count < CACHE_SIZE)
    {
      entries[count++] = {hash, index};
    }
    else
    {
      // Simple replacement strategy - replace oldest
      entries[0] = {hash, index};
    }
  }

  uint8_t FastResourceCache::Find(uint32_t hash) const
  {
    // Unrolled loop for small cache
    for (uint8_t i = 0; i < count; ++i)
    {
      if (entries[i].nameHash == hash) return entries[i].index;
    }
    return 0xFF;
  }

  void FastResourceCache::Clear()
  {
    count = 0;
  }

  uint16_t FrameBufferManager::AllocateEntry(NameID id)
  {
    if (m_nextIndex >= MAX_RESOURCES)
    {
      return MAX_RESOURCES; // Return invalid index
    }
    uint16_t idx = m_nextIndex++;
    if (id < MAX_NAME_IDS)
    {
      m_nameToIndex[id] = idx;
    }
    return idx;
  }

  FrameBufferManager::Entry* FrameBufferManager::GetEntry(NameID id)
  {
    if (id < MAX_NAME_IDS)
    {
      uint16_t idx = m_nameToIndex[id];
      if (idx < MAX_RESOURCES && m_resources[idx].active)
      {
        return &m_resources[idx];
      }
    }
    LOG_ERROR("FBM::GetBuffer: -> FAILED");
    return nullptr;
  }

  gfx::Buffer FrameBufferManager::GetBuffer(NameID id, uint32_t frameIdx) const
  {
    if (id < MAX_NAME_IDS)
    {
      uint16_t idx = m_nameToIndex[id];
      if (idx < MAX_RESOURCES && m_resources[idx].active)
      {
        return m_resources[idx].buffers[frameIdx].get();
      }
    }
    LOG_ERROR("FBM::GetBuffer: -> FAILED. Resource ID {} is out of bounds for lookup.", id);
    return gfx::Buffer{0};
  }

  ExecutionContext::ExecutionContext(gfx::Cmd* cmd, const FrameData& frameData, RenderGraph* graph,
                                     size_t passIndex) : m_cmd(cmd), m_cmdWrapper(cmd), m_frameData(frameData),
                                                         m_pGraph(graph), m_passIndex(passIndex)
  {
  }

  uint32_t ExecutionContext::HashString(const char* str)
  {
    uint32_t hash = 2166136261u;
    while (*str)
    {
      hash ^= static_cast<uint32_t>(*str++);
      hash *= 16777619u;
    }
    return hash;
  }

  gfx::Texture ExecutionContext::GetTexture(LogicalResourceName name) const
  {
    ResourceHandle handle = GetResourceCached(name);
    if (handle.isValid() && handle.type == ResourceType::Texture)
    {
      return handle.texture;
    }
    LOG_ERROR("GetTexture: FAILED. No valid texture handle found for '{}'.", name);
    return gfx::Texture{0};
  }

  gfx::Buffer ExecutionContext::GetBuffer(LogicalResourceName name) const
  {
    NameID id = m_pGraph->FindNameID(name);
    if (id == INVALID_NAME_ID)
    {
      LOG_ERROR("GetBuffer: FAILED. Name '{}' not found in registry.", name);
      return gfx::Buffer{0};
    }
    const auto* info = m_pGraph->GetResolvedResource(id);
    if (!info || info->definition.type != ResourceType::Buffer)
    {
      LOG_ERROR("GetBuffer: FAILED. No valid buffer resource info found for ID {}.", id);
      return gfx::Buffer{0};
    }
    if (!info->isExternal && !info->definition.persistent)
    {
      uint32_t frameIdx = m_pGraph->GetCurrentFrameIndex();
      return m_pGraph->m_frameBuffers.GetBuffer(id, frameIdx);
    }
    else
    {
      ResourceHandle resHandle = GetResourceCached(name);
      if (resHandle.isValid() && resHandle.type == ResourceType::Buffer)
      {
        return resHandle.buffer;
      }
    }
    LOG_ERROR("GetBuffer: FAILED. Lookup for '{}' (ID: {}) fell through without returning a valid handle.", name, id);
    return gfx::Buffer{0};
  }

  void ExecutionContext::ResizeBuffer(LogicalResourceName name, uint64_t newSize) const
  {
    NameID id = m_pGraph->FindNameID(name);
    if (id == INVALID_NAME_ID) return;
    if (auto* entry = m_pGraph->m_frameBuffers.GetEntry(id))
    {
      m_pGraph->EnsureBufferCapacity(*entry, newSize);
    }
  }

  uint64_t ExecutionContext::GetBufferSize(LogicalResourceName name) const
  {
    NameID id = m_pGraph->FindNameID(name);
    if (id == INVALID_NAME_ID) return 0;
    const auto* info = m_pGraph->GetResolvedResource(id);
    if (!info) return 0;
    if (info->definition.type == ResourceType::Buffer && (info->isExternal || info->definition.persistent))
    {
      const auto desc = info->definition.desc;
      return std::get<gfx::BufferDesc>(desc).size;
    }
    if (auto* entry = m_pGraph->m_frameBuffers.GetEntry(id))
    {
      return entry->currentSize;
    }
    return 0;
  }

  gfx::Texture ExecutionContext::GetTextureByIndex(size_t index) const
  {
    const auto& execData = m_pGraph->m_passExecData[m_passIndex];
    if (index >= execData.handleCount)
    {
      LOG_ERROR("GetTexture: FAILED. No valid handle found at index {}", index);
      return gfx::Texture{0};
    }
    const ResourceHandle& handle = m_pGraph->m_handleStorage[execData.handleOffset + index];
    return (handle.type == ResourceType::Texture) ? handle.texture : gfx::Texture{0};
  }

  gfx::Buffer ExecutionContext::GetBufferByIndex(size_t index) const
  {
    const auto& execData = m_pGraph->m_passExecData[m_passIndex];
    if (index >= execData.handleCount)
    {
      LOG_ERROR("GetTexture: FAILED. No valid handle found at index {}", index);
      return gfx::Buffer{0};
    }
    const ResourceHandle& handle = m_pGraph->m_handleStorage[execData.handleOffset + index];
    return (handle.type == ResourceType::Buffer) ? handle.buffer : gfx::Buffer{0};
  }

  GfxRenderer* ExecutionContext::GetGfxRenderer() const
  {
    return m_pGraph->m_gfxRenderer;
  }

  HinaContext* ExecutionContext::GetHinaContext() const
  {
    GfxRenderer* renderer = GetGfxRenderer();
    return renderer ? renderer->getHinaContext() : nullptr;
  }

  UniformAllocation ExecutionContext::AllocateUniform(uint64_t size)
  {
    return m_pGraph->m_uniformRing.allocate(size);
  }

  UniformAllocation ExecutionContext::WriteUniform(const void* data, uint64_t size)
  {
    return m_pGraph->m_uniformRing.write(data, size);
  }

  // ============================================================================
  // UniformRing Implementation
  // ============================================================================

  bool UniformRing::init(uint64_t sizePerFrame)
  {
    if (m_initialized) {
      return true;
    }

    // Get alignment from device caps
    const hina_device_caps* caps = hina_get_device_caps();
    m_alignment = caps ? caps->min_uniform_buffer_alignment : 256;
    if (m_alignment == 0) {
      m_alignment = 256; // Safe fallback
    }

    m_sizePerFrame = sizePerFrame;

    // Create triple-buffered uniform buffers
    hina_buffer_desc desc = {};
    desc.size = sizePerFrame;
    desc.memory = HINA_BUFFER_CPU;
    desc.usage = HINA_BUFFER_UNIFORM;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      gfx::Buffer buf = hina_make_buffer(&desc);
      if (!hina_buffer_is_valid(buf)) {
        shutdown();
        return false;
      }
      m_buffers[i].reset(buf);
      m_mapped[i] = hina_mapped_buffer_ptr(buf);
      if (!m_mapped[i]) {
        shutdown();
        return false;
      }
    }

    m_initialized = true;
    m_currentFrame = 0;
    m_currentOffset = 0;
    return true;
  }

  void UniformRing::shutdown()
  {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // HOST_VISIBLE buffers are persistently mapped in hina-vk, don't unmap them
      m_mapped[i] = nullptr;
      m_buffers[i].reset();
    }
    m_initialized = false;
  }

  void UniformRing::beginFrame(uint32_t frameIndex)
  {
    m_currentFrame = frameIndex % MAX_FRAMES_IN_FLIGHT;
    m_currentOffset = 0;
  }

  UniformAllocation UniformRing::allocate(uint64_t size)
  {
    UniformAllocation result = {};

    if (!m_initialized || size == 0) {
      return result;
    }

    // Align the current offset
    uint64_t alignedOffset = (m_currentOffset + m_alignment - 1) & ~(uint64_t)(m_alignment - 1);

    // Check if we have space
    if (alignedOffset + size > m_sizePerFrame) {
      // Out of space - this is a serious error in production
      return result;
    }

    result.buffer = m_buffers[m_currentFrame].get();
    result.offset = alignedOffset;
    result.mapped = static_cast<uint8_t*>(m_mapped[m_currentFrame]) + alignedOffset;

    m_currentOffset = alignedOffset + size;
    return result;
  }

  UniformAllocation UniformRing::write(const void* data, uint64_t size)
  {
    UniformAllocation alloc = allocate(size);
    if (alloc.isValid() && data) {
      std::memcpy(alloc.mapped, data, size);
    }
    return alloc;
  }

  ResourceHandle ExecutionContext::GetResourceCached(LogicalResourceName name) const
  {
    NameID id = m_pGraph->FindNameID(name);
    if (id == INVALID_NAME_ID) return ResourceHandle{};

    // External resources (swapchain, SCENE_COLOR, SCENE_DEPTH, VIEW_OUTPUT) are updated per-frame
    // via ImportExternalTexture/UpdateViewOutputResource. Always use the current resolved handle
    // instead of stale cached handles from compilation.
    const auto* info = m_pGraph->GetResolvedResource(id);
    if (info && info->isExternal)
    {
      return m_pGraph->GetResolvedHandle(id);
    }

    // For transient resources, use the cached handle storage (populated during compilation)
    uint32_t hash = HashString(name);
    uint8_t cachedIdx = m_cache.Find(hash);
    if (cachedIdx != 0xFF)
    {
      const auto& execData = m_pGraph->m_passExecData[m_passIndex];
      return m_pGraph->m_handleStorage[execData.handleOffset + cachedIdx];
    }
    const auto& execData = m_pGraph->m_passExecData[m_passIndex];
    const auto& metadata = m_pGraph->m_passMetadata[m_passIndex];
    for (uint8_t i = 0; i < execData.handleCount; ++i)
    {
      if (metadata.declaredResourceAccesses[i].id == id)
      {
        m_cache.Insert(hash, i);
        return m_pGraph->m_handleStorage[execData.handleOffset + i];
      }
    }
    return ResourceHandle{};
  }

  // RenderPassBuilder implementations
  RenderPassBuilder::PassBuilder::PassBuilder(RenderPassBuilder* parent) : m_parent(parent)
  {
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::DeclareTransientResource(
    LogicalResourceName name, const gfx::TextureDesc& desc)
  {
    m_parent->m_pRenderGraph->RegisterTransientResource(name, desc);
    return *this;
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::DeclareTransientResource(
    LogicalResourceName name, const gfx::BufferDesc& desc)
  {
    m_parent->m_pRenderGraph->RegisterTransientResource(name, desc);
    return *this;
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::UseResource(
    LogicalResourceName name, AccessType accessType)
  {
    NameID id = m_parent->m_pRenderGraph->InternName(name);
    m_resourceDeclarations.push_back({id, accessType});
    return *this;
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::SetPriority(PassPriority priority)
  {
    m_priority = priority;
    return *this;
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::ExecuteAfter(const char* passName)
  {
    NameID targetId = m_parent->m_pRenderGraph->FindNameID(passName);
    if (targetId != INVALID_NAME_ID)
    {
      m_executeAfterTargets.push_back(targetId);
    }
    return *this;
  }

  RenderPassBuilder& RenderPassBuilder::PassBuilder::AddGraphicsPass(const char* passName,
                                                                     const PassDeclarationInfo& declaration,
                                                                     ExecuteLambda&& executeLambda)
  {
    m_parent->SubmitGraphicsPass(*this, passName, declaration, std::move(executeLambda));
    return *m_parent;
  }

  RenderPassBuilder& RenderPassBuilder::PassBuilder::AddGenericPass(const char* passName, ExecuteLambda&& executeLambda)
  {
    m_parent->SubmitGenericPass(*this, passName, std::move(executeLambda));
    return *m_parent;
  }

  RenderPassBuilder::PassBuilder RenderPassBuilder::CreatePass()
  {
    return PassBuilder(this);
  }

  RenderPassBuilder::RenderPassBuilder(RenderGraph* graph, IRenderFeature* feature) : m_pRenderGraph(graph),
                                                                                      m_pCurrentFeature(feature)
  {
  }

  void RenderPassBuilder::SubmitGraphicsPass(PassBuilder& builder, const char* passName,
                                             const PassDeclarationInfo& declaration, ExecuteLambda&& executeLambda)
  {
    NameID passID = m_pRenderGraph->InternName(passName);
    GraphicsPassDeclaration declWithIDs{};
    auto convertAttachment = [&](const AttachmentDesc& publicDesc, GraphAttachmentDescription& internalDesc)
    {
      if (publicDesc.textureName)
      {
        internalDesc.textureNameID = m_pRenderGraph->InternName(publicDesc.textureName);
      }
      if (publicDesc.resolveTextureName)
      {
        internalDesc.resolveTextureNameID = m_pRenderGraph->InternName(publicDesc.resolveTextureName);
      }
      internalDesc.loadOp = publicDesc.loadOp;
      internalDesc.storeOp = publicDesc.storeOp;
      internalDesc.layer = publicDesc.layer;
      internalDesc.level = publicDesc.level;
      memcpy(&internalDesc.clearColor, &publicDesc.clearColor, sizeof(internalDesc.clearColor));
      internalDesc.clearDepth = publicDesc.clearDepth;
      internalDesc.clearStencil = publicDesc.clearStencil;
    };
    // Convert color attachments
    for (size_t i = 0; i < gfx::MAX_COLOR_ATTACHMENTS; ++i)
    {
      convertAttachment(declaration.colorAttachments[i], declWithIDs.colorAttachments[i]);
    }
    convertAttachment(declaration.depthAttachment, declWithIDs.depthAttachment);
    convertAttachment(declaration.stencilAttachment, declWithIDs.stencilAttachment);
    // Copy multiview settings
    declWithIDs.layerCount = declaration.layerCount;
    declWithIDs.viewMask = declaration.viewMask;
    if (declaration.framebufferDebugName)
    {
      declWithIDs.framebufferDebugName = declaration.framebufferDebugName;
    }
    // Get the feature mask for this feature (0 if not registered)
    FeatureMask featureMask = m_pCurrentFeature ? m_pRenderGraph->GetFeatureMask(m_pCurrentFeature) : 0;

    PassExecutionData execData{
      .passID = passID, .passType = 0,
      // Graphics
      .handleCount = static_cast<uint8_t>(builder.m_resourceDeclarations.size()), .handleOffset = 0,
      .executeLambda = nullptr
    };
    PassMetadata metadata{
      .debugName = passName, .declaredResourceAccesses = std::move(builder.m_resourceDeclarations),
      .priority = builder.m_priority, .executeAfterTargets = std::move(builder.m_executeAfterTargets),
      .graphicsDeclaration = declWithIDs, .requiredFeatureMask = featureMask, .featureOwner = m_pCurrentFeature
    };
    m_pRenderGraph->RegisterPass(std::move(execData), std::move(metadata), std::move(executeLambda));
  }

  void RenderPassBuilder::SubmitGenericPass(PassBuilder& builder, const char* passName, ExecuteLambda&& executeLambda)
  {
    NameID passID = m_pRenderGraph->InternName(passName);

    // Get the feature mask for this feature (0 if not registered)
    FeatureMask featureMask = m_pCurrentFeature ? m_pRenderGraph->GetFeatureMask(m_pCurrentFeature) : 0;

    PassExecutionData execData{
      .passID = passID, .passType = 1,
      // Generic
      .handleCount = static_cast<uint8_t>(builder.m_resourceDeclarations.size()), .handleOffset = 0,
      .executeLambda = nullptr
    };
    PassMetadata metadata{
      .debugName = passName, .declaredResourceAccesses = std::move(builder.m_resourceDeclarations),
      .priority = builder.m_priority, .executeAfterTargets = std::move(builder.m_executeAfterTargets),
      .requiredFeatureMask = featureMask, .featureOwner = m_pCurrentFeature
    };
    m_pRenderGraph->RegisterPass(std::move(execData), std::move(metadata), std::move(executeLambda));
  }
} // namespace internal
RenderGraph::RenderGraph() : linearColorSystem()
{
  // Pre-reserve capacity
  m_passExecData.reserve(32);
  m_passMetadata.reserve(32);
  m_executionLambdas.reserve(32);
  m_handleStorage.reserve(256);
  m_storedTextures.reserve(16);
  m_resolvedResourceInfo.reserve(64);
  m_idToStringMap.reserve(64);
  m_nameRegistry.reserve(64);
  m_swapchainID = InternName(RenderResources::SWAPCHAIN_IMAGE);
  m_viewOutputID = InternName(RenderResources::VIEW_OUTPUT);
}

RenderGraph::~RenderGraph()
{
  m_uniformRing.shutdown();
}

void RenderGraph::SetGfxRenderer(GfxRenderer* renderer)
{
  m_gfxRenderer = renderer;
  // Initialize LinearColorSystem with the context
  if (renderer) {
    HinaContext* hinaCtx = renderer->getHinaContext();
    if (hinaCtx) {
      linearColorSystem.Initialize(hinaCtx);
    }
    // Initialize uniform ring buffer (requires hina to be initialized)
    if (!m_uniformRing.init()) {
      // Log error but continue - features won't be able to use uniform allocation
    }
  }
}

void RenderGraph::BuildAdjacencyListAndSort()
{
  size_t numPasses = m_passExecData.size();
  if (numPasses == 0)
  {
    m_compiledPassOrder.clear();
    return;
  }
  // Build pass ID to index map
  std::unordered_map<internal::NameID, size_t> passIdToIndex;
  passIdToIndex.reserve(numPasses);
  for (size_t i = 0; i < numPasses; ++i)
  {
    passIdToIndex[m_passExecData[i].passID] = i;
  }
  // Build adjacency list
  std::vector<std::vector<size_t>> adj(numPasses);
  std::vector<int> inDegree(numPasses, 0);
  // Sort passes by priority first
  std::vector<size_t> sortedIndices(numPasses);
  std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
  std::sort(sortedIndices.begin(), sortedIndices.end(), [this](size_t a, size_t b)
  {
    return static_cast<int>(m_passMetadata[a].priority) < static_cast<int>(m_passMetadata[b].priority);
  });
  // Track resource dependencies
  std::unordered_map<internal::NameID, size_t> lastWriter;
  std::unordered_map<internal::NameID, std::vector<size_t>> readersSinceLastWrite;
  for (size_t idx : sortedIndices)
  {
    const auto& metadata = m_passMetadata[idx];
    // Handle explicit ExecuteAfter dependencies
    for (internal::NameID targetId : metadata.executeAfterTargets)
    {
      if (auto it = passIdToIndex.find(targetId); it != passIdToIndex.end())
      {
        adj[it->second].push_back(idx);
        inDegree[idx]++;
      }
    }
    // Handle resource dependencies
    for (const auto& usage : metadata.declaredResourceAccesses)
    {
      internal::NameID resId = usage.id;
      // Read dependencies
      if (usage.accessType == AccessType::Read || usage.accessType == AccessType::ReadWrite)
      {
        if (auto writerIt = lastWriter.find(resId); writerIt != lastWriter.end())
        {
          adj[writerIt->second].push_back(idx);
          inDegree[idx]++;
        }
        if (usage.accessType == AccessType::Read)
        {
          readersSinceLastWrite[resId].push_back(idx);
        }
      }
      // Write dependencies
      if (usage.accessType == AccessType::Write || usage.accessType == AccessType::ReadWrite)
      {
        // All readers must complete before this write
        if (auto readersIt = readersSinceLastWrite.find(resId); readersIt != readersSinceLastWrite.end())
        {
          for (size_t readerIdx : readersIt->second)
          {
            adj[readerIdx].push_back(idx);
            inDegree[idx]++;
          }
          readersIt->second.clear();
        }
        // FIX: Add Write-After-Write (WAW) dependency
        if (auto writerIt = lastWriter.find(resId); writerIt != lastWriter.end())
        {
          adj[writerIt->second].push_back(idx);
          inDegree[idx]++;
        }
        lastWriter[resId] = idx;
      }
    }
  }
  // Topological sort using Kahn's algorithm
  std::queue<size_t> q;
  for (size_t i = 0; i < numPasses; ++i)
  {
    if (inDegree[i] == 0) q.push(i);
  }
  m_compiledPassOrder.clear();
  m_compiledPassOrder.reserve(numPasses);
  while (!q.empty())
  {
    size_t u = q.front();
    q.pop();
    m_compiledPassOrder.push_back(u);
    for (size_t v : adj[u])
    {
      if (--inDegree[v] == 0) q.push(v);
    }
  }
  // Check for cycles
  if (m_compiledPassOrder.size() != numPasses)
  {
    LOG_ERROR("Cycle detected in render graph! Only {} of {} passes compiled.", m_compiledPassOrder.size(), numPasses);
    m_compiledPassOrder.clear();
  }
}

bool RenderGraph::canCompile() const
{
  // Check if we have a GfxRenderer and HinaContext
  if (!m_gfxRenderer) return false;
  HinaContext* hinaCtx = m_gfxRenderer->getHinaContext();
  if (!hinaCtx) return false;
  // Check if we have a swapchain
  if (!hinaCtx->hasSwapchain()) return false;
  gfx::Texture swapchainTex = hinaCtx->getCurrentSwapchainHinaTexture();
  if (!gfx::isValid(swapchainTex)) return false;
  // Verify dimensions are valid
  gfx::Dimensions dims = hinaCtx->getDimensions(swapchainTex);
  if (dims.width == 0 || dims.height == 0) return false;
  return true;
}

bool RenderGraph::hasMinimumResources() const
{
  // Features use their own storage (GfxMeshStorage, GfxMaterialSystem)
  return true;
}

bool RenderGraph::tryCompile()
{
  if (!canCompile())
  {
    m_compilationDeferred = true;
    m_isCompiled = false;
    LOG_INFO("RenderGraph compilation deferred - swapchain not ready");
    return false;
  }
  if (!hasMinimumResources())
  {
    LOG_ERROR("RenderGraph cannot compile - missing required buffers");
    m_compilationDeferred = true;
    m_isCompiled = false;
    return false;
  }
  m_compilationDeferred = false;
  Compile();
  return m_isCompiled;
}

void RenderGraph::Compile()
{
  m_passExecData.clear();
  m_passMetadata.clear();
  m_executionLambdas.clear();
  m_compiledPassOrder.clear();
  m_executionSteps.clear();
  m_resolvedResourceInfo.clear();
  m_storedTextures.clear();
  m_frameBuffers = {};
  m_handleStorage.clear();
  // Reset string interning
  m_stringStorage.clear();
  m_nameRegistry.clear();
  m_idToStringMap.clear();
  m_nextNameID = 1;
  m_swapchainID = InternName(RenderResources::SWAPCHAIN_IMAGE);
  m_viewOutputID = InternName(RenderResources::VIEW_OUTPUT);

  // Get context from GfxRenderer
  HinaContext* hinaCtx = m_gfxRenderer ? m_gfxRenderer->getHinaContext() : nullptr;
  if (!hinaCtx) {
    LOG_WARNING("Cannot compile RenderGraph - no HinaContext");
    m_isCompiled = false;
    return;
  }
  // Validate swapchain availability
  if (!hinaCtx->hasSwapchain())
  {
    LOG_WARNING("Cannot compile RenderGraph - no swapchain");
    m_isCompiled = false;
    return;
  }
  gfx::Texture initialSwapchainTexture = hinaCtx->getCurrentSwapchainHinaTexture();
  if (!gfx::isValid(initialSwapchainTexture))
  {
    LOG_WARNING("Cannot compile RenderGraph - invalid swapchain texture");
    m_isCompiled = false;
    return;
  }
  gfx::Dimensions swapchainDims = hinaCtx->getDimensions(initialSwapchainTexture);
  if (swapchainDims.width == 0 || swapchainDims.height == 0)
  {
    LOG_WARNING("Cannot compile RenderGraph - invalid dimensions");
    m_isCompiled = false;
    return;
  }
  m_lastCompileDimensions = {swapchainDims.width, swapchainDims.height, swapchainDims.depth};
  gfx::TextureDesc swapchainDesc{
    .type = HINA_TEX_TYPE_2D,
    .format = static_cast<hina_format>(hinaCtx->getSwapchainFormat()),
    .width = swapchainDims.width,
    .height = swapchainDims.height,
    .depth = 1,
    .layers = 1,
    .mip_levels = 1,
    .usage = static_cast<hina_texture_usage_flags>(gfx::TextureUsage::RenderTarget)
  };
  // Register external resources
  ImportExternalTexture(RenderResources::SWAPCHAIN_IMAGE, initialSwapchainTexture, swapchainDesc);
  // Register standard render targets at fixed internal resolution
  // This avoids recompilation on window resize - only swapchain resources change
  linearColorSystem.RegisterLinearColorResources(*this);
  // SCENE_DEPTH is the shared depth buffer - G-buffer pass writes to it directly
  // InputAttachment is required for tile pass depth input (subpass reads depth from tile memory)
  RegisterTransientResource(RenderResources::SCENE_DEPTH, gfx::TextureDesc{
                              .type = HINA_TEX_TYPE_2D,
                              .format = HINA_FORMAT_D32_SFLOAT,
                              .width = RenderResources::INTERNAL_WIDTH,
                              .height = RenderResources::INTERNAL_HEIGHT,
                              .depth = 1,
                              .layers = 1,
                              .mip_levels = 1,
                              .usage = static_cast<hina_texture_usage_flags>(gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled | gfx::TextureUsage::InputAttachment)
                            });
  for (IRenderFeature* feature : m_features)
  {
    internal::RenderPassBuilder builder(this, feature);
    feature->SetupPasses(builder);
  }
  {
    internal::RenderPassBuilder builder(this, nullptr);
    linearColorSystem.RegisterToneMappingPass(builder);
  }
  if (m_passExecData.empty())
  {
    m_isCompiled = false;
    return;
  }

  // Register VIEW_OUTPUT as external resource (will be updated per-Execute with actual texture)
  // This is a placeholder - the concrete handle is set in UpdateViewOutputResource()
  if (auto* info = GetResolvedResource(m_viewOutputID))
  {
    // Get actual VIEW_OUTPUT format from GfxRenderer (matches swapchain format)
    HinaContext* hinaCtx = m_gfxRenderer ? m_gfxRenderer->getHinaContext() : nullptr;
    hina_format viewOutputFormat = hinaCtx ? static_cast<hina_format>(hinaCtx->getSwapchainFormat())
                                           : HINA_FORMAT_B8G8R8A8_SRGB;
    *info = ResolvedResourceInfo{
      .concreteHandle = ResourceHandle{},  // Placeholder - set in Execute()
      .definition = ResourceProperties::FromDesc(gfx::TextureDesc{
        .type = HINA_TEX_TYPE_2D,
        .format = viewOutputFormat,  // Must match actual VIEW_OUTPUT texture format
        .width = RenderResources::INTERNAL_WIDTH,
        .height = RenderResources::INTERNAL_HEIGHT,
        .depth = 1,
        .layers = 1,
        .mip_levels = 1,
        // ViewOutput is a render target for the resolve pass (fullscreen tonemap) and sampled by ImGui
        .usage = static_cast<hina_texture_usage_flags>(gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled | gfx::TextureUsage::TransferSrc)
      }, true),
      .isExternal = true,
      .firstUsePassIndex = ~0u,
      .lastUsePassIndex = 0
    };
  }

  // ResolveViewOutput pass - copies SCENE_COLOR to VIEW_OUTPUT (per-view texture for ImGui to sample)
  PassExecutionData viewResolveExec{
    .passID = InternName("ResolveViewOutput"), .passType = 1,
    .handleCount = 2, .handleOffset = 0, .executeLambda = nullptr
  };
  PassMetadata viewResolveMeta{
    .debugName = "ResolveViewOutput",
    .declaredResourceAccesses = {
      {InternName(RenderResources::SCENE_COLOR), AccessType::Read},
      {m_viewOutputID, AccessType::Write}
    },
    .priority = static_cast<internal::RenderPassBuilder::PassPriority>(640),
    .featureOwner = nullptr
  };
  ExecuteLambda viewResolveLambda = [this](internal::ExecutionContext& ctx)
  {
    ExecuteResolveViewOutput(ctx);
  };
  RegisterPass(std::move(viewResolveExec), std::move(viewResolveMeta), std::move(viewResolveLambda));

  // FinalBlit pass - copies VIEW_OUTPUT to SWAPCHAIN_IMAGE for presentation
  // VIEW_OUTPUT contains the composited scene + ImGui (if enabled)
  // Only runs for the presented view (swapchainImage is only valid for that view)
  PassExecutionData finalBlitExec{
    .passID = InternName("FinalBlit"), .passType = 1,
    .handleCount = 2, .handleOffset = 0, .executeLambda = nullptr
  };
  PassMetadata finalBlitMeta{
    .debugName = "FinalBlit",
    .declaredResourceAccesses = {
      {m_viewOutputID, AccessType::Read},
      {m_swapchainID, AccessType::Write}
    },
    .priority = internal::RenderPassBuilder::PassPriority::Present,
    .featureOwner = nullptr
  };
  ExecuteLambda finalBlitLambda = [this](internal::ExecutionContext& ctx)
  {
    ExecuteFinalBlit(ctx);
  };
  RegisterPass(std::move(finalBlitExec), std::move(finalBlitMeta), std::move(finalBlitLambda));

  // Sort passes
  BuildAdjacencyListAndSort();
  if (m_compiledPassOrder.empty())
  {
    m_isCompiled = false;
    return;
  }
  for (size_t passIdx : m_compiledPassOrder)
  {
    auto& execData = m_passExecData[passIdx];
    const auto& metadata = m_passMetadata[passIdx];
    execData.handleOffset = static_cast<uint16_t>(m_handleStorage.size());
    execData.handleCount = static_cast<uint8_t>(metadata.declaredResourceAccesses.size());
    for (const auto& usage : metadata.declaredResourceAccesses)
    {
      if (auto* info = GetResolvedResource(usage.id))
      {
        info->firstUsePassIndex = std::min(info->firstUsePassIndex, static_cast<uint32_t>(passIdx));
        info->lastUsePassIndex = std::max(info->lastUsePassIndex, static_cast<uint32_t>(passIdx));
        m_handleStorage.push_back(info->concreteHandle); // Still a placeholder at this stage
      }
      else
      {
        m_handleStorage.emplace_back(); // Push invalid handle
      }
    }
  }
  for (size_t i = 0; i < m_resolvedResourceInfo.size(); ++i)
  {
    auto& info = m_resolvedResourceInfo[i];
    internal::NameID id = static_cast<internal::NameID>(i + 1);
    if (info.isExternal || info.firstUsePassIndex == ~0u)
    {
      continue;
    }
    if (info.definition.type == ResourceType::Texture)
    {
      gfx::TextureDesc actualDesc = std::get<gfx::TextureDesc>(info.definition.desc);
      // Check for swapchain-relative dimensions (0,0,0)
      if (actualDesc.width == 0 && actualDesc.height == 0) {
        actualDesc.width = m_lastCompileDimensions.width;
        actualDesc.height = m_lastCompileDimensions.height;
        actualDesc.depth = 1;
      }
      // Check for undefined format - inherit from swapchain
      if (actualDesc.format == HINA_FORMAT_UNDEFINED) {
        actualDesc.format = static_cast<hina_format>(hinaCtx->getSwapchainFormat());
      }
      m_storedTextures.emplace_back(hinaCtx->createTextureHolder(actualDesc));
      info.concreteHandle = ResourceHandle(m_storedTextures.back().get());
    }
    else if (info.definition.type == ResourceType::Buffer)
    {
      const gfx::BufferDesc& desc = std::get<gfx::BufferDesc>(info.definition.desc);
      uint16_t entryIdx = m_frameBuffers.AllocateEntry(id);
      if (entryIdx >= internal::MAX_RESOURCES)
      {
        continue;
      }
      auto* entry = &m_frameBuffers.m_resources[entryIdx];
      entry->desc = desc;
      entry->currentSize = desc.size;
      entry->active = true;
      EnsureBufferCapacity(*entry, desc.size);
      info.concreteHandle = ResourceHandle(entry->buffers[m_currentFrameIndex]);
    }
  }
  // Update handle storage with concrete resources
  for (size_t passIdx : m_compiledPassOrder)
  {
    const auto& execData = m_passExecData[passIdx];
    const auto& metadata = m_passMetadata[passIdx];
    for (size_t i = 0; i < execData.handleCount; ++i)
    {
      const auto& usage = metadata.declaredResourceAccesses[i];
      if (auto* info = GetResolvedResource(usage.id))
      {
        const bool isTransientBuffer = !info->isExternal && !info->definition.persistent && info->definition.type ==
          ResourceType::Buffer;
        if (!isTransientBuffer) m_handleStorage[execData.handleOffset + i] = info->concreteHandle;
      }
    }
  }
  // Batch graphics passes and create execution steps
  m_executionSteps.reserve(m_compiledPassOrder.size());
  size_t i = 0;
  while (i < m_compiledPassOrder.size())
  {
    size_t passIdx = m_compiledPassOrder[i];
    const auto& execData = m_passExecData[passIdx];
    if (execData.passType == 1)
    {
      // Generic
      m_executionSteps.emplace_back(passIdx);
      ++i;
      continue;
    }
    // Start graphics batch
    BatchedGraphicsPassExecution batch{};
    const auto& firstMeta = m_passMetadata[passIdx];
    FillRenderPassFromDeclaration(firstMeta.graphicsDeclaration, batch.renderPassInfo);
    std::vector<gfx::Texture> textures;
    std::vector<gfx::Buffer> buffers;
    batch.passIndices[0] = passIdx;
    batch.passCount = 1;
    AggregateDependenciesForPass(passIdx, textures, buffers);
    // Try to batch compatible passes
    size_t j = i + 1;
    while (j < m_compiledPassOrder.size() && batch.passCount < 8)
    {
      size_t nextIdx = m_compiledPassOrder[j];
      const auto& nextExecData = m_passExecData[nextIdx];
      const auto& nextMeta = m_passMetadata[nextIdx];
      if (nextExecData.passType == 0 && // Graphics
        AreGraphicsDeclarationsEqual(firstMeta.graphicsDeclaration, nextMeta.graphicsDeclaration))
      {
        batch.passIndices[batch.passCount++] = nextIdx;
        AggregateDependenciesForPass(nextIdx, textures, buffers);
        ++j;
      }
      else
      {
        break;
      }
    }
    // Sort and unique dependencies for better performance
    std::sort(textures.begin(), textures.end());
    textures.erase(std::unique(textures.begin(), textures.end()), textures.end());
    std::sort(buffers.begin(), buffers.end());
    buffers.erase(std::unique(buffers.begin(), buffers.end()), buffers.end());
    // Fill dependencies
    int texIdx = 0;
    for (const auto& tex : textures)
    {
      if (texIdx < gfx::MAX_SUBMIT_DEPENDENCIES)
      {
        batch.accumulatedDependencies.textures[texIdx++] = tex;
      }
    }
    int bufIdx = 0;
    for (const auto& buf : buffers)
    {
      if (bufIdx < gfx::MAX_SUBMIT_DEPENDENCIES)
      {
        batch.accumulatedDependencies.buffers[bufIdx++] = buf;
      }
    }
    m_executionSteps.emplace_back(std::move(batch));
    i = j;
  }
  m_isCompiled = true;
}

void RenderGraph::RegisterTransientResource(LogicalResourceName name, const ResourceProperties& properties)
{
  if (!name || !name[0]) return;
  internal::NameID id = InternName(name);
  if (id == internal::INVALID_NAME_ID) return;
  if (auto* info = GetResolvedResource(id))
  {
    *info = ResolvedResourceInfo{
      .id = id, .concreteHandle = ResourceHandle{}, .definition = properties, .isExternal = false,
      .firstUsePassIndex = ~0u, .lastUsePassIndex = 0
    };
  }
}

void RenderGraph::RegisterTransientResource(LogicalResourceName name, const gfx::TextureDesc& desc)
{
  RegisterTransientResource(name, ResourceProperties::FromDesc(desc, false));
}

void RenderGraph::RegisterTransientResource(LogicalResourceName name, const gfx::BufferDesc& desc)
{
  RegisterTransientResource(name, ResourceProperties::FromDesc(desc, false));
}

void RenderGraph::RegisterPass(PassExecutionData&& execData, PassMetadata&& metadata, ExecuteLambda&& lambda)
{
  // Store lambda separately and keep pointer in exec data
  m_executionLambdas.push_back(std::move(lambda));
  execData.executeLambda = &m_executionLambdas.back();
  m_passExecData.push_back(std::move(execData));
  m_passMetadata.push_back(std::move(metadata));
}

// Blit shader for scaling fixed-resolution scene to swapchain (HSL format)
static const char* kBlitShader = R"(#hina
group Blit = 0;

bindings(Blit, start=0) {
  texture sampler2D uSourceTexture;
}

push_constant BlitData {
  uint preTransform; // 0=Identity, 1=Rotate90, 2=Rotate180, 3=Rotate270
} pc;

struct Varyings {
  vec2 uv;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain() {
  Varyings out;
  out.uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  // Vulkan NDC (with a normal, positive-height viewport): y = -1 is top, y = +1 is bottom.
  // Generate a fullscreen triangle that matches that convention (no implicit vertical flip).
  gl_Position = vec4(out.uv * vec2(2, 2) + vec2(-1, -1), 0.0, 1.0);
  return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
  FragOut out;
  vec2 transformedUV = in.uv;
  // Transform UV coordinates based on Android pre-rotation
  if (pc.preTransform == 1u) { // Rotate90
    transformedUV = vec2(in.uv.y, 1.0 - in.uv.x);
  } else if (pc.preTransform == 2u) { // Rotate180
    transformedUV = vec2(1.0 - in.uv.x, 1.0 - in.uv.y);
  } else if (pc.preTransform == 3u) { // Rotate270
    transformedUV = vec2(1.0 - in.uv.y, in.uv.x);
  }
  out.color = texture(uSourceTexture, transformedUV);
  return out;
}
#hina_end
)";

void RenderGraph::EnsureBlitPipelineCreated()
{
  if (m_blitPipelineCreated) return;
  if (!m_gfxRenderer) return;
  HinaContext* hinaCtx = m_gfxRenderer->getHinaContext();
  if (!hinaCtx) return;

  // Compile HSL shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kBlitShader, "blit_shader", &error);
  if (!module) {
    LOG_ERROR("[RenderGraph] Blit shader compilation failed: {}", error ? error : "Unknown");
    if (error) hslc_free_log(error);
    return;
  }

  // Create sampler
  {
    gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_w = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_blitSampler.reset(gfx::createSampler(samplerDesc));
  }

  // Create bind group layout for source texture
  {
    hina_bind_group_layout_entry layout_entries[1] = {};
    layout_entries[0].binding = 0;
    layout_entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_entries[0].stage_flags = HINA_STAGE_FRAGMENT;  // Must use bit flag, not enum
    layout_entries[0].count = 1;

    hina_bind_group_layout_desc layout_desc = {};
    layout_desc.entries = layout_entries;
    layout_desc.entry_count = 1;

    m_blitBindGroupLayout.reset(hina_create_bind_group_layout(&layout_desc));
  }

  // Create pipeline using HSL module
  {
    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;
    pip_desc.depth.depth_test = false;
    pip_desc.depth.depth_write = false;
    pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

    // Color format - query actual surface format from hina-vk
    hina_format surfaceFormat = hina_get_surface_format();
    pip_desc.color_formats[0] = surfaceFormat != HINA_FORMAT_UNDEFINED
                                  ? surfaceFormat
                                  : HINA_FORMAT_B8G8R8A8_SRGB;

    // Use the blit bind group layout
    if (hina_bind_group_layout_is_valid(m_blitBindGroupLayout.get())) {
      pip_desc.bind_group_layouts[0] = m_blitBindGroupLayout.get();
    }

    m_blitPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  }

  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_blitPipeline.get())) {
    LOG_ERROR("[RenderGraph] Blit pipeline creation failed");
    return;
  }

  m_blitPipelineCreated = true;
}

void RenderGraph::EnsureResolveOutputPipelineCreated()
{
  if (m_resolveOutputPipelineCreated) return;
  if (!m_gfxRenderer) return;
  HinaContext* hinaCtx = m_gfxRenderer->getHinaContext();
  if (!hinaCtx) return;

  // Load shader from file
  std::string shaderSource;
  if (!VFS::ReadFile("shaders/resolve_output.hina_sl", shaderSource)) {
    LOG_ERROR("[RenderGraph] Failed to load resolve_output.hina_sl shader");
    return;
  }

  // Compile HSL shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(shaderSource.c_str(), "resolve_output_shader", &error);
  if (!module) {
    LOG_ERROR("[RenderGraph] Resolve output shader compilation failed: {}", error ? error : "Unknown");
    if (error) hslc_free_log(error);
    return;
  }

  // Create sampler
  {
    gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_w = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_resolveOutputSampler.reset(gfx::createSampler(samplerDesc));
  }

  // Create bind group layout for HDR scene color texture
  {
    hina_bind_group_layout_entry layout_entries[1] = {};
    layout_entries[0].binding = 0;
    layout_entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_entries[0].stage_flags = HINA_STAGE_FRAGMENT;
    layout_entries[0].count = 1;

    hina_bind_group_layout_desc layout_desc = {};
    layout_desc.entries = layout_entries;
    layout_desc.entry_count = 1;

    m_resolveOutputBindGroupLayout.reset(hina_create_bind_group_layout(&layout_desc));
  }

  // Create pipeline using HSL module
  {
    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;
    pip_desc.depth.depth_test = false;
    pip_desc.depth.depth_write = false;
    pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

    // Output to VIEW_OUTPUT - use the actual swapchain format
    // Note: VIEW_OUTPUT textures are created with the swapchain format in GfxRenderer
    // We can't use HINA_FORMAT_SWAPCHAIN here because it may not resolve correctly
    // if the swapchain isn't fully initialized when this pipeline is created
    HinaContext* hinaCtx = m_gfxRenderer ? m_gfxRenderer->getHinaContext() : nullptr;
    pip_desc.color_formats[0] = hinaCtx ? static_cast<hina_format>(hinaCtx->getSwapchainFormat())
                                        : HINA_FORMAT_B8G8R8A8_SRGB;

    // Use the resolve output bind group layout
    if (hina_bind_group_layout_is_valid(m_resolveOutputBindGroupLayout.get())) {
      pip_desc.bind_group_layouts[0] = m_resolveOutputBindGroupLayout.get();
    }

    m_resolveOutputPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  }

  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_resolveOutputPipeline.get())) {
    LOG_ERROR("[RenderGraph] Resolve output pipeline creation failed");
    return;
  }

  LOG_INFO("[RenderGraph] Resolve output pipeline created successfully");
  m_resolveOutputPipelineCreated = true;
}

void RenderGraph::ExecuteFinalBlit(const internal::ExecutionContext& ctx)
{
  // Check swapchain validity before accessing resources
  // For non-presented views, swapchain is intentionally set invalid
  if (auto* swapInfo = GetResolvedResource(m_swapchainID))
  {
    if (!swapInfo->concreteHandle.isValid()) return;
  }
  else
  {
    return;
  }

  // Copy VIEW_OUTPUT (composited scene + ImGui) to swapchain
  gfx::Texture viewOutput = ctx.GetTexture(RenderResources::VIEW_OUTPUT);
  gfx::Texture swapchainImage = ctx.GetTexture(RenderResources::SWAPCHAIN_IMAGE);
  if (!gfx::isValid(viewOutput) || !gfx::isValid(swapchainImage)) return;

  // Skip if VIEW_OUTPUT == swapchain (non-ImGui case where ResolveViewOutput already wrote to swapchain)
  if (viewOutput == swapchainImage) return;

  HinaContext* hinaCtx = m_gfxRenderer ? m_gfxRenderer->getHinaContext() : nullptr;
  if (!hinaCtx) return;

  gfx::Dimensions viewDims = hinaCtx->getDimensions(viewOutput);
  gfx::Dimensions swapchainDims = hinaCtx->getDimensions(swapchainImage);

  // Get pre-transform for Android
  uint32_t preTransformValue = 0; // 0 = Identity
#if defined(__ANDROID__)
  gfx::SurfaceTransform transform = hinaCtx->getSwapchainPreTransform();
  switch (transform)
  {
    case gfx::SurfaceTransform::Rotate90:
      preTransformValue = 1;
      break;
    case gfx::SurfaceTransform::Rotate180:
      preTransformValue = 2;
      break;
    case gfx::SurfaceTransform::Rotate270:
      preTransformValue = 3;
      break;
    default:
      preTransformValue = 0;
      break;
  }
#endif

  // Fast blit path: for matching dimensions and no pre-transform
  if (preTransformValue == 0 && viewDims.width == swapchainDims.width && viewDims.height == swapchainDims.height)
  {
    // Use hina blit for copy to swapchain
    hina_cmd_blit_texture(ctx.GetCmd(), viewOutput, swapchainImage, 0, 0, HINA_FILTER_NEAREST);
    return;
  }

  // Use shader-based blit to scale from fixed internal resolution to swapchain size
  // and apply pre-rotation on Android
  this->EnsureBlitPipelineCreated();

  gfx::CommandBuffer& gfxCmd = ctx.GetCommandBuffer();

  // Begin rendering to swapchain
  gfx::RenderPass renderPassInfo{};
  renderPassInfo.color[0] = {.loadOp = gfx::LoadOp::DontCare, .storeOp = gfx::StoreOp::Store};

  gfx::Framebuffer framebuffer{};
  framebuffer.color[0] = {.texture = swapchainImage};

  gfx::Dependencies deps{};
  deps.textures[0] = viewOutput;

  gfxCmd.beginRendering(renderPassInfo, framebuffer, deps);

  // Bind pipeline and set up rendering
  gfxCmd.bindPipeline(m_blitPipeline.get());
  gfxCmd.setViewport({
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(swapchainDims.width),
    .height = static_cast<float>(swapchainDims.height)
  });
  gfxCmd.setScissor({
    .x = 0,
    .y = 0,
    .width = swapchainDims.width,
    .height = swapchainDims.height
  });

  // Create transient bind group for source texture using the stored layout
  gfx::TextureView sourceView = hina_texture_get_default_view(viewOutput);
  hina_transient_bind_group tbg = hina_alloc_transient_bind_group(m_blitBindGroupLayout.get());
  hina_transient_write_combined_image(&tbg, 0, sourceView, m_blitSampler.get());
  hina_cmd_bind_transient_group(ctx.GetCmd(), 0, tbg);

  struct BlitPushConstants {
    uint32_t preTransform; // 0=Identity, 1=Rotate90, 2=Rotate180, 3=Rotate270
  } pc = {
    .preTransform = preTransformValue
  };
  gfxCmd.pushConstants(pc);
  gfxCmd.draw(3);

  gfxCmd.endRendering();
}

void RenderGraph::ExecuteResolveViewOutput(const internal::ExecutionContext& ctx)
{
  gfx::Texture sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  gfx::Texture viewOutput = ctx.GetTexture(RenderResources::VIEW_OUTPUT);

  if (!gfx::isValid(sceneColor) || !gfx::isValid(viewOutput)) {
    return;
  }

  HinaContext* hinaCtx = m_gfxRenderer ? m_gfxRenderer->getHinaContext() : nullptr;
  if (!hinaCtx) return;

  // Ensure resolve output pipeline with tone mapping is created
  this->EnsureResolveOutputPipelineCreated();
  if (!hina_pipeline_is_valid(m_resolveOutputPipeline.get())) {
    // Fallback to blit pipeline if resolve pipeline failed
    this->EnsureBlitPipelineCreated();
    if (!hina_pipeline_is_valid(m_blitPipeline.get())) {
      return;
    }
  }

  gfx::Dimensions dstDims = hinaCtx->getDimensions(viewOutput);
  gfx::Cmd* cmd = ctx.GetCmd();

  // Transition sceneColor to shader read for sampling
  hina_cmd_transition_texture(cmd, sceneColor, HINA_TEXSTATE_SHADER_READ);

  // Set up pass action directly
  hina_pass_action pass = {};
  pass.width = dstDims.width;
  pass.height = dstDims.height;
  pass.colors[0].image = hina_texture_get_default_view(viewOutput);
  pass.colors[0].load_op = HINA_LOAD_OP_DONT_CARE;
  pass.colors[0].store_op = HINA_STORE_OP_STORE;

  hina_cmd_begin_pass(cmd, &pass);

  // Use resolve output pipeline with tone mapping if available
  bool useResolveOutput = hina_pipeline_is_valid(m_resolveOutputPipeline.get());
  if (useResolveOutput) {
    hina_cmd_bind_pipeline(cmd, m_resolveOutputPipeline.get());
  } else {
    hina_cmd_bind_pipeline(cmd, m_blitPipeline.get());
  }

  hina_viewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(dstDims.width);
  viewport.height = static_cast<float>(dstDims.height);
  viewport.min_depth = 0.0f;
  viewport.max_depth = 1.0f;
  hina_cmd_set_viewport(cmd, &viewport);

  hina_scissor scissor = {};
  scissor.x = 0;
  scissor.y = 0;
  scissor.width = dstDims.width;
  scissor.height = dstDims.height;
  hina_cmd_set_scissor(cmd, &scissor);

  // Create transient bind group for source texture
  gfx::TextureView sourceView = hina_texture_get_default_view(sceneColor);
  if (useResolveOutput) {
    hina_transient_bind_group tbg = hina_alloc_transient_bind_group(m_resolveOutputBindGroupLayout.get());
    hina_transient_write_combined_image(&tbg, 0, sourceView, m_resolveOutputSampler.get());
    hina_cmd_bind_transient_group(cmd, 0, tbg);

    // Get tone mapping settings from LinearColorSystem
    const auto& settings = linearColorSystem.GetToneMappingSettings();
    ResolveOutputPushConstants pc = {
      .toneMappingMode = static_cast<int32_t>(settings.mode),
      .exposure = settings.exposure,
      .maxWhite = settings.maxWhite,
      .P = settings.P,
      .a = settings.a,
      .m = settings.m,
      .l = settings.l,
      .c = settings.c,
      .b = settings.b
    };
    hina_cmd_push_constants(cmd, 0, sizeof(pc), &pc);
  } else {
    // Fallback to blit pipeline
    hina_transient_bind_group tbg = hina_alloc_transient_bind_group(m_blitBindGroupLayout.get());
    hina_transient_write_combined_image(&tbg, 0, sourceView, m_blitSampler.get());
    hina_cmd_bind_transient_group(cmd, 0, tbg);

    struct BlitPushConstants {
      uint32_t preTransform;
    } pc = {.preTransform = 0};
    hina_cmd_push_constants(cmd, 0, sizeof(pc), &pc);
  }

  hina_cmd_draw(cmd, 3, 1, 0, 0);

  hina_cmd_end_pass(cmd);

  // Explicitly transition VIEW_OUTPUT to SHADER_READ for ImGui to sample it
  hina_cmd_transition_texture(ctx.GetCmd(), viewOutput, HINA_TEXSTATE_SHADER_READ);
}

void RenderGraph::UpdateViewOutputResource(gfx::Texture outputHandle)
{
  ResourceHandle newHandle = gfx::isValid(outputHandle) ? ResourceHandle(outputHandle) : ResourceHandle{};

  if (auto* info = GetResolvedResource(m_viewOutputID))
  {
    info->concreteHandle = newHandle;
  }
  else
  {
    return;
  }

  // Also update handle storage for all passes that use VIEW_OUTPUT
  // This is necessary because handle storage is populated during compilation
  // and external resources like VIEW_OUTPUT are updated per-frame
  for (size_t passIdx = 0; passIdx < m_passExecData.size(); ++passIdx)
  {
    const auto& execData = m_passExecData[passIdx];
    const auto& metadata = m_passMetadata[passIdx];
    for (uint8_t i = 0; i < execData.handleCount; ++i)
    {
      if (metadata.declaredResourceAccesses[i].id == m_viewOutputID)
      {
        m_handleStorage[execData.handleOffset + i] = newHandle;
      }
    }
  }
}

ResourceHandle RenderGraph::GetResolvedHandle(internal::NameID id) const
{
  if (const auto* info = GetResolvedResource(id))
  {
    return info->concreteHandle;
  }
  return ResourceHandle{};
}

void RenderGraph::AddFeature(IRenderFeature* feature)
{
  if (feature)
  {
    m_features.push_back(feature);
    Invalidate();
  }
}

void RenderGraph::RemoveFeature(IRenderFeature* feature)
{
  if (!feature) return;
  if (auto it = std::find(m_features.begin(), m_features.end(), feature); it != m_features.end())
  {
    m_features.erase(it);
    Invalidate();
  }
}

FeatureMask RenderGraph::RegisterFeature(IRenderFeature* feature)
{
  if (!feature) return 0;

  // Check if already registered
  auto it = m_featureIdMap.find(feature);
  if (it != m_featureIdMap.end())
  {
    return FeatureBit(it->second);
  }

  // Assign new ID (max 64 features due to FeatureMask being uint64_t)
  if (m_nextFeatureId >= 64)
  {
    LOG_ERROR("Too many features registered (max 64)");
    return 0;
  }

  RenderFeatureId id = m_nextFeatureId++;
  m_featureIdMap[feature] = id;
  feature->SetFeatureId(id);

  LOG_INFO("Registered feature '{}' with ID {} (mask: 0x{:X})", feature->GetName(), id, FeatureBit(id));

  return FeatureBit(id);
}

void RenderGraph::UnregisterFeature(IRenderFeature* feature)
{
  if (!feature) return;
  m_featureIdMap.erase(feature);
  // Note: We don't recycle IDs to avoid mask confusion
}

FeatureMask RenderGraph::GetFeatureMask(IRenderFeature* feature) const
{
  if (!feature) return 0;
  auto it = m_featureIdMap.find(feature);
  if (it != m_featureIdMap.end())
  {
    return FeatureBit(it->second);
  }
  return 0;
}

void RenderGraph::ClearFeaturesAndPasses()
{
  m_features.clear();
  m_passExecData.clear();
  m_passMetadata.clear();
  m_executionLambdas.clear();
  m_compiledPassOrder.clear();
  m_executionSteps.clear();
  m_resolvedResourceInfo.clear();
  m_storedTextures.clear();
  m_frameBuffers = {};
  m_handleStorage.clear();
  // Reset string interning
  m_stringStorage.clear();
  m_nameRegistry.clear();
  m_idToStringMap.clear();
  m_nextNameID = 1;
  m_swapchainID = InternName(RenderResources::SWAPCHAIN_IMAGE);
  m_viewOutputID = InternName(RenderResources::VIEW_OUTPUT);
  Invalidate();
}

internal::NameID RenderGraph::InternName(LogicalResourceName name)
{
  if (!name) return internal::INVALID_NAME_ID;
  std::string_view sv(name);
  // Binary search in sorted vector for better cache locality
  const auto it = std::lower_bound(m_nameRegistry.begin(), m_nameRegistry.end(), sv,
                                   [](const auto& pair, std::string_view val)
                                   {
                                     return pair.first < val;
                                   });
  if (it != m_nameRegistry.end() && it->first == sv)
  {
    return it->second;
  }
  // Store string in deque for stable pointers
  m_stringStorage.emplace_back(name);
  std::string_view stored = m_stringStorage.back();
  // -----------------------------------------------------------
  internal::NameID newID = m_nextNameID++;
  m_nameRegistry.insert(it, {stored, newID});
  if (newID > m_idToStringMap.size())
  {
    m_idToStringMap.resize(newID);
  }
  m_idToStringMap[newID - 1] = stored;
  return newID;
}

internal::NameID RenderGraph::FindNameID(LogicalResourceName name) const
{
  if (!name) return internal::INVALID_NAME_ID;
  std::string_view sv(name);
  // Binary search in sorted vector
  auto it = std::lower_bound(m_nameRegistry.begin(), m_nameRegistry.end(), sv,
                             [](const auto& pair, std::string_view val)
                             {
                               return pair.first < val;
                             });
  if (it != m_nameRegistry.end() && it->first == sv)
  {
    return it->second;
  }
  return internal::INVALID_NAME_ID;
}

LogicalResourceName RenderGraph::GetNameString(internal::NameID id) const
{
  if (id == internal::INVALID_NAME_ID || id == 0 || id > m_idToStringMap.size())
  {
    return "[Invalid]";
  }
  return m_idToStringMap[id - 1].data();
}

RenderGraph::ResolvedResourceInfo* RenderGraph::GetResolvedResource(internal::NameID id)
{
  if (id == internal::INVALID_NAME_ID || id == 0) return nullptr;
  size_t idx = id - 1;
  if (idx >= m_resolvedResourceInfo.size())
  {
    m_resolvedResourceInfo.resize(id);
  }
  return &m_resolvedResourceInfo[idx];
}

const RenderGraph::ResolvedResourceInfo* RenderGraph::GetResolvedResource(internal::NameID id) const
{
  if (id == internal::INVALID_NAME_ID || id == 0 || id > m_resolvedResourceInfo.size())
  {
    return nullptr;
  }
  return &m_resolvedResourceInfo[id - 1];
}

internal::NameID RenderGraph::ImportExternalTexture(LogicalResourceName name, gfx::Texture textureHandle,
                                                    const gfx::TextureDesc& desc)
{
  internal::NameID id = InternName(name);
  if (id == internal::INVALID_NAME_ID) return id;
  if (auto* info = GetResolvedResource(id))
  {
    *info = ResolvedResourceInfo{
      .concreteHandle = ResourceHandle(textureHandle), .definition = ResourceProperties::FromDesc(desc, true),
      .isExternal = true, .firstUsePassIndex = ~0u, .lastUsePassIndex = 0
    };
  }
  return id;
}

internal::NameID RenderGraph::ImportExternalBuffer(LogicalResourceName name, gfx::Buffer bufferHandle,
                                                   const gfx::BufferDesc& desc)
{
  internal::NameID id = InternName(name);
  if (id == internal::INVALID_NAME_ID) return id;
  if (auto* info = GetResolvedResource(id))
  {
    *info = ResolvedResourceInfo{
      .concreteHandle = ResourceHandle(bufferHandle), .definition = ResourceProperties::FromDesc(desc, true),
      .isExternal = true, .firstUsePassIndex = ~0u, .lastUsePassIndex = 0
    };
  }
  return id;
}

void RenderGraph::Execute(gfx::Cmd* cmd, const FrameData& frameData, const ViewOutputConfig& outputConfig)
{
  // Note: BeginFrame() should be called explicitly by the caller before Execute()
  // This allows multi-view rendering where Execute() is called multiple times per frame

  // Handle deferred compilation
  if (m_compilationDeferred && canCompile())
  {
    LOG_INFO("Executing deferred RenderGraph compilation");
    if (!tryCompile())
    {
      LOG_ERROR("Deferred compilation failed");
      return;
    }
  }
  // NOTE: We no longer invalidate on swapchain resize because:
  // - Internal render resolution is fixed at INTERNAL_RESOLUTION (1920x1080)
  // - Only the final blit pass scales to swapchain size
  // - This avoids transient buffer reallocation issues on resize
  // Early exit if can't compile
  if (!canCompile())
  {
    return; // Silent exit on Android when no surface
  }
  if (!m_isCompiled)
  {
    if (!tryCompile())
    {
      LOG_WARNING("Failed to compile RenderGraph");
      return;
    }
  }

  // Always set swapchain correctly for this view (regardless of whether we just compiled or not)
  // Only the presented view should have a valid swapchain so FinalBlit runs only once
  if (gfx::isValid(outputConfig.swapchainImage))
  {
    UpdateSwapchainResource();
  }
  else
  {
    // Set swapchain to invalid for non-presented views so FinalBlit skips
    if (auto* info = GetResolvedResource(m_swapchainID))
    {
      info->concreteHandle = ResourceHandle{};
    }
  }
  // Update external output textures
  if (gfx::isValid(outputConfig.resolvedColor))
  {
    UpdateViewOutputResource(outputConfig.resolvedColor);
  }
  if (!m_isCompiled || m_executionSteps.empty()) {
    LOG_WARNING("[RenderGraph] Cannot execute: compiled={}, steps={}", m_isCompiled, m_executionSteps.size());
    return;
  }

  // Set the active feature mask from the view's featureMask
  m_activeFeatureMask = frameData.featureMask;

  // Helper lambda to check if a pass should execute based on feature mask
  auto shouldExecutePass = [this](const PassMetadata& metadata) -> bool {
    // Passes with no required feature mask (system passes) always execute
    if (metadata.requiredFeatureMask == 0) return true;
    // Check if any required feature is enabled in the active mask
    return (metadata.requiredFeatureMask & m_activeFeatureMask) != 0;
  };

  gfx::CommandBuffer cmdWrapper(cmd);

  // Execute passes
  for (const auto& step : m_executionSteps)
  {
    if (step.type == CompiledExecutionStep::Type::Generic)
    {
      size_t passIdx = step.genericPassIndex;
      const auto& execData = m_passExecData[passIdx];
      const auto& metadata = m_passMetadata[passIdx];

      // Skip passes whose required features are not enabled
      if (!shouldExecutePass(metadata)) continue;

      if (metadata.debugName && metadata.debugName[0])
      {
        cmdWrapper.pushDebugLabel(metadata.debugName);
      }
      internal::ExecutionContext context(cmd, frameData, this, passIdx);
      (*execData.executeLambda)(context);
      if (metadata.debugName && metadata.debugName[0])
      {
        cmdWrapper.popDebugLabel();
      }
    }
    else
    {
      const auto& batch = step.graphicsBatch;

      // For batched passes, check if ANY pass in the batch should execute
      // (batch contains passes with compatible render targets)
      bool shouldExecuteBatch = false;
      for (uint8_t i = 0; i < batch.passCount; ++i)
      {
        if (shouldExecutePass(m_passMetadata[batch.passIndices[i]]))
        {
          shouldExecuteBatch = true;
          break;
        }
      }
      if (!shouldExecuteBatch) continue;

      gfx::Framebuffer framebuffer;
      FillFramebufferFromDeclaration(m_passMetadata[batch.passIndices[0]].graphicsDeclaration, framebuffer);
      cmdWrapper.beginRendering(batch.renderPassInfo, framebuffer, batch.accumulatedDependencies);
      for (uint8_t i = 0; i < batch.passCount; ++i)
      {
        size_t passIdx = batch.passIndices[i];
        const auto& execData = m_passExecData[passIdx];
        const auto& metadata = m_passMetadata[passIdx];

        // Still filter individual passes within the batch
        if (!shouldExecutePass(metadata)) continue;

        if (metadata.debugName && metadata.debugName[0])
        {
          cmdWrapper.pushDebugLabel(metadata.debugName);
        }
        internal::ExecutionContext context(cmd, frameData, this, passIdx);
        (*execData.executeLambda)(context);
        if (metadata.debugName && metadata.debugName[0])
        {
          cmdWrapper.popDebugLabel();
        }
      }
      cmdWrapper.endRendering();
    }
  }
}

void RenderGraph::UpdateSwapchainResource()
{
  HinaContext* hinaCtx = m_gfxRenderer ? m_gfxRenderer->getHinaContext() : nullptr;
  if (!hinaCtx) {
    Invalidate();
    return;
  }
  gfx::Texture swapchainTexture = hinaCtx->getCurrentSwapchainHinaTexture();
  if (auto* info = GetResolvedResource(m_swapchainID))
  {
    info->concreteHandle = ResourceHandle(swapchainTexture);
  }
  else
  {
    Invalidate();
  }
}
