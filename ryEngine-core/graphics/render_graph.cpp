#include "render_graph.h"
#include <map>
#include <queue>
#include <cstring>
#include <algorithm>
#include <numeric>

void RenderGraph::EnsureBufferCapacity(internal::FrameBufferManager::Entry& resource, uint64_t requiredSize)
{
  // ** ADDED DEBUG LOG **

  if(resource.currentSize >= requiredSize && resource.buffers[0].valid())
  {
    return;
  }

  if(requiredSize == 0)
  {
    return;
  }

  resource.desc.size = requiredSize;
  resource.currentSize = requiredSize;

  // Recreate all frame buffers with new size
  for(uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
  {
    resource.buffers[frame] = m_vkContext.createBuffer(resource.desc, ("TransientBuffer_Frame" + std::to_string(frame)).c_str());
  }
}

void RenderGraph::ResizeTransientBuffer(LogicalResourceName name, uint64_t newSize)
{
    internal::NameID id = FindNameID(name);
    if (id == internal::INVALID_NAME_ID)
        return;

    if (const auto* info = GetResolvedResource(id))
    {
        if (info->isExternal || info->definition.persistent)
            return;
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

void RenderGraph::AddTransientObserver(internal::ITransientResourceObserver* observer)
{
  if(observer && std::find(m_transientObservers.begin(), m_transientObservers.end(), observer) == m_transientObservers.end())
  {
    m_transientObservers.push_back(observer);
  }
}

void RenderGraph::RemoveTransientObserver(internal::ITransientResourceObserver* observer)
{
  std::erase(m_transientObservers, observer);
}

void RenderGraph::NotifyTransientObservers()
{
  if(m_transientObservers.empty())
    return;

  std::unordered_map<std::string_view, uint32_t> bindlessMap;
  bindlessMap.reserve(m_resolvedResourceInfo.size());

  for(size_t i = 0; i < m_resolvedResourceInfo.size(); ++i)
  {
    const auto& resourceInfo = m_resolvedResourceInfo[i];
    if(!resourceInfo.isExternal && resourceInfo.concreteHandle.isValid() && resourceInfo.concreteHandle.type == ResourceType::Texture)
    {
      internal::NameID id = static_cast<internal::NameID>(i + 1);
      std::string_view resourceName = GetNameString(id);
      uint32_t bindlessIndex = resourceInfo.concreteHandle.texture.index();
      bindlessMap[resourceName] = bindlessIndex;
    }
  }

  for(auto* observer : m_transientObservers)
  {
    observer->OnTransientResourcesUpdated(bindlessMap);
  }
}

bool RenderGraph::AreGraphicsDeclarationsEqual(const GraphicsPassDeclaration& decl1, const GraphicsPassDeclaration& decl2)
{
  // Fast comparison with memcmp since we now use NameIDs
  return std::memcmp(&decl1, &decl2, sizeof(GraphicsPassDeclaration)) == 0;
}

void RenderGraph::FillVkRenderPassFromDeclaration(const GraphicsPassDeclaration& graphDecl, vk::RenderPass& vkRp)
{
  vkRp = {};

  // Unroll loop for better performance
  for(unsigned i = 0; i < vk::MAX_COLOR_ATTACHMENTS; ++i)
  {
    const auto& graphAtt = graphDecl.colorAttachments[i];

    if(graphAtt.IsActive())
    {
      vkRp.color[i].loadOp = graphAtt.loadOp;
      vkRp.color[i].storeOp = graphAtt.storeOp;
      vkRp.color[i].layer = graphAtt.layer;
      vkRp.color[i].level = graphAtt.level;
      std::memcpy(&vkRp.color[i].clearColor, &graphAtt.clearColor, sizeof(vkRp.color[i].clearColor));
    }
    else
    {
      vkRp.color[i].loadOp = vk::LoadOp::Invalid;
    }
  }

  if(graphDecl.depthAttachment.IsActive())
  {
    vkRp.depth.loadOp = graphDecl.depthAttachment.loadOp;
    vkRp.depth.storeOp = graphDecl.depthAttachment.storeOp;
    vkRp.depth.layer = graphDecl.depthAttachment.layer;
    vkRp.depth.level = graphDecl.depthAttachment.level;
    vkRp.depth.clearDepth = graphDecl.depthAttachment.clearDepth;
    vkRp.depth.clearStencil = graphDecl.depthAttachment.clearStencil;
  }
  else
  {
    vkRp.depth.loadOp = vk::LoadOp::Invalid;
  }

  if(graphDecl.stencilAttachment.IsActive())
  {
    if(!graphDecl.depthAttachment.IsActive() || graphDecl.depthAttachment.textureNameID != graphDecl.stencilAttachment.textureNameID)
    {
      vkRp.stencil.loadOp = graphDecl.stencilAttachment.loadOp;
      vkRp.stencil.storeOp = graphDecl.stencilAttachment.storeOp;
      vkRp.stencil.layer = graphDecl.stencilAttachment.layer;
      vkRp.stencil.level = graphDecl.stencilAttachment.level;
      vkRp.stencil.clearStencil = graphDecl.stencilAttachment.clearStencil;
    }
    else
    {
      vkRp.depth.clearStencil = graphDecl.stencilAttachment.clearStencil;
    }
  }
  else
  {
    vkRp.stencil.loadOp = vk::LoadOp::Invalid;
  }
}

void RenderGraph::FillVkFramebufferFromDeclaration(const GraphicsPassDeclaration& graphDecl, vk::Framebuffer& vkFb)
{
  vkFb = {};
  vkFb.debugName = graphDecl.framebufferDebugName;

  auto getTextureHandle = [this](internal::NameID nameID) -> vk::TextureHandle
  {
    if(nameID == internal::INVALID_NAME_ID)
    {
      LOG_ERROR("GetTexture: FAILED. No valid handle found");
      return vk::TextureHandle{};
    }

    if(const auto* info = GetResolvedResource(nameID))
    {
      if(info->concreteHandle.type == ResourceType::Texture)
      {
        return info->concreteHandle.texture;
      }
    }
    LOG_ERROR("GetTexture: FAILED. No valid handle found");
    return vk::TextureHandle{};
  };

  // Fill color attachments
  for(unsigned i = 0; i < vk::MAX_COLOR_ATTACHMENTS; ++i)
  {
    const auto& graphAtt = graphDecl.colorAttachments[i];
    if(graphAtt.IsActive())
    {
      vkFb.color[i].texture = getTextureHandle(graphAtt.textureNameID);

      if(graphAtt.resolveTextureNameID != internal::INVALID_NAME_ID)
      {
        vkFb.color[i].resolveTexture = getTextureHandle(graphAtt.resolveTextureNameID);
      }
    }
  }

  // Fill depth/stencil attachments
  if(graphDecl.depthAttachment.IsActive())
  {
    vkFb.depthStencil.texture = getTextureHandle(graphDecl.depthAttachment.textureNameID);

    if(graphDecl.depthAttachment.resolveTextureNameID != internal::INVALID_NAME_ID)
    {
      vkFb.depthStencil.resolveTexture = getTextureHandle(graphDecl.depthAttachment.resolveTextureNameID);
    }
  }

  if(graphDecl.stencilAttachment.IsActive())
  {
    if(!graphDecl.depthAttachment.IsActive() || graphDecl.depthAttachment.textureNameID != graphDecl.stencilAttachment.textureNameID)
    {
      vkFb.depthStencil.texture = getTextureHandle(graphDecl.stencilAttachment.textureNameID);

      if(graphDecl.stencilAttachment.resolveTextureNameID != internal::INVALID_NAME_ID)
      {
        vkFb.depthStencil.resolveTexture = getTextureHandle(graphDecl.stencilAttachment.resolveTextureNameID);
      }
    }
  }
}

void RenderGraph::AggregateDependenciesForPass(size_t passIndex, std::vector<vk::TextureHandle>& textures, std::vector<vk::BufferHandle>& buffers) const
{
  const auto& execData = m_passExecData[passIndex];
  const auto& metadata = m_passMetadata[passIndex];

  // Helper to check if resource is an output attachment
  auto isOutputAttachment = [&](internal::NameID resourceId) -> bool
  {
    if(execData.passType != 0)
      return false; // Not graphics

    const auto& gd = metadata.graphicsDeclaration;

    // Check all attachments
    for(const auto& att : gd.colorAttachments)
    {
      if(!att.IsActive())
        continue;
      if(att.textureNameID == resourceId || att.resolveTextureNameID == resourceId)
      {
        return true;
      }
    }

    if(gd.depthAttachment.IsActive())
    {
      if(gd.depthAttachment.textureNameID == resourceId || gd.depthAttachment.resolveTextureNameID == resourceId)
      {
        return true;
      }
    }

    if(gd.stencilAttachment.IsActive())
    {
      if(gd.stencilAttachment.textureNameID == resourceId || gd.stencilAttachment.resolveTextureNameID == resourceId)
      {
        return true;
      }
    }

    return false;
  };

  // Process resource accesses
  for(size_t i = 0; i < execData.handleCount; ++i)
  {
    const auto& usage = metadata.declaredResourceAccesses[i];

    if(usage.id == internal::INVALID_NAME_ID || usage.id == m_swapchainID)
      continue;

    // Only include resources that are read by shaders
    if(usage.accessType == AccessType::Read || usage.accessType == AccessType::ReadWrite)
    {
      if(isOutputAttachment(usage.id))
        continue;

      const ResourceHandle& handle = m_handleStorage[execData.handleOffset + i];

      if(handle.type == ResourceType::Texture && handle.texture.valid())
      {
        textures.push_back(handle.texture);
      }
      else if(handle.type == ResourceType::Buffer && handle.buffer.valid())
      {
        buffers.push_back(handle.buffer);
      }
    }
  }
}

// ResourceProperties implementation
ResourceProperties ResourceProperties::FromDesc(const vk::TextureDesc& textureDesc, bool isPersistent)
{
  return { .desc = textureDesc, .type = ResourceType::Texture, .persistent = isPersistent };
}

ResourceProperties ResourceProperties::FromDesc(const vk::BufferDesc& bufferDesc, bool isPersistent)
{
  return { .desc = bufferDesc, .type = ResourceType::Buffer, .persistent = isPersistent };
}

// GraphAttachmentDescription implementation
bool GraphAttachmentDescription::operator==(const GraphAttachmentDescription& other) const
{
  // Now using NameIDs, comparison is much faster
  return textureNameID == other.textureNameID && resolveTextureNameID == other.resolveTextureNameID && loadOp == other.loadOp && storeOp == other.storeOp && layer == other.layer && level == other.level && std::memcmp(&clearColor, &other.clearColor, sizeof(clearColor)) == 0 && clearDepth == other.clearDepth && clearStencil == other.clearStencil;
}

// Internal namespace implementations
namespace internal
{
  void FastResourceCache::Insert(uint32_t hash, uint8_t index)
  {
    if(count < CACHE_SIZE)
    {
      entries[count++] = { hash, index };
    }
    else
    {
      // Simple replacement strategy - replace oldest
      entries[0] = { hash, index };
    }
  }

  uint8_t FastResourceCache::Find(uint32_t hash) const
  {
    // Unrolled loop for small cache
    for(uint8_t i = 0; i < count; ++i)
    {
      if(entries[i].nameHash == hash)
        return entries[i].index;
    }
    return 0xFF;
  }

  void FastResourceCache::Clear()
  {
    count = 0;
  }

  uint16_t FrameBufferManager::AllocateEntry(NameID id)
  {
    if(m_nextIndex >= MAX_RESOURCES)
    {
      return MAX_RESOURCES; // Return invalid index
    }

    uint16_t idx = m_nextIndex++;
    if(id < MAX_NAME_IDS)
    {
      m_nameToIndex[id] = idx;
    }
    return idx;
  }

  FrameBufferManager::Entry* FrameBufferManager::GetEntry(NameID id)
  {
    if(id < MAX_NAME_IDS)
    {
      uint16_t idx = m_nameToIndex[id];
      if(idx < MAX_RESOURCES && m_resources[idx].active)
      {
        return &m_resources[idx];
      }
    }
    LOG_ERROR("FBM::GetBuffer: -> FAILED");
    return nullptr;
  }

  vk::BufferHandle FrameBufferManager::GetBuffer(NameID id, uint32_t frameIdx) const
  {
    if(id < MAX_NAME_IDS)
    {
      uint16_t idx = m_nameToIndex[id];
      if(idx < MAX_RESOURCES && m_resources[idx].active)
      {
        const auto& handle = m_resources[idx].buffers[frameIdx];
        return handle;
      }
    }
    LOG_ERROR("FBM::GetBuffer: -> FAILED. Resource ID {} is out of bounds for lookup.", id);
    return vk::BufferHandle{};
  }

  ExecutionContext::ExecutionContext(vk::ICommandBuffer& commandBuffer, const FrameData& frameData, RenderGraph* graph, size_t passIndex) : m_vkCommandBuffer(commandBuffer), m_frameData(frameData), m_pGraph(graph), m_passIndex(passIndex)
  {
  }

  uint32_t ExecutionContext::HashString(const char* str)
  {
    uint32_t hash = 2166136261u;
    while(*str)
    {
      hash ^= static_cast<uint32_t>(*str++);
      hash *= 16777619u;
    }
    return hash;
  }

  vk::TextureHandle ExecutionContext::GetTexture(LogicalResourceName name) const
  {
    ResourceHandle handle = GetResourceCached(name);
    if(handle.isValid() && handle.type == ResourceType::Texture)
    {
      return handle.texture;
    }
    LOG_ERROR("GetTexture: FAILED. No valid texture handle found for '{}'.", name);
    return vk::TextureHandle{};
  }

  vk::BufferHandle ExecutionContext::GetBuffer(LogicalResourceName name) const
  {
    NameID id = m_pGraph->FindNameID(name);
    if(id == INVALID_NAME_ID)
    {
      LOG_ERROR("GetBuffer: FAILED. Name '{}' not found in registry.", name);
      return vk::BufferHandle{};
    }
    const auto* info = m_pGraph->GetResolvedResource(id);
    if(!info || info->definition.type != ResourceType::Buffer)
    {
      LOG_ERROR("GetBuffer: FAILED. No valid buffer resource info found for ID {}.", id);
      return vk::BufferHandle{};
    }

    if(!info->isExternal && !info->definition.persistent)
    {
      uint32_t frameIdx = m_pGraph->GetCurrentFrameIndex();
      return m_pGraph->m_frameBuffers.GetBuffer(id, frameIdx);
    }
    else
    {
      ResourceHandle resHandle = GetResourceCached(name);
      if(resHandle.isValid() && resHandle.type == ResourceType::Buffer)
      {
        return resHandle.buffer;
      }
    }

    LOG_ERROR("GetBuffer: FAILED. Lookup for '{}' (ID: {}) fell through without returning a valid handle.", name, id);
    return vk::BufferHandle{};
  }

  void ExecutionContext::ResizeBuffer(LogicalResourceName name, uint64_t newSize) const
  {
    NameID id = m_pGraph->FindNameID(name);
    if(id == INVALID_NAME_ID)
      return;

    if(auto* entry = m_pGraph->m_frameBuffers.GetEntry(id))
    {
      m_pGraph->EnsureBufferCapacity(*entry, newSize);
    }
  }

  uint64_t ExecutionContext::GetBufferSize(LogicalResourceName name) const
  {
      NameID id = m_pGraph->FindNameID(name);
      if (id == INVALID_NAME_ID)
          return 0;

      const auto* info = m_pGraph->GetResolvedResource(id);
      if (!info)
          return 0;

      if (info->definition.type == ResourceType::Buffer &&
          (info->isExternal || info->definition.persistent))
      {
      	const auto desc = info->definition.desc;
          return std::get<vk::BufferDesc>(desc).size;
      }

      if (auto* entry = m_pGraph->m_frameBuffers.GetEntry(id))
      {
          return entry->currentSize;
      }

      return 0;
  }

  vk::TextureHandle ExecutionContext::GetTextureByIndex(size_t index) const
  {
    const auto& execData = m_pGraph->m_passExecData[m_passIndex];
    if(index >= execData.handleCount)
    {
      LOG_ERROR("GetTexture: FAILED. No valid handle found at index {}", index);
      return vk::TextureHandle{};
    }

    const ResourceHandle& handle = m_pGraph->m_handleStorage[execData.handleOffset + index];
    return (handle.type == ResourceType::Texture) ? handle.texture : vk::TextureHandle{};
  }

  vk::BufferHandle ExecutionContext::GetBufferByIndex(size_t index) const
  {
    const auto& execData = m_pGraph->m_passExecData[m_passIndex];
    if(index >= execData.handleCount)
    {
      LOG_ERROR("GetTexture: FAILED. No valid handle found at index {}", index);
      return vk::BufferHandle{};
    }
    const ResourceHandle& handle = m_pGraph->m_handleStorage[execData.handleOffset + index];
    return (handle.type == ResourceType::Buffer) ? handle.buffer : vk::BufferHandle{};
  }

  vk::IContext& ExecutionContext::GetvkContext() const
  {
    return m_pGraph->m_vkContext;
  }

  ResourceHandle ExecutionContext::GetResourceCached(LogicalResourceName name) const
  {
    // Special handling for swapchain (changes per frame)
    if(name == RenderResources::SWAPCHAIN_IMAGE || std::strcmp(name, RenderResources::SWAPCHAIN_IMAGE) == 0)
    {
      NameID id = m_pGraph->FindNameID(name);
      if(id != INVALID_NAME_ID)
      {
        return m_pGraph->GetResolvedHandle(id);
      }
      return ResourceHandle{};
    }
    uint32_t hash = HashString(name);
    uint8_t cachedIdx = m_cache.Find(hash);
    if(cachedIdx != 0xFF)
    {
      const auto& execData = m_pGraph->m_passExecData[m_passIndex];
      return m_pGraph->m_handleStorage[execData.handleOffset + cachedIdx];
    }
    NameID id = m_pGraph->FindNameID(name);
    if(id == INVALID_NAME_ID)
      return ResourceHandle{};

    const auto& execData = m_pGraph->m_passExecData[m_passIndex];
    const auto& metadata = m_pGraph->m_passMetadata[m_passIndex];

    for(uint8_t i = 0; i < execData.handleCount; ++i)
    {
      if(metadata.declaredResourceAccesses[i].id == id)
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

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::DeclareTransientResource(LogicalResourceName name, const vk::TextureDesc& desc)
  {
    m_parent->m_pRenderGraph->RegisterTransientResource(name, desc);
    return *this;
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::DeclareTransientResource(LogicalResourceName name, const vk::BufferDesc& desc)
  {
    m_parent->m_pRenderGraph->RegisterTransientResource(name, desc);
    return *this;
  }

  RenderPassBuilder::PassBuilder& RenderPassBuilder::PassBuilder::UseResource(LogicalResourceName name, AccessType accessType)
  {
    NameID id = m_parent->m_pRenderGraph->InternName(name);
    m_resourceDeclarations.push_back({ id, accessType });
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
    if(targetId != INVALID_NAME_ID)
    {
      m_executeAfterTargets.push_back(targetId);
    }
    return *this;
  }

  RenderPassBuilder& RenderPassBuilder::PassBuilder::AddGraphicsPass(const char* passName, const PassDeclarationInfo& declaration, ExecuteLambda&& executeLambda)
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

  RenderPassBuilder::RenderPassBuilder(RenderGraph* graph, IRenderFeature* feature) : m_pRenderGraph(graph), m_pCurrentFeature(feature)
  {
  }

  void RenderPassBuilder::SubmitGraphicsPass(PassBuilder& builder, const char* passName, const PassDeclarationInfo& declaration, ExecuteLambda&& executeLambda)
  {
    NameID passID = m_pRenderGraph->InternName(passName);
    GraphicsPassDeclaration declWithIDs{};
    auto convertAttachment = [&](const AttachmentDesc& publicDesc, GraphAttachmentDescription& internalDesc)
    {
      if(publicDesc.textureName)
      {
        internalDesc.textureNameID = m_pRenderGraph->InternName(publicDesc.textureName);
      }
      if(publicDesc.resolveTextureName)
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
    for(size_t i = 0; i < vk::MAX_COLOR_ATTACHMENTS; ++i)
    {
      convertAttachment(declaration.colorAttachments[i], declWithIDs.colorAttachments[i]);
    }
    convertAttachment(declaration.depthAttachment, declWithIDs.depthAttachment);
    convertAttachment(declaration.stencilAttachment, declWithIDs.stencilAttachment);
    if(declaration.framebufferDebugName)
    {
      declWithIDs.framebufferDebugName = declaration.framebufferDebugName;
    }
    PassExecutionData execData{ .passID = passID,
      .passType = 0,
      // Graphics
      .handleCount = static_cast<uint8_t>(builder.m_resourceDeclarations.size()),
      .handleOffset = 0,
      .executeLambda = nullptr };
    PassMetadata metadata{ .debugName = passName, .declaredResourceAccesses = std::move(builder.m_resourceDeclarations), .priority = builder.m_priority, .executeAfterTargets = std::move(builder.m_executeAfterTargets), .graphicsDeclaration = declWithIDs, .featureOwner = m_pCurrentFeature };

    m_pRenderGraph->RegisterPass(std::move(execData), std::move(metadata), std::move(executeLambda));
  }

  void RenderPassBuilder::SubmitGenericPass(PassBuilder& builder, const char* passName, ExecuteLambda&& executeLambda)
  {
    NameID passID = m_pRenderGraph->InternName(passName);

    PassExecutionData execData{ .passID = passID,
      .passType = 1,
      // Generic
      .handleCount = static_cast<uint8_t>(builder.m_resourceDeclarations.size()),
      .handleOffset = 0,
      .executeLambda = nullptr };

    PassMetadata metadata{ .debugName = passName, .declaredResourceAccesses = std::move(builder.m_resourceDeclarations), .priority = builder.m_priority, .executeAfterTargets = std::move(builder.m_executeAfterTargets), .featureOwner = m_pCurrentFeature };

    m_pRenderGraph->RegisterPass(std::move(execData), std::move(metadata), std::move(executeLambda));
  }
} // namespace internal

RenderGraph::RenderGraph(vk::IContext& vkContext, GPUBuffers& gpu_buffers) : m_vkContext(vkContext), m_gpu_buffers(gpu_buffers), oit(vkContext), linearColorSystem(vkContext)
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
}

RenderGraph::~RenderGraph() = default;

void RenderGraph::BuildAdjacencyListAndSort()
{
  size_t numPasses = m_passExecData.size();
  if(numPasses == 0)
  {
    m_compiledPassOrder.clear();
    return;
  }

  // Build pass ID to index map
  std::unordered_map<internal::NameID, size_t> passIdToIndex;
  passIdToIndex.reserve(numPasses);
  for(size_t i = 0; i < numPasses; ++i)
  {
    passIdToIndex[m_passExecData[i].passID] = i;
  }

  // Build adjacency list
  std::vector<std::vector<size_t>> adj(numPasses);
  std::vector<int> inDegree(numPasses, 0);

  // Sort passes by priority first
  std::vector<size_t> sortedIndices(numPasses);
  std::iota(sortedIndices.begin(), sortedIndices.end(), 0);

  std::sort(sortedIndices.begin(),
            sortedIndices.end(),
            [this](size_t a, size_t b)
  {
    return static_cast<int>(m_passMetadata[a].priority) < static_cast<int>(m_passMetadata[b].priority);
  });

  // Track resource dependencies
  std::unordered_map<internal::NameID, size_t> lastWriter;
  std::unordered_map<internal::NameID, std::vector<size_t>> readersSinceLastWrite;

  for(size_t idx : sortedIndices)
  {
    const auto& metadata = m_passMetadata[idx];

    // Handle explicit ExecuteAfter dependencies
    for(internal::NameID targetId : metadata.executeAfterTargets)
    {
      if(auto it = passIdToIndex.find(targetId); it != passIdToIndex.end())
      {
        adj[it->second].push_back(idx);
        inDegree[idx]++;
      }
    }

    // Handle resource dependencies
    for(const auto& usage : metadata.declaredResourceAccesses)
    {
      internal::NameID resId = usage.id;

      // Read dependencies
      if(usage.accessType == AccessType::Read || usage.accessType == AccessType::ReadWrite)
      {
        if(auto writerIt = lastWriter.find(resId); writerIt != lastWriter.end())
        {
          adj[writerIt->second].push_back(idx);
          inDegree[idx]++;
        }

        if(usage.accessType == AccessType::Read)
        {
          readersSinceLastWrite[resId].push_back(idx);
        }
      }

      // Write dependencies
      if(usage.accessType == AccessType::Write || usage.accessType == AccessType::ReadWrite)
      {
        // All readers must complete before this write
        if(auto readersIt = readersSinceLastWrite.find(resId); readersIt != readersSinceLastWrite.end())
        {
          for(size_t readerIdx : readersIt->second)
          {
            adj[readerIdx].push_back(idx);
            inDegree[idx]++;
          }
          readersIt->second.clear();
        }

        // FIX: Add Write-After-Write (WAW) dependency
        if(auto writerIt = lastWriter.find(resId); writerIt != lastWriter.end())
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
  for(size_t i = 0; i < numPasses; ++i)
  {
    if(inDegree[i] == 0)
      q.push(i);
  }

  m_compiledPassOrder.clear();
  m_compiledPassOrder.reserve(numPasses);

  while(!q.empty())
  {
    size_t u = q.front();
    q.pop();
    m_compiledPassOrder.push_back(u);

    for(size_t v : adj[u])
    {
      if(--inDegree[v] == 0)
        q.push(v);
    }
  }

  // Check for cycles
  if(m_compiledPassOrder.size() != numPasses)
  {
    m_compiledPassOrder.clear();
  }
}

bool RenderGraph::canCompile() const
{
  // Check if context has swapchain
  if (!m_vkContext.hasSwapchain())
  {
    return false;
  }

  // Verify we can get valid swapchain texture
  vk::TextureHandle swapchainTex = m_vkContext.getCurrentSwapchainTexture();
  if (!swapchainTex.valid())
  {
    return false;
  }

  // Verify dimensions are valid
  vk::Dimensions dims = m_vkContext.getDimensions(swapchainTex);
  if (dims.width == 0 || dims.height == 0)
  {
    return false;
  }

  return true;
}

bool RenderGraph::hasMinimumResources() const
{
  vk::BufferHandle materialBuffer = m_gpu_buffers.GetMaterialBuffer();
  vk::BufferHandle vertexBuffer = m_gpu_buffers.GetVertexBuffer();
  vk::BufferHandle indexBuffer = m_gpu_buffers.GetIndexBuffer();

  return materialBuffer.valid() && vertexBuffer.valid() && indexBuffer.valid();
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

  // Validate swapchain availability
  if (!m_vkContext.hasSwapchain())
  {
    LOG_WARNING("Cannot compile RenderGraph - no swapchain");
    m_isCompiled = false;
    return;
  }

  vk::TextureHandle initialSwapchainTexture = m_vkContext.getCurrentSwapchainTexture();
  if (!initialSwapchainTexture.valid())
  {
    LOG_WARNING("Cannot compile RenderGraph - invalid swapchain texture");
    m_isCompiled = false;
    return;
  }

  vk::Dimensions swapchainDims = m_vkContext.getDimensions(initialSwapchainTexture);
  if (swapchainDims.width == 0 || swapchainDims.height == 0)
  {
    LOG_WARNING("Cannot compile RenderGraph - invalid dimensions");
    m_isCompiled = false;
    return;
  }
  m_lastCompileDimensions = swapchainDims;

  vk::TextureDesc swapchainDesc{ .type = vk::TextureType::Tex2D, .format = m_vkContext.getSwapchainFormat(), .dimensions = swapchainDims, .usage = vk::TextureUsageBits_Attachment };

  // Register external resources
  ImportExternalTexture(RenderResources::SWAPCHAIN_IMAGE, initialSwapchainTexture, swapchainDesc);
  ImportExternalBuffer(RenderResources::MATERIAL_BUFFER, m_gpu_buffers.GetMaterialBuffer(), m_gpu_buffers.GetMaterialBufferDesc());
  ImportExternalBuffer(RenderResources::VERTEX_BUFFER, m_gpu_buffers.GetVertexBuffer(), m_gpu_buffers.GetVertexBufferDesc());
  ImportExternalBuffer(RenderResources::INDEX_BUFFER, m_gpu_buffers.GetIndexBuffer(), m_gpu_buffers.GetIndexBufferDesc());
  ImportExternalBuffer(RenderResources::MESH_DECOMPRESSION_BUFFER, m_gpu_buffers.GetMeshDecompressionBuffer(), m_gpu_buffers.GetMeshDecompressionBufferDesc());
  ImportExternalBuffer(RenderResources::SKINNING_BUFFER, m_gpu_buffers.GetSkinningBuffer(), m_gpu_buffers.GetSkinningBufferDesc());
  ImportExternalBuffer(RenderResources::MORPH_DELTA_BUFFER, m_gpu_buffers.GetMorphDeltaBuffer(), m_gpu_buffers.GetMorphDeltaBufferDesc());
  ImportExternalBuffer(RenderResources::MORPH_VERTEX_BASE_BUFFER, m_gpu_buffers.GetMorphVertexBaseBuffer(), m_gpu_buffers.GetMorphVertexBaseBufferDesc());
  ImportExternalBuffer(RenderResources::MORPH_VERTEX_COUNT_BUFFER, m_gpu_buffers.GetMorphVertexCountBuffer(), m_gpu_buffers.GetMorphVertexCountBufferDesc());

  // Register standard render targets
  linearColorSystem.RegisterLinearColorResources(*this);
  RegisterTransientResource(RenderResources::SCENE_DEPTH, vk::TextureDesc{ .type = vk::TextureType::Tex2D, .format = vk::Format::Z_F32, .dimensions = ResourceProperties::SWAPCHAIN_RELATIVE_DIMENSIONS, .usage = vk::TextureUsageBits_Attachment });

  for(IRenderFeature* feature : m_features)
  {
    internal::RenderPassBuilder builder(this, feature);
    feature->SetupPasses(builder);
  }
  {
    internal::RenderPassBuilder builder(this, nullptr);
    linearColorSystem.RegisterToneMappingPass(builder);
  }
  {
    internal::RenderPassBuilder builder(this, nullptr);
    oit.SetupPasses(builder);
  }

  if(m_passExecData.empty())
  {
    m_isCompiled = false;
    return;
  }


  PassExecutionData finalBlitExec{ .passID = InternName("FinalBlit"),
    .passType = 1,
    // Generic
    .handleCount = 2,
    .handleOffset = 0,
    .executeLambda = nullptr };
  PassMetadata finalBlitMeta{ .debugName = "FinalBlit", .declaredResourceAccesses = {{InternName(RenderResources::SCENE_COLOR), AccessType::Read}, {InternName(RenderResources::SWAPCHAIN_IMAGE), AccessType::Write}}, .priority = internal::RenderPassBuilder::PassPriority::Present, .featureOwner = nullptr };
  ExecuteLambda finalBlitLambda = [this](internal::ExecutionContext& ctx)
  {
    ExecuteFinalBlit(ctx);
  };
  RegisterPass(std::move(finalBlitExec), std::move(finalBlitMeta), std::move(finalBlitLambda));


  // Sort passes
  BuildAdjacencyListAndSort();

  if(m_compiledPassOrder.empty())
  {
    m_isCompiled = false;
    return;
  }
  for(size_t passIdx : m_compiledPassOrder)
  {
    auto& execData = m_passExecData[passIdx];
    const auto& metadata = m_passMetadata[passIdx];

    execData.handleOffset = static_cast<uint16_t>(m_handleStorage.size());
    execData.handleCount = static_cast<uint8_t>(metadata.declaredResourceAccesses.size());

    for(const auto& usage : metadata.declaredResourceAccesses)
    {
      if(auto* info = GetResolvedResource(usage.id))
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

  for(size_t i = 0; i < m_resolvedResourceInfo.size(); ++i)
  {
    auto& info = m_resolvedResourceInfo[i];
    internal::NameID id = static_cast<internal::NameID>(i + 1);
    if(info.isExternal || info.firstUsePassIndex == ~0u)
    {
      continue;
    }

    if(info.definition.type == ResourceType::Texture)
    {
      vk::TextureDesc actualDesc = std::get<vk::TextureDesc>(info.definition.desc);

      if(actualDesc.dimensions == ResourceProperties::SWAPCHAIN_RELATIVE_DIMENSIONS)
        actualDesc.dimensions = m_lastCompileDimensions;
      if(actualDesc.format == vk::Format::Invalid)
        actualDesc.format = m_vkContext.getSwapchainFormat();
      m_storedTextures.emplace_back(m_vkContext.createTexture(actualDesc));
      info.concreteHandle = ResourceHandle(m_storedTextures.back());
    }
    else if(info.definition.type == ResourceType::Buffer)
    {
      const vk::BufferDesc& desc = std::get<vk::BufferDesc>(info.definition.desc);

      uint16_t entryIdx = m_frameBuffers.AllocateEntry(id);
      if(entryIdx >= internal::MAX_RESOURCES)
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
  for(size_t passIdx : m_compiledPassOrder)
  {
    const auto& execData = m_passExecData[passIdx];
    const auto& metadata = m_passMetadata[passIdx];

    for(size_t i = 0; i < execData.handleCount; ++i)
    {
      const auto& usage = metadata.declaredResourceAccesses[i];
      if(auto* info = GetResolvedResource(usage.id))
      {
        const bool isTransientBuffer = !info->isExternal && !info->definition.persistent && info->definition.type == ResourceType::Buffer;

        if(!isTransientBuffer)
          m_handleStorage[execData.handleOffset + i] = info->concreteHandle;
      }
    }
  }

  // Batch graphics passes and create execution steps
  m_executionSteps.reserve(m_compiledPassOrder.size());

  size_t i = 0;
  while(i < m_compiledPassOrder.size())
  {
    size_t passIdx = m_compiledPassOrder[i];
    const auto& execData = m_passExecData[passIdx];

    if(execData.passType == 1)
    {
      // Generic
      m_executionSteps.emplace_back(passIdx);
      ++i;
      continue;
    }

    // Start graphics batch
    BatchedGraphicsPassExecution batch{};
    const auto& firstMeta = m_passMetadata[passIdx];
    FillVkRenderPassFromDeclaration(firstMeta.graphicsDeclaration, batch.renderPassInfo);

    std::vector<vk::TextureHandle> textures;
    std::vector<vk::BufferHandle> buffers;

    batch.passIndices[0] = passIdx;
    batch.passCount = 1;
    AggregateDependenciesForPass(passIdx, textures, buffers);

    // Try to batch compatible passes
    size_t j = i + 1;
    while(j < m_compiledPassOrder.size() && batch.passCount < 8)
    {
      size_t nextIdx = m_compiledPassOrder[j];
      const auto& nextExecData = m_passExecData[nextIdx];
      const auto& nextMeta = m_passMetadata[nextIdx];

      if(nextExecData.passType == 0 && // Graphics
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
    for(const auto& tex : textures)
    {
      if(texIdx < vk::Dependencies::MAX_SUBMIT_DEPENDENCIES)
      {
        batch.accumulatedDependencies.textures[texIdx++] = tex;
      }
    }

    int bufIdx = 0;
    for(const auto& buf : buffers)
    {
      if(bufIdx < vk::Dependencies::MAX_SUBMIT_DEPENDENCIES)
      {
        batch.accumulatedDependencies.buffers[bufIdx++] = buf;
      }
    }

    m_executionSteps.emplace_back(std::move(batch));
    i = j;
  }

  NotifyTransientObservers();
  m_isCompiled = true;
}

void RenderGraph::RegisterTransientResource(LogicalResourceName name, const ResourceProperties& properties)
{
  if(!name || !name[0])
    return;

  internal::NameID id = InternName(name);
  if(id == internal::INVALID_NAME_ID)
    return;

  if(auto* info = GetResolvedResource(id))
  {
    *info = ResolvedResourceInfo{ .id = id, .concreteHandle = ResourceHandle{}, .definition = properties, .isExternal = false, .firstUsePassIndex = ~0u, .lastUsePassIndex = 0 };
  }
}

void RenderGraph::RegisterTransientResource(LogicalResourceName name, const vk::TextureDesc& desc)
{
  RegisterTransientResource(name, ResourceProperties::FromDesc(desc, false));
}

void RenderGraph::RegisterTransientResource(LogicalResourceName name, const vk::BufferDesc& desc)
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

void RenderGraph::ExecuteFinalBlit(const internal::ExecutionContext& ctx)
{
  vk::TextureHandle sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  vk::TextureHandle swapchainImage = ctx.GetTexture(RenderResources::SWAPCHAIN_IMAGE);

  if(!sceneColor.valid() || !swapchainImage.valid())
    return;

  auto& cmd = ctx.GetvkCommandBuffer();

  vk::Dimensions sceneDims = m_vkContext.getDimensions(sceneColor);
  vk::Dimensions swapchainDims = m_vkContext.getDimensions(swapchainImage);

  vk::Dimensions copyExtent = { .width = std::min(sceneDims.width, swapchainDims.width), .height = std::min(sceneDims.height, swapchainDims.height), .depth = 1 };

  cmd.cmdCopyImage(sceneColor, swapchainImage, copyExtent, vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ 0, 0, 0 }, vk::TextureLayers{ 0, 0, 1 }, vk::TextureLayers{ 0, 0, 1 });
}

ResourceHandle RenderGraph::GetResolvedHandle(internal::NameID id) const
{
  if(const auto* info = GetResolvedResource(id))
  {
    return info->concreteHandle;
  }
  return ResourceHandle{};
}

void RenderGraph::AddFeature(IRenderFeature* feature)
{
  if(feature)
  {
    m_features.push_back(feature);
    Invalidate();
  }
}

void RenderGraph::RemoveFeature(IRenderFeature* feature)
{
  if(!feature)
    return;

  if(auto it = std::find(m_features.begin(), m_features.end(), feature); it != m_features.end())
  {
    m_features.erase(it);
    Invalidate();
  }
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

  Invalidate();
}

internal::NameID RenderGraph::InternName(LogicalResourceName name)
{
  if(!name)
    return internal::INVALID_NAME_ID;

  std::string_view sv(name);

  // Binary search in sorted vector for better cache locality
  const auto it = std::lower_bound(m_nameRegistry.begin(),
                                   m_nameRegistry.end(),
                                   sv,
                                   [](const auto& pair, std::string_view val)
  {
    return pair.first < val;
  });

  if(it != m_nameRegistry.end() && it->first == sv)
  {
    return it->second;
  }

  // Store string in deque for stable pointers
  m_stringStorage.emplace_back(name);
  std::string_view stored = m_stringStorage.back();
  // -----------------------------------------------------------

  internal::NameID newID = m_nextNameID++;

  m_nameRegistry.insert(it, { stored, newID });

  if(newID > m_idToStringMap.size())
  {
    m_idToStringMap.resize(newID);
  }
  m_idToStringMap[newID - 1] = stored;

  return newID;
}

internal::NameID RenderGraph::FindNameID(LogicalResourceName name) const
{
  if(!name)
    return internal::INVALID_NAME_ID;

  std::string_view sv(name);

  // Binary search in sorted vector
  auto it = std::lower_bound(m_nameRegistry.begin(),
                             m_nameRegistry.end(),
                             sv,
                             [](const auto& pair, std::string_view val)
  {
    return pair.first < val;
  });

  if(it != m_nameRegistry.end() && it->first == sv)
  {
    return it->second;
  }

  return internal::INVALID_NAME_ID;
}

LogicalResourceName RenderGraph::GetNameString(internal::NameID id) const
{
  if(id == internal::INVALID_NAME_ID || id == 0 || id > m_idToStringMap.size())
  {
    return "[Invalid]";
  }
  return m_idToStringMap[id - 1].data();
}

RenderGraph::ResolvedResourceInfo* RenderGraph::GetResolvedResource(internal::NameID id)
{
  if(id == internal::INVALID_NAME_ID || id == 0)
    return nullptr;

  size_t idx = id - 1;
  if(idx >= m_resolvedResourceInfo.size())
  {
    m_resolvedResourceInfo.resize(id);
  }
  return &m_resolvedResourceInfo[idx];
}

const RenderGraph::ResolvedResourceInfo* RenderGraph::GetResolvedResource(internal::NameID id) const
{
  if(id == internal::INVALID_NAME_ID || id == 0 || id > m_resolvedResourceInfo.size())
  {
    return nullptr;
  }
  return &m_resolvedResourceInfo[id - 1];
}

internal::NameID RenderGraph::ImportExternalTexture(LogicalResourceName name, vk::TextureHandle textureHandle, const vk::TextureDesc& desc)
{
  internal::NameID id = InternName(name);
  if(id == internal::INVALID_NAME_ID)
    return id;

  if(auto* info = GetResolvedResource(id))
  {
    *info = ResolvedResourceInfo{ .concreteHandle = ResourceHandle(textureHandle), .definition = ResourceProperties::FromDesc(desc, true), .isExternal = true, .firstUsePassIndex = ~0u, .lastUsePassIndex = 0 };
  }

  return id;
}

internal::NameID RenderGraph::ImportExternalBuffer(LogicalResourceName name, vk::BufferHandle bufferHandle, const vk::BufferDesc& desc)
{
  internal::NameID id = InternName(name);
  if(id == internal::INVALID_NAME_ID)
    return id;

  if(auto* info = GetResolvedResource(id))
  {
    *info = ResolvedResourceInfo{ .concreteHandle = ResourceHandle(bufferHandle), .definition = ResourceProperties::FromDesc(desc, true), .isExternal = true, .firstUsePassIndex = ~0u, .lastUsePassIndex = 0 };
  }

  return id;
}

void RenderGraph::Execute(vk::ICommandBuffer& vkCommandBuffer, const FrameData& frameData)
{
  BeginFrame();

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

  // Check for swapchain resize
  if (m_isCompiled && m_vkContext.hasSwapchain())
  {
    vk::TextureHandle currentSwapchain = m_vkContext.getCurrentSwapchainTexture();
    if (currentSwapchain.valid())
    {
      vk::Dimensions currentDims = m_vkContext.getDimensions(currentSwapchain);

      if (currentDims.width != m_lastCompileDimensions.width ||
          currentDims.height != m_lastCompileDimensions.height)
      {
        Invalidate();
      }
    }
  }

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
  else
  {
    UpdateSwapchainResource();
  }

  if (!m_isCompiled || m_executionSteps.empty())
    return;

  // Execute passes
  for(const auto& step : m_executionSteps)
  {
    if(step.type == CompiledExecutionStep::Type::Generic)
    {
      size_t passIdx = step.genericPassIndex;
      const auto& execData = m_passExecData[passIdx];
      const auto& metadata = m_passMetadata[passIdx];

      if(metadata.debugName && metadata.debugName[0])
      {
        vkCommandBuffer.cmdPushDebugGroupLabel(metadata.debugName);
      }

      internal::ExecutionContext context(vkCommandBuffer, frameData, this, passIdx);
      (*execData.executeLambda)(context);

      if(metadata.debugName && metadata.debugName[0])
      {
        vkCommandBuffer.cmdPopDebugGroupLabel();
      }
    }
    else
    {
      const auto& batch = step.graphicsBatch;

      vk::Framebuffer framebuffer;
      FillVkFramebufferFromDeclaration(m_passMetadata[batch.passIndices[0]].graphicsDeclaration, framebuffer);

      vkCommandBuffer.cmdBeginRendering(batch.renderPassInfo, framebuffer, batch.accumulatedDependencies);

      for(uint8_t i = 0; i < batch.passCount; ++i)
      {
        size_t passIdx = batch.passIndices[i];
        const auto& execData = m_passExecData[passIdx];
        const auto& metadata = m_passMetadata[passIdx];

        if(metadata.debugName && metadata.debugName[0])
        {
          vkCommandBuffer.cmdPushDebugGroupLabel(metadata.debugName);
        }

        internal::ExecutionContext context(vkCommandBuffer, frameData, this, passIdx);
        (*execData.executeLambda)(context);

        if(metadata.debugName && metadata.debugName[0])
        {
          vkCommandBuffer.cmdPopDebugGroupLabel();
        }
      }

      vkCommandBuffer.cmdEndRendering();
    }
  }
}

void RenderGraph::UpdateSwapchainResource()
{
  vk::TextureHandle swapchainHandle = m_vkContext.getCurrentSwapchainTexture();
  if(auto* info = GetResolvedResource(m_swapchainID))
  {
    info->concreteHandle = ResourceHandle(swapchainHandle);
  }
  else
  {
    Invalidate();
  }
}
