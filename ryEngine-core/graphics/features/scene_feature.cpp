#include "scene_feature.h"
#include "graphics/renderer.h"
#include "graphics/shader.h"
#include "resource/resource_manager.h"

void SceneRenderFeature::UpdateScene(uint64_t renderFeatureID,
                                     const Resource::Scene& gameScene,
                                     const Resource::ResourceManager&
                                     asset_system,
                                     Renderer& renderer
)
{
  auto params = static_cast<SceneRenderParams*>(renderer.
                                                GetFeatureParameterBlockPtr(renderFeatureID));
  if(!params)
    return;

  params->clear();
  params->reserve(gameScene.objects.size());

  // Build all scene data in one pass
  for(size_t objectIndex = 0; objectIndex < gameScene.objects.size(); ++
      objectIndex)
  {
    const SceneObject& obj = gameScene.objects[objectIndex];

    // Validate handles
    if(!obj.mesh.isValid() || !obj.material.isValid())
      continue;

    const auto* meshData = asset_system.getMesh(obj.mesh);
    if(!meshData)
      continue;

    // Store object data
    params->objectTransforms.push_back(obj.transform);
    params->materialIndices.push_back(
      asset_system.getMaterialIndex(obj.material));

    // Transform mesh bounds to world space
    auto center = vec3(
      meshData->bounds.x,
      meshData->bounds.y,
      meshData->bounds.z);
    float radius = meshData->bounds.w * obj.maxScale;
    auto worldCenter = vec3(obj.transform * vec4(center, 1.0f));
    params->objectBounds.emplace_back(
      worldCenter - vec3(radius),
      worldCenter + vec3(radius));

    // Build draw command
    DrawIndexedIndirectCommand cmd = {
      .count = meshData->indexCount,
      .instanceCount = 1,
      .firstIndex = static_cast<uint32_t>(meshData->indexByteOffset / sizeof(
        uint32_t)),
      .baseVertex = static_cast<int32_t>(meshData->vertexByteOffset / sizeof(
        Vertex)),
      .baseInstance = static_cast<uint32_t>(objectIndex) };

    DrawData drawData = {
      .transformId = static_cast<uint32_t>(objectIndex),
      .materialId = asset_system.getMaterialIndex(obj.material) };

    uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.
                                                      size());
    params->drawCommands.push_back(cmd);
    params->drawData.push_back(drawData);

    if(asset_system.isMaterialTransparent(obj.material))
      params->transparentIndices.push_back(drawCommandIndex);
    else
      params->opaqueIndices.push_back(drawCommandIndex);
  }

  params->lights.clear();
  params->lights.reserve(gameScene.lights.size());

  for(const SceneLight& sceneLight : gameScene.lights)
  {
    // Skip disabled lights
    if(sceneLight.intensity <= 0.0f)
      continue;

    // Skip lights with zero/invalid color
    if(length(sceneLight.color) <= 0.0f)
      continue;

    Lighting::GPULight gpuLight;
    convertSceneLight(sceneLight, gpuLight);
    params->lights.push_back(gpuLight);
  }

  params->activeLightCount = static_cast<uint32_t>(params->lights.size());
  /*LOG_INFO("SceneRenderFeature: Updated {} objects ({} opaque, {} transparent)",
         params->getObjectCount(),
         params->getOpaqueCount(),
         params->getTransparentCount());*/
}

void SceneRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  constexpr size_t kInitialObjectCapacity = 1024;

  // Culling pass
  passBuilder.CreatePass().DeclareTransientResource(
    "ObjectTransforms",
    vk::BufferDesc{
      .usage = vk::BufferUsageBits_Storage,
      .storage = vk::StorageType::HostVisible,
      // Use initial capacity instead of MAX_OBJECTS
      .size = kInitialObjectCapacity * sizeof(mat4) }).UseResource(
        "ObjectTransforms",
        AccessType::Write).DeclareTransientResource(
          "ObjectBounds",
          vk::BufferDesc{
            .usage = vk::BufferUsageBits_Storage,
            .storage = vk::StorageType::HostVisible,
            // Use initial capacity
            .size = kInitialObjectCapacity * sizeof(BoundingBox) }).
            UseResource("ObjectBounds", AccessType::Write).
    DeclareTransientResource(
      "DrawDataBuffer",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        // Use initial capacity
        .size = kInitialObjectCapacity * sizeof(DrawData) }).
        UseResource("DrawDataBuffer", AccessType::Write).
    DeclareTransientResource(
      "IndirectBufferOpaque",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Indirect |
        vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
      // Use initial capacity
      .size = kInitialObjectCapacity * sizeof(
        DrawIndexedIndirectCommand) +
      sizeof(uint32_t) }).UseResource(
        "IndirectBufferOpaque",
        AccessType::Write).
    DeclareTransientResource(
      "IndirectBufferTransparent",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Indirect |
        vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = kInitialObjectCapacity * sizeof(
          DrawIndexedIndirectCommand) +
        sizeof(uint32_t) }).
    UseResource("IndirectBufferTransparent", AccessType::Write).
    DeclareTransientResource(
      "CullingData",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        // This buffer is fixed-size, so no change is needed.
        .size = sizeof(CullingData) }).UseResource(
          "CullingData",
          AccessType::Write)
    .SetPriority(
      internal::RenderPassBuilder::PassPriority::EarlySetup).
    AddGenericPass(
      "SceneCulling",
      [this](internal::ExecutionContext& ctx)
  {
    ExecuteCullingPass(ctx);
  });

  passBuilder.CreatePass().
    UseResource(Lighting::CLUSTER_BOUNDS, AccessType::Write).
    UseResource(Lighting::LIGHTING_BUFFER, AccessType::Read).
    UseResource(RenderResources::SCENE_COLOR, AccessType::Read).
    SetPriority(
      internal::RenderPassBuilder::PassPriority::LightCulling).
    AddGenericPass(
      "ClusterBoundsGeneration",
      [this](internal::ExecutionContext& ctx)
  {
    executeClusterBoundsGeneration(ctx);
  });

  // Light Culling Pass  
  passBuilder.CreatePass().UseResource(Lighting::LIGHT_BUFFER, AccessType::Read).
    UseResource(Lighting::CLUSTER_BOUNDS, AccessType::Read).
    UseResource(Lighting::LIGHT_INDICES, AccessType::Write).
    UseResource(Lighting::LIGHT_LISTS, AccessType::Write).SetPriority(
      internal::RenderPassBuilder::PassPriority::LightCulling).
    AddGenericPass(
      "LightCulling",
      [this](internal::ExecutionContext& ctx)
  {
    executeLightCulling(ctx);
  });
  // Calculate lighting buffer sizes
  uint32_t lightBufferSize = Parameters::MAX_LIGHTS * sizeof(
    Lighting::GPULight);
  uint32_t clusterBoundsSize = Lighting::TOTAL_CLUSTERS * sizeof(
    Lighting::ClusterBounds);
  uint32_t lightIndicesSize = sizeof(uint32_t) +
    Lighting::MAX_TOTAL_LIGHT_INDICES * sizeof(uint32_t);
  uint32_t lightListsSize = Lighting::TOTAL_CLUSTERS * sizeof(
    Lighting::LightList);

  // Lighting Setup Pass - initialize lighting data
  passBuilder.CreatePass().
    UseResource(RenderResources::SCENE_COLOR, AccessType::Read).
    DeclareTransientResource(
      Lighting::LIGHT_BUFFER,
      vk::BufferDesc
      {
        .usage = vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = lightBufferSize }).
        DeclareTransientResource(
          Lighting::CLUSTER_BOUNDS,
          vk::BufferDesc
          {
            .usage = vk::BufferUsageBits_Storage,
            .storage = vk::StorageType::Device,
            .size = clusterBoundsSize }).
            DeclareTransientResource(
              Lighting::LIGHT_INDICES,
              vk::BufferDesc
              {
                .usage = vk::BufferUsageBits_Storage,
                .storage = vk::StorageType::Device,
                .size = lightIndicesSize }).
                DeclareTransientResource(
                  Lighting::LIGHT_LISTS,
                  vk::BufferDesc
                  {
                    .usage = vk::BufferUsageBits_Storage,
                    .storage = vk::StorageType::Device,
                    .size = lightListsSize }).DeclareTransientResource(
                      Lighting::LIGHTING_BUFFER,
                      vk::BufferDesc{
                        .usage = vk::BufferUsageBits_Storage,
                        .storage = vk::StorageType::HostVisible,
                        .size = sizeof(Lighting::LightingBuffer) }).UseResource(
                          Lighting::LIGHT_BUFFER,
                          AccessType::Write).UseResource(
                            Lighting::LIGHTING_BUFFER,
                            AccessType::Write).SetPriority(
                              internal::RenderPassBuilder::PassPriority::EarlySetup).
    AddGenericPass(
      "LightingSetup",
      [this](internal::ExecutionContext& ctx)
  {
    executeLightingSetup(ctx);
  });

  PassDeclarationInfo opaquePass;
  opaquePass.framebufferDebugName = "OpaqueScene";
  opaquePass.colorAttachments[0] = {
      .textureName = RenderResources::SCENE_COLOR,
      .loadOp = vk::LoadOp::Load,         // Changed from Clear
      .storeOp = vk::StoreOp::Store
  };
  opaquePass.depthAttachment = {
      .textureName = RenderResources::SCENE_DEPTH,
      .loadOp = vk::LoadOp::Load,         // Changed from Clear
      .storeOp = vk::StoreOp::Store
  };
  passBuilder.CreatePass().UseResource("ObjectTransforms", AccessType::Read).
    UseResource("DrawDataBuffer", AccessType::Read).
    UseResource("IndirectBufferOpaque", AccessType::Read).
    UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::INDEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::MATERIAL_BUFFER, AccessType::Read).
    UseResource(Lighting::LIGHTING_BUFFER, AccessType::Read).
    UseResource(RenderResources::SCENE_COLOR, AccessType::Write).
    UseResource(RenderResources::SCENE_DEPTH, AccessType::Write).
    SetPriority(internal::RenderPassBuilder::PassPriority::Opaque).
    AddGraphicsPass(
      "SceneOpaque",
      opaquePass,
      [this](internal::ExecutionContext& ctx)
  {
    ExecuteOpaquePass(ctx);
  });
  passBuilder.CreatePass().UseResource("ObjectTransforms", AccessType::Read)
    // Add this
    .UseResource("DrawDataBuffer", AccessType::Read) // Add this  
    .UseResource("IndirectBufferTransparent", AccessType::Read).
    UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::INDEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::MATERIAL_BUFFER, AccessType::Read).
    UseResource(Lighting::LIGHTING_BUFFER, AccessType::Read).
    UseResource(RenderResources::SCENE_DEPTH, AccessType::Read)
    // Read depth for testing
    .UseResource(OITResources::HEAD_TEXTURE, AccessType::ReadWrite)
    // OIT head pointers
    .UseResource(OITResources::FRAGMENT_BUFFER, AccessType::Write)
    // OIT fragment storage  
    .UseResource(OITResources::ATOMIC_COUNTER, AccessType::ReadWrite)
    // Fragment allocation counter
    .SetPriority(
      internal::RenderPassBuilder::PassPriority::Transparent).
    AddGraphicsPass(
      "SceneTransparent",
      PassDeclarationInfo{
        .depthAttachment = {
          .textureName = RenderResources::SCENE_DEPTH,
          .loadOp = vk::LoadOp::Load,
          .storeOp = vk::StoreOp::Store // Preserve for later passes
        } },
      [this](internal::ExecutionContext& context)
  {
    ExecuteTransparentPass(context);
  });
  PassDeclarationInfo skyboxPassInfo;
  skyboxPassInfo.framebufferDebugName = "Skybox";
  skyboxPassInfo.colorAttachments[0] = {
      .textureName = RenderResources::SCENE_COLOR,
      .loadOp = vk::LoadOp::Load,        // Clear here instead
      .storeOp = vk::StoreOp::Store
  };
  skyboxPassInfo.depthAttachment = {
      .textureName = RenderResources::SCENE_DEPTH,
      .loadOp = vk::LoadOp::Clear,        // Clear here instead
      .storeOp = vk::StoreOp::Store,
      .clearDepth = 1.0f
  };
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::Write)
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Read) // Read-only for depth test
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(299)) // Just before opaque
    .AddGraphicsPass(
      "Skybox",
      skyboxPassInfo,
      [this](internal::ExecutionContext& ctx) {
    ExecuteSkyboxPass(ctx);
  });

};

void SceneRenderFeature::ExecuteCullingPass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  EnsureComputePipeline(ctx);

  const size_t objectCount = params.getObjectCount();
  const size_t drawCommandCount = params.getDrawCommandCount();
  const size_t opaqueCount = params.getOpaqueCount();
  const size_t transparentCount = params.getTransparentCount();
  if(objectCount == 0)
  {
    // Clear the indirect buffer's draw count to 0 if there are no objects
    // to prevent rendering stale data from a previous frame.
    auto indirectBuffer = ctx.GetBuffer("IndirectBufferOpaque");
    const uint32_t zero = 0;
    ctx.GetvkContext().upload(indirectBuffer, &zero, sizeof(uint32_t));
    return;
  }
  if(transparentCount == 0)
  {
    auto transparentIndirectBuffer = ctx.GetBuffer("IndirectBufferTransparent");
    const uint32_t zero = 0;
    ctx.GetvkContext().upload(transparentIndirectBuffer, &zero, sizeof(uint32_t));
  }
  const size_t requiredTransformsSize = objectCount * sizeof(mat4);
  const size_t requiredBoundsSize = objectCount * sizeof(BoundingBox);
  const size_t requiredDrawDataSize = drawCommandCount * sizeof(DrawData);
  const size_t requiredIndirectSize = sizeof(uint32_t) + opaqueCount * sizeof(
    DrawIndexedIndirectCommand);
  const size_t requiredTransparentIndirectSize = sizeof(uint32_t) +
    transparentCount * sizeof(DrawIndexedIndirectCommand);

  // Resize buffers if they are not large enough. The render graph handles the details.
  if(requiredTransformsSize > ctx.GetBufferSize("ObjectTransforms"))
    ctx.ResizeBuffer("ObjectTransforms", requiredTransformsSize);
  if(requiredBoundsSize > ctx.GetBufferSize("ObjectBounds"))
    ctx.ResizeBuffer("ObjectBounds", requiredBoundsSize);
  if(requiredDrawDataSize > ctx.GetBufferSize("DrawDataBuffer"))
    ctx.ResizeBuffer("DrawDataBuffer", requiredDrawDataSize);
  if(requiredIndirectSize > ctx.GetBufferSize("IndirectBufferOpaque"))
    ctx.ResizeBuffer("IndirectBufferOpaque", requiredIndirectSize);
  if(requiredTransparentIndirectSize > ctx.GetBufferSize(
    "IndirectBufferTransparent"))
  {
    ctx.ResizeBuffer(
      "IndirectBufferTransparent",
      requiredTransparentIndirectSize);
  }

  auto& cmd = ctx.GetvkCommandBuffer();

  // The rest of the function remains the same, but now it operates on correctly-sized buffers.
  ctx.GetvkContext().upload(
    ctx.GetBuffer("ObjectTransforms"),
    params.objectTransforms.data(),
    requiredTransformsSize);
  ctx.GetvkContext().upload(
    ctx.GetBuffer("ObjectBounds"),
    params.objectBounds.data(),
    requiredBoundsSize);
  ctx.GetvkContext().upload(
    ctx.GetBuffer("DrawDataBuffer"),
    params.drawData.data(),
    requiredDrawDataSize);

  CullingData cullingData = {};
  cullingData.numMeshesToCull = static_cast<uint32_t>(opaqueCount);
  cullingData.numVisibleMeshes = 0;

  const FrameData& frameData = ctx.GetFrameData();
  mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
  getFrustumPlanes(viewProj, cullingData.frustumPlanes);
  getFrustumCorners(viewProj, cullingData.frustumCorners);

  ctx.GetvkContext().upload(
    ctx.GetBuffer("CullingData"),
    &cullingData,
    sizeof(CullingData));

  auto indirectBuffer = ctx.GetBuffer("IndirectBufferOpaque");
  ctx.GetvkContext().upload(indirectBuffer, &opaqueCount, sizeof(uint32_t));

  auto transparentIndirectBuffer = ctx.GetBuffer("IndirectBufferTransparent");
  ctx.GetvkContext().upload(
    transparentIndirectBuffer,
    &transparentCount,
    sizeof(uint32_t));

  if(opaqueCount > 0)
  {
    std::vector<DrawIndexedIndirectCommand> opaqueCommands;
    opaqueCommands.reserve(opaqueCount);
    for(uint32_t i = 0; i < opaqueCount; ++i)
    {
      uint32_t drawIndex = params.opaqueIndices[i];
      opaqueCommands.push_back(params.drawCommands[drawIndex]);
    }

    ctx.GetvkContext().upload(
      indirectBuffer,
      opaqueCommands.data(),
      opaqueCommands.size() * sizeof(DrawIndexedIndirectCommand),
      sizeof(uint32_t)); // Offset past the count
  }

  if(transparentCount > 0)
  {
    std::vector<DrawIndexedIndirectCommand> transparentCommands;
    transparentCommands.reserve(transparentCount);
    for(uint32_t i = 0; i < transparentCount; ++i)
    {
      uint32_t drawIndex = params.transparentIndices[i];
      transparentCommands.push_back(params.drawCommands[drawIndex]);
    }

    ctx.GetvkContext().upload(
      transparentIndirectBuffer,
      transparentCommands.data(),
      transparentCommands.size() * sizeof(DrawIndexedIndirectCommand),
      sizeof(uint32_t));
  }
  struct
  {
    uint64_t commands;
    uint64_t drawData;
    uint64_t AABBs;
    uint64_t frustum;
  } pushConstants = {
      .commands = ctx.GetvkContext().gpuAddress(indirectBuffer),
      .drawData = ctx.GetvkContext().
                      gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
      .AABBs = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectBounds")),
      .frustum = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("CullingData")) };

  cmd.cmdBindComputePipeline(m_cullingPipeline);
  cmd.cmdPushConstants(pushConstants);

  uint32_t numThreadGroups = static_cast<uint32_t>((opaqueCount + 63) / 64);
  if(numThreadGroups > 0)
    cmd.cmdDispatchThreadGroups({ numThreadGroups });
}

void SceneRenderFeature::ExecuteOpaquePass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  EnsureRenderPipelines(ctx);
  if(params.getOpaqueCount() == 0)
    return;

  const FrameData& frameData = ctx.GetFrameData();
  auto& cmd = ctx.GetvkCommandBuffer();

  cmd.cmdBindIndexBuffer(
    ctx.GetBuffer(RenderResources::INDEX_BUFFER),
    vk::IndexFormat::UI32);
  cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
  cmd.cmdBindRenderPipeline(m_opaqueRenderPipeline);
  cmd.cmdBindDepthState(
    { .compareOp = vk::CompareOp::Less, .isDepthWriteEnabled = true });

  PushConstants pushConstants = {
    .viewProj = frameData.projMatrix * frameData.viewMatrix,
    .cameraPos = vec4(frameData.cameraPos,1.0f),
    .bufferTransforms = ctx.GetvkContext().
                            gpuAddress(ctx.GetBuffer("ObjectTransforms")),
    .bufferDrawData = ctx.GetvkContext().
                          gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
    .bufferMaterials = ctx.GetvkContext().gpuAddress(
      ctx.GetBuffer(RenderResources::MATERIAL_BUFFER)),
    .oitBuffer = 0,
    // Not used in opaque pass
    .lightingBuffer = ctx.GetvkContext().gpuAddress(
      ctx.GetBuffer(Lighting::LIGHTING_BUFFER)),
    .irradianceTexture = params.irradianceTexture,
    .prefilteredTexture = params.prefilterTexture,
    .brdfLutTexture = params.brdfLUT,
    .environmentIntensity = params.environmentIntensity
  };

  cmd.cmdPushConstants(pushConstants);
  cmd.cmdDrawIndexedIndirectCount(
    ctx.GetBuffer("IndirectBufferOpaque"),
    sizeof(uint32_t),
    ctx.GetBuffer("IndirectBufferOpaque"),
    0,
    params.getOpaqueCount(),
    sizeof(DrawIndexedIndirectCommand));
}

void SceneRenderFeature::ExecuteTransparentPass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  if(params.getTransparentCount() == 0)
    return;

  const FrameData& frameData = ctx.GetFrameData();
  auto& cmd = ctx.GetvkCommandBuffer();

  cmd.cmdBindRenderPipeline(m_transparentRenderPipeline);
  cmd.cmdBindIndexBuffer(
    ctx.GetBuffer(RenderResources::INDEX_BUFFER),
    vk::IndexFormat::UI32);
  cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));

  cmd.cmdBindDepthState(
    { .compareOp = vk::CompareOp::Less, .isDepthWriteEnabled = false });

  // Same structure as opaque - OIT buffer contains all metadata
  PushConstants pushConstants = {
    .viewProj = frameData.projMatrix * frameData.viewMatrix,
    .cameraPos = vec4(frameData.cameraPos,1.0f),
    .bufferTransforms = ctx.GetvkContext().
                            gpuAddress(ctx.GetBuffer("ObjectTransforms")),
    .bufferDrawData = ctx.GetvkContext().
                          gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
    .bufferMaterials = ctx.GetvkContext().
                           gpuAddress(
                             ctx.GetBuffer(RenderResources::MATERIAL_BUFFER)),
    .oitBuffer = ctx.GetvkContext().gpuAddress(
      ctx.GetBuffer(OITResources::OIT_BUFFER)),
    .lightingBuffer = ctx.GetvkContext().gpuAddress(
      ctx.GetBuffer(Lighting::LIGHTING_BUFFER)),
    .irradianceTexture = params.irradianceTexture,
    .prefilteredTexture = params.prefilterTexture,
    .brdfLutTexture = params.brdfLUT,
    .environmentIntensity = params.environmentIntensity
  };

  cmd.cmdPushConstants(pushConstants);

  cmd.cmdDrawIndexedIndirectCount(
    ctx.GetBuffer("IndirectBufferTransparent"),
    sizeof(uint32_t),
    ctx.GetBuffer("IndirectBufferTransparent"),
    0,
    params.getTransparentCount(),
    sizeof(DrawIndexedIndirectCommand));
}

void SceneRenderFeature::executeLightingSetup(internal::ExecutionContext& ctx)
{
  const auto& frameData = ctx.GetFrameData();
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());

  // Get all the lighting buffers
  vk::BufferHandle lightBuffer = ctx.GetBuffer(Lighting::LIGHT_BUFFER);
  vk::BufferHandle clusterBounds = ctx.GetBuffer(Lighting::CLUSTER_BOUNDS);
  vk::BufferHandle lightIndices = ctx.GetBuffer(Lighting::LIGHT_INDICES);
  vk::BufferHandle lightLists = ctx.GetBuffer(Lighting::LIGHT_LISTS);
  vk::BufferHandle lightingBuffer = ctx.GetBuffer(Lighting::LIGHTING_BUFFER);

  if(params.activeLightCount > 0)
  {
    void* mapped = ctx.GetvkContext().getMappedPtr(lightBuffer);
    std::memcpy(
      mapped,
      params.lights.data(),
      params.activeLightCount * sizeof(Lighting::GPULight));
    ctx.GetvkContext().flushMappedMemory(
      lightBuffer,
      0,
      params.activeLightCount * sizeof(Lighting::GPULight));
  }

  // Calculate screen dimensions and camera parameters
  vk::TextureHandle sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  vk::Dimensions screenDims = ctx.GetvkContext().getDimensions(sceneColor);

  // Create lighting metadata buffer
  Lighting::LightingBuffer lightingData = {
    .bufferLights = ctx.GetvkContext().gpuAddress(lightBuffer),
    .bufferClusterBounds = ctx.GetvkContext().gpuAddress(clusterBounds),
    .bufferLightIndices = ctx.GetvkContext().gpuAddress(lightIndices),
    .bufferLightLists = ctx.GetvkContext().gpuAddress(lightLists),
    .totalLightCount = params.activeLightCount,
    .clusterDimX = Lighting::CLUSTER_DIM_X,
    .clusterDimY = Lighting::CLUSTER_DIM_Y,
    .clusterDimZ = Lighting::CLUSTER_DIM_Z,
    .zNear = frameData.zNear,
    .zFar = frameData.zFar,
    .screenDims = vec2(
      static_cast<float>(screenDims.width),
      static_cast<float>(screenDims.height)),
    .pad0 = 0,
    .pad1 = 0,
    .viewMatrix = frameData.viewMatrix
  };

  // Upload lighting metadata
  void* mapped = ctx.GetvkContext().getMappedPtr(lightingBuffer);
  std::memcpy(mapped, &lightingData, sizeof(Lighting::LightingBuffer));
  ctx.GetvkContext().flushMappedMemory(
    lightingBuffer,
    0,
    sizeof(Lighting::LightingBuffer));
}

// Add to scene_feature.cpp

void SceneRenderFeature::executeClusterBoundsGeneration(
  internal::ExecutionContext& ctx
)
{
  EnsureLightingPipelines(ctx);

  const auto& frameData = ctx.GetFrameData();
  auto& cmd = ctx.GetvkCommandBuffer();

  float fovRadians = glm::radians(frameData.fovY);
  float tanHalfFovY = tan(fovRadians * 0.5f);

  vk::TextureHandle sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  vk::Dimensions screenDims = ctx.GetvkContext().getDimensions(sceneColor);

  // FIX: Match the GLSL layout exactly
  struct ClusterBoundsPushConstants
  {
    uint64_t clusterBounds;
    uint32_t clusterDimX, clusterDimY, clusterDimZ;
    // These will be read as uvec3 
    float screenWidth, screenHeight;
    float zNear, zFar;
    float tanHalfFovY;
    uint32_t pad[3]; // Ensure 16-byte alignment
  } pushConstants = {
      .clusterBounds = ctx.GetvkContext().
                           gpuAddress(ctx.GetBuffer(Lighting::CLUSTER_BOUNDS)),
      .clusterDimX = Lighting::CLUSTER_DIM_X,
      .clusterDimY = Lighting::CLUSTER_DIM_Y,
      .clusterDimZ = Lighting::CLUSTER_DIM_Z,
      .screenWidth = static_cast<float>(screenDims.width),
      .screenHeight = static_cast<float>(screenDims.height),
      .zNear = frameData.zNear,
      .zFar = frameData.zFar,
      .tanHalfFovY = tanHalfFovY,
      .pad = {0, 0, 0} };

  cmd.cmdBindComputePipeline(m_clusterBoundsPipeline);
  cmd.cmdPushConstants(pushConstants);

  uint32_t groupsX = (Lighting::CLUSTER_DIM_X + 3) / 4;
  uint32_t groupsY = (Lighting::CLUSTER_DIM_Y + 3) / 4;
  uint32_t groupsZ = (Lighting::CLUSTER_DIM_Z + 3) / 4;
  cmd.cmdDispatchThreadGroups({ groupsX, groupsY, groupsZ });
}

void SceneRenderFeature::executeLightCulling(internal::ExecutionContext& ctx)
{
  EnsureLightingPipelines(ctx);

  const auto& frameData = ctx.GetFrameData();
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  auto& cmd = ctx.GetvkCommandBuffer();

  // Reset atomic counter
  vk::BufferHandle lightIndices = ctx.GetBuffer(Lighting::LIGHT_INDICES);
  cmd.cmdFillBuffer(lightIndices, 0, sizeof(uint32_t), 0);

  struct LightCullingPushConstants
  {
    uint64_t lights;
    uint64_t clusterBounds;
    uint64_t lightIndices;
    uint64_t lightLists;
    mat4 viewMatrix;
    uint32_t totalLightCount;
    uint32_t totalClusters;
    uint32_t clusterDim[3];
    uint32_t pad; // Ensure 16-byte alignment
  } pushConstants = {
      .lights = ctx.GetvkContext().
                    gpuAddress(ctx.GetBuffer(Lighting::LIGHT_BUFFER)),
      .clusterBounds = ctx.GetvkContext().
                           gpuAddress(ctx.GetBuffer(Lighting::CLUSTER_BOUNDS)),
      .lightIndices = ctx.GetvkContext().gpuAddress(lightIndices),
      .lightLists = ctx.GetvkContext().
                        gpuAddress(ctx.GetBuffer(Lighting::LIGHT_LISTS)),
      .viewMatrix = frameData.viewMatrix,
      .totalLightCount = params.activeLightCount,
      .totalClusters = Lighting::TOTAL_CLUSTERS,
      .clusterDim = {
        Lighting::CLUSTER_DIM_X,
        Lighting::CLUSTER_DIM_Y,
        Lighting::CLUSTER_DIM_Z} };

  cmd.cmdBindComputePipeline(m_lightCullingPipeline);
  cmd.cmdPushConstants(pushConstants);

  // One thread per cluster (following your culling.comp pattern)
  uint32_t groupsX = (Lighting::CLUSTER_DIM_X + 3) / 4;
  uint32_t groupsY = (Lighting::CLUSTER_DIM_Y + 3) / 4;
  uint32_t groupsZ = (Lighting::CLUSTER_DIM_Z + 3) / 4;
  cmd.cmdDispatchThreadGroups({ groupsX, groupsY, groupsZ });
}

void SceneRenderFeature::EnsureLightingPipelines(internal::ExecutionContext& ctx
)
{
  if(m_clusterBoundsPipeline.empty())
  {
    m_clusterBoundsShader = loadShaderModule(
      ctx.GetvkContext(),
      "shaders/cluster_bounds.comp");
    vk::ComputePipelineDesc desc = {
      .smComp = m_clusterBoundsShader,
      .debugName = "ClusterBoundsPipeline" };
    m_clusterBoundsPipeline = ctx.GetvkContext().createComputePipeline(desc);
  }

  if(m_lightCullingPipeline.empty())
  {
    m_lightCullingShader = loadShaderModule(
      ctx.GetvkContext(),
      "shaders/light_culling.comp");
    vk::ComputePipelineDesc desc = {
      .smComp = m_lightCullingShader,
      .debugName = "LightCullingPipeline" };
    m_lightCullingPipeline = ctx.GetvkContext().createComputePipeline(desc);
  }
}

void SceneRenderFeature::ExecuteSkyboxPass(internal::ExecutionContext& ctx)
{
    const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());
    
    EnsureSkyboxPipeline(ctx);
    
    const FrameData& frameData = ctx.GetFrameData();
    auto& cmd = ctx.GetvkCommandBuffer();
    
    cmd.cmdBindRenderPipeline(m_skyboxRenderPipeline);
    cmd.cmdBindDepthState({
        .compareOp = vk::CompareOp::LessEqual,  // Allow far plane
        .isDepthWriteEnabled = false
    });
    
    // CRITICAL: Remove translation from view matrix
    mat4 viewOnly = frameData.viewMatrix;
    viewOnly[3][0] = 0.0f;  // Remove translation
    viewOnly[3][1] = 0.0f;
    viewOnly[3][2] = 0.0f;
    
    // Precompute inverse on CPU
    mat4 viewProj = frameData.projMatrix * viewOnly;
    mat4 invViewProj = inverse(viewProj);
    
    struct SkyboxPushConstants {
        mat4 invViewProj;
        vec4 cameraPos;
        uint32_t environmentTexture;
        float environmentIntensity;
    } pushConstants = {
        .invViewProj = invViewProj,
        .cameraPos = vec4(frameData.cameraPos, 1.0f),
        .environmentTexture = params.prefilterTexture,
        .environmentIntensity = params.environmentIntensity
    };
    
    cmd.cmdPushConstants(pushConstants);
    cmd.cmdDraw(3, 1, 0, 0); // 3 vertices for triangle
}

void SceneRenderFeature::convertSceneLight(const SceneLight& sceneLight,
                                           Lighting::GPULight& gpuLight
)
{
  // Convert position (already in world space)
  gpuLight.position = sceneLight.position;

  // Convert color and intensity
  gpuLight.color = sceneLight.color * sceneLight.intensity;

  // Convert type
  gpuLight.type = static_cast<uint32_t>(sceneLight.type);

  // Convert direction (normalize)
  gpuLight.direction = normalize(sceneLight.direction);

  // Calculate range based on attenuation
  switch(sceneLight.type)
  {
    case LightType::Point:
    case LightType::Spot:
    {
      // For point/spot lights, calculate range where light contribution becomes negligible
      // Using the formula: range where attenuation drops to 1/256 of original intensity
      const float minIntensity = 1.0f / 256.0f;
      const float constant = sceneLight.attenuation.x;
      const float linear = sceneLight.attenuation.y;
      const float quadratic = sceneLight.attenuation.z;

      if(quadratic > 0.0f)
      {
        // Solve quadratic equation: constant + linear*d + quadratic*d^2 = 1/minIntensity
        float a = quadratic;
        float b = linear;
        float c = constant - (1.0f / minIntensity);
        float discriminant = b * b - 4.0f * a * c;

        if(discriminant >= 0.0f)
          gpuLight.range = (-b + sqrt(discriminant)) / (2.0f * a);
        else
          gpuLight.range = 100.0f; // Fallback
      }
      else if(linear > 0.0f)
      {
        // Linear attenuation only
        gpuLight.range = (1.0f / minIntensity - constant) / linear;
      }
      else
      {
        // Constant attenuation only - light never falls off
        gpuLight.range = 1000.0f; // Large but finite range
      }

      // Clamp to reasonable bounds
      gpuLight.range = std::clamp(gpuLight.range, 0.1f, 1000.0f);
      break;
    }

    case LightType::Directional:
      // Directional lights have infinite range
      gpuLight.range = 10000.0f;
      break;

    case LightType::Area:
      // Area lights - use a reasonable default for now
      gpuLight.range = length(sceneLight.areaSize) * 10.0f;
      break;
  }

  // Convert spot light angle
  if(sceneLight.type == LightType::Spot)
    gpuLight.spotAngle = cos(sceneLight.outerConeAngle);
  else
    gpuLight.spotAngle = -1.0f; // Not a spot light
}

void SceneRenderFeature::EnsureSkyboxPipeline(internal::ExecutionContext& ctx) {
    if (m_skyboxRenderPipeline.valid()) return;
    
    m_skyboxVertShader = loadShaderModule(ctx.GetvkContext(), "shaders/skybox.vert");
    m_skyboxFragShader = loadShaderModule(ctx.GetvkContext(), "shaders/skybox.frag");
    
    vk::TextureHandle color = ctx.GetTexture(RenderResources::SCENE_COLOR);
    vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
    
    vk::RenderPipelineDesc desc = {
        .vertexInput = {}, // No vertex input
        .smVert = m_skyboxVertShader,
        .smFrag = m_skyboxFragShader,
        .color = {{
            .format = ctx.GetvkContext().getFormat(color),
            .blendEnabled = false
        }},
        .depthFormat = ctx.GetvkContext().getFormat(depth),
        .cullMode = vk::CullMode::None,  // No culling for screen triangle
        .samplesCount = sampleCount,
        .debugName = "SkyboxPipeline"
    };
    
    m_skyboxRenderPipeline = ctx.GetvkContext().createRenderPipeline(desc);
    if (!m_skyboxRenderPipeline.valid()) {
        throw std::runtime_error("Failed to create skybox render pipeline");
    }
}

void SceneRenderFeature::EnsureComputePipeline(internal::ExecutionContext& ctx)
{
  if(m_cullingPipeline.empty())
  {
    m_cullingShader = loadShaderModule(
      ctx.GetvkContext(),
      "shaders/culling.comp");
    vk::ComputePipelineDesc desc = {
      .smComp = m_cullingShader,
      .debugName = "SceneCullingPipeline" };
    m_cullingPipeline = ctx.GetvkContext().createComputePipeline(desc);
    if(!m_cullingPipeline.valid())
      throw std::runtime_error("Failed to create culling compute pipeline");
  }
}

void SceneRenderFeature::EnsureRenderPipelines(internal::ExecutionContext& ctx)
{
  if(m_opaqueRenderPipeline.valid() && m_transparentRenderPipeline.valid())
    return;
  vk::VertexInput vertexInput;
  vertexInput.attributes[0] = {
    .location = 0,
    .format = vk::VertexFormat::Float3,
    .offset = offsetof(Vertex,position) };
  vertexInput.attributes[1] = {
    .location = 1,
    .format = vk::VertexFormat::Float1,
    .offset = offsetof(Vertex,uv_x) };
  vertexInput.attributes[2] = {
    .location = 2,
    .format = vk::VertexFormat::Float3,
    .offset = offsetof(Vertex,normal) };
  vertexInput.attributes[3] = {
    .location = 3,
    .format = vk::VertexFormat::Float1,
    .offset = offsetof(Vertex,uv_y) };
  vertexInput.attributes[4] = {
    .location = 4,
    .format = vk::VertexFormat::Float4,
    .offset = offsetof(Vertex,tangent) };
  vertexInput.inputBindings[0].stride = sizeof(Vertex);
  vertShader = loadShaderModule(ctx.GetvkContext(), "shaders/object.vert");
  if(!m_opaqueRenderPipeline.valid())
  {
    vk::TextureHandle color = ctx.GetTexture(RenderResources::SCENE_COLOR);
    vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);

    opaquefragShader = loadShaderModule(
      ctx.GetvkContext(),
      "shaders/opaque.frag");
    // Create render pipeline descriptor  
    vk::RenderPipelineDesc desc = {
      .vertexInput = vertexInput,
      .smVert = vertShader,
      .smFrag = opaquefragShader,
      .color = {
        {.format = ctx.GetvkContext().getFormat(color), .blendEnabled = false}},

        // Depth attachment (SCENE_DEPTH format) 
        .depthFormat = depth.valid() ? ctx.GetvkContext().getFormat(depth)
                         : vk::Format::Invalid,
      // Rasterization settings
      .cullMode = vk::CullMode::Back,
      // Back-face culling
      // MSAA settings (if using multisampling)
      .samplesCount = sampleCount,
      // Adjust based on your MSAA settings
      .minSampleShading = 0.0f,
      .debugName = "SceneOpaquePipeline" };

    m_opaqueRenderPipeline = ctx.GetvkContext().createRenderPipeline(desc);
    if(!m_opaqueRenderPipeline.valid())
      throw std::runtime_error("Failed to create opaque render pipeline");
  }
  if(!m_transparentRenderPipeline.valid())
  {
    vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
    transparentfragShader = loadShaderModule(
      ctx.GetvkContext(),
      "shaders/transparent.frag");
    vk::RenderPipelineDesc desc = {
      .vertexInput = vertexInput,
      .smVert = vertShader,
      .smFrag = transparentfragShader,
      // Depth attachment (SCENE_DEPTH format) 
      .depthFormat = ctx.GetvkContext().getFormat(depth),
      // Rasterization settings
      .cullMode = vk::CullMode::None,
      // Back-face culling
      // MSAA settings (if using multisampling)
      .samplesCount = sampleCount,
      // Adjust based on your MSAA settings
      .debugName = "SceneTransparentPipeline" };

    m_transparentRenderPipeline = ctx.GetvkContext().createRenderPipeline(desc);
    if(!m_transparentRenderPipeline.valid())
      throw std::runtime_error("Failed to create transparent render pipeline");
  }
}

const char* SceneRenderFeature::GetName() const
{
  return "SceneRenderFeature";
}
