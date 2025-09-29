#include "scene_feature.h"
#include "GameSettings.h"
#include "graphics/renderer.h"
#include "graphics/shader.h"
#include "assets/core/asset_system.h"
#include "assets/io/scene_loader.h"

using namespace Render;

void SceneRenderFeature::UpdateScene(uint64_t renderFeatureID,
                                     const AssetLoading::Scene& gameScene,
                                     const AssetLoading::AssetSystem&
                                     asset_system,
                                     Renderer& renderer)
{
  SceneRenderParams* params = static_cast<SceneRenderParams*>(renderer.
    GetFeatureParameterBlockPtr(renderFeatureID));
  if (!params)
    return;

  params->clear();
  params->reserve(gameScene.objects.size());

  // Build all scene data in one pass
  for (size_t objectIndex = 0; objectIndex < gameScene.objects.size(); ++
       objectIndex)
  {
    const SceneObject& obj = gameScene.objects[objectIndex];

    // Validate handles
    if (!obj.mesh.isValid() || !obj.material.isValid())
      continue;

    const auto* meshData = asset_system.getMesh(obj.mesh);
    if (!meshData)
      continue;

    // Store object data
    params->objectTransforms.push_back(obj.transform);
    params->materialIndices.push_back(
      asset_system.getMaterialIndex(obj.material));

    // Transform mesh bounds to world space
    vec3 center = vec3(meshData->bounds.x,
      meshData->bounds.y,
      meshData->bounds.z);
    float radius = meshData->bounds.w;
    vec3 worldCenter = vec3(obj.transform * vec4(center,1.0f));
    params->objectBounds.emplace_back(worldCenter - vec3(radius),
      worldCenter + vec3(radius));

    // Build draw command
    DrawIndexedIndirectCommand cmd = {.count = meshData->indexCount,
      .instanceCount = 1,
      .firstIndex = static_cast<uint32_t>(
        meshData->indexByteOffset / sizeof(uint32_t)),
      .baseVertex = static_cast<int32_t>(
        meshData->vertexByteOffset / sizeof(Vertex)),
      .baseInstance = static_cast<uint32_t>(objectIndex)};

    DrawData drawData = {.transformId = static_cast<uint32_t>(objectIndex),
      .materialId = asset_system.getMaterialIndex(obj.material)};

    uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.
      size());
    params->drawCommands.push_back(cmd);
    params->drawData.push_back(drawData);

    // Determine transparency and add to render queues
    bool isTransparent = asset_system.isMaterialTransparent(obj.material);

    if (isTransparent)
    {
      params->transparentIndices.push_back(drawCommandIndex);
    }
    else
    {
      params->opaqueIndices.push_back(drawCommandIndex);
    }
  }

  /*LOG_INFO("SceneRenderFeature: Updated {} objects ({} opaque, {} transparent)",
           params->getObjectCount(),
           params->getOpaqueCount(),
           params->getTransparentCount());*/
}

void SceneRenderFeature::SetupPasses(Render::internal::RenderPassBuilder& passBuilder)
{
    constexpr size_t kInitialObjectCapacity = 1024;

    // Culling pass
    passBuilder.CreatePass()
        .DeclareTransientResource("ObjectTransforms",
            vk::BufferDesc{
                .usage = vk::BufferUsageBits_Storage,
                .storage = vk::StorageType::HostVisible,
                // Use initial capacity instead of MAX_OBJECTS
                .size = kInitialObjectCapacity * sizeof(mat4)
            })
        .UseResource("ObjectTransforms", AccessType::Write)
        .DeclareTransientResource("ObjectBounds",
            vk::BufferDesc{
                .usage = vk::BufferUsageBits_Storage,
                .storage = vk::StorageType::HostVisible,
                // Use initial capacity
                .size = kInitialObjectCapacity * sizeof(BoundingBox)
            })
        .UseResource("ObjectBounds", AccessType::Write)
        .DeclareTransientResource("DrawDataBuffer",
            vk::BufferDesc{
                .usage = vk::BufferUsageBits_Storage,
                .storage = vk::StorageType::HostVisible,
                // Use initial capacity
                .size = kInitialObjectCapacity * sizeof(DrawData)
            })
        .UseResource("DrawDataBuffer", AccessType::Write)
        .DeclareTransientResource("IndirectBufferOpaque",
            vk::BufferDesc{
                .usage = vk::BufferUsageBits_Indirect | vk::BufferUsageBits_Storage,
                .storage = vk::StorageType::HostVisible,
                // Use initial capacity
                .size = kInitialObjectCapacity * sizeof(DrawIndexedIndirectCommand) + sizeof(uint32_t)
            })
        .UseResource("IndirectBufferOpaque", AccessType::Write)
        .DeclareTransientResource("CullingData",
            vk::BufferDesc{
                .usage = vk::BufferUsageBits_Storage,
                .storage = vk::StorageType::HostVisible,
                // This buffer is fixed-size, so no change is needed.
                .size = sizeof(CullingData)
            })
        .UseResource("CullingData", AccessType::Write)
        .SetPriority(Render::internal::RenderPassBuilder::PassPriority::EarlySetup)
        .AddGenericPass("SceneCulling", [this](Render::internal::ExecutionContext& ctx) {
            ExecuteCullingPass(ctx);
        });


  const float grey = 125.0f / 255.0f;
  PassDeclarationInfo gPassInfo;
  gPassInfo.framebufferDebugName = "GridDraw";
  gPassInfo.colorAttachments[0] = {.textureName = RenderResources::SCENE_COLOR,
    .loadOp = vk::LoadOp::Clear,
    .storeOp = vk::StoreOp::Store,
    .clearColor = {grey, grey, grey, 1.0}};
  gPassInfo.depthAttachment = {.textureName = RenderResources::SCENE_DEPTH,
    .loadOp = vk::LoadOp::Clear,
    .storeOp = vk::StoreOp::Store,
    .clearDepth = 1.0f,};
  passBuilder.CreatePass().UseResource("ObjectTransforms",AccessType::Read).
              UseResource("DrawDataBuffer",AccessType::Read).
              UseResource("IndirectBufferOpaque",AccessType::Read).
              UseResource(RenderResources::VERTEX_BUFFER,AccessType::Read).
              UseResource(RenderResources::INDEX_BUFFER,AccessType::Read).
              UseResource(RenderResources::MATERIAL_BUFFER,AccessType::Read).
              UseResource(RenderResources::SCENE_COLOR,AccessType::Write).
              UseResource(RenderResources::SCENE_DEPTH,AccessType::Write).
              SetPriority(Render::internal::RenderPassBuilder::PassPriority::Opaque).
              AddGraphicsPass("SceneOpaque",
                gPassInfo,
                [this](Render::internal::ExecutionContext& ctx)
                {
                  ExecuteOpaquePass(ctx);
                });
}

void SceneRenderFeature::ExecuteCullingPass(Render::internal::ExecutionContext& ctx)
{
      const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());
    EnsureComputePipeline(ctx);

    const size_t objectCount = params.getObjectCount();
    const size_t drawCommandCount = params.getDrawCommandCount();
    const size_t opaqueCount = params.getOpaqueCount();

    if (objectCount == 0)
    {
        // Clear the indirect buffer's draw count to 0 if there are no objects
        // to prevent rendering stale data from a previous frame.
        auto indirectBuffer = ctx.GetBuffer("IndirectBufferOpaque");
        const uint32_t zero = 0;
        ctx.GetvkContext().upload(indirectBuffer, &zero, sizeof(uint32_t));
        return;
    }

    // --- Start of New Resizing Logic ---
    // Calculate required sizes for the current frame's data.
    const size_t requiredTransformsSize = objectCount * sizeof(mat4);
    const size_t requiredBoundsSize = objectCount * sizeof(BoundingBox);
    const size_t requiredDrawDataSize = drawCommandCount * sizeof(DrawData);
    const size_t requiredIndirectSize = sizeof(uint32_t) + opaqueCount * sizeof(DrawIndexedIndirectCommand);

    // Resize buffers if they are not large enough. The render graph handles the details.
    if (requiredTransformsSize > ctx.GetBufferSize("ObjectTransforms"))
    {
        ctx.ResizeBuffer("ObjectTransforms", requiredTransformsSize);
    }
    if (requiredBoundsSize > ctx.GetBufferSize("ObjectBounds"))
    {
        ctx.ResizeBuffer("ObjectBounds", requiredBoundsSize);
    }
    if (requiredDrawDataSize > ctx.GetBufferSize("DrawDataBuffer"))
    {
        ctx.ResizeBuffer("DrawDataBuffer", requiredDrawDataSize);
    }
    if (requiredIndirectSize > ctx.GetBufferSize("IndirectBufferOpaque"))
    {
        ctx.ResizeBuffer("IndirectBufferOpaque", requiredIndirectSize);
    }
    // --- End of New Resizing Logic ---

    auto& cmd = ctx.GetvkCommandBuffer();

    // The rest of the function remains the same, but now it operates on correctly-sized buffers.
    ctx.GetvkContext().upload(ctx.GetBuffer("ObjectTransforms"), params.objectTransforms.data(), requiredTransformsSize);
    ctx.GetvkContext().upload(ctx.GetBuffer("ObjectBounds"), params.objectBounds.data(), requiredBoundsSize);
    ctx.GetvkContext().upload(ctx.GetBuffer("DrawDataBuffer"), params.drawData.data(), requiredDrawDataSize);
    
    CullingData cullingData = {};
    cullingData.numMeshesToCull = static_cast<uint32_t>(opaqueCount);
    cullingData.numVisibleMeshes = 0;

    const Render::FrameData& frameData = ctx.GetFrameData();
    mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
    extractFrustumPlanes(viewProj, cullingData.frustumPlanes);
    extractFrustumCorners(viewProj, cullingData.frustumCorners);

    ctx.GetvkContext().upload(ctx.GetBuffer("CullingData"), &cullingData, sizeof(CullingData));

    auto indirectBuffer = ctx.GetBuffer("IndirectBufferOpaque");
    ctx.GetvkContext().upload(indirectBuffer, &opaqueCount, sizeof(uint32_t));

    if (opaqueCount > 0)
    {
        std::vector<DrawIndexedIndirectCommand> opaqueCommands;
        opaqueCommands.reserve(opaqueCount);
        for (uint32_t i = 0; i < opaqueCount; ++i)
        {
            uint32_t drawIndex = params.opaqueIndices[i];
            opaqueCommands.push_back(params.drawCommands[drawIndex]);
        }
        
        ctx.GetvkContext().upload(indirectBuffer,
            opaqueCommands.data(),
            opaqueCommands.size() * sizeof(DrawIndexedIndirectCommand),
            sizeof(uint32_t)); // Offset past the count
    }

    struct
    {
        uint64_t commands;
        uint64_t drawData;
        uint64_t AABBs;
        uint64_t frustum;
    } pushConstants = {
        .commands = ctx.GetvkContext().gpuAddress(indirectBuffer),
        .drawData = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
        .AABBs = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectBounds")),
        .frustum = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("CullingData"))
    };

    cmd.cmdBindComputePipeline(m_cullingPipeline);
    cmd.cmdPushConstants(pushConstants);

    uint32_t numThreadGroups = static_cast<uint32_t>((opaqueCount + 63) / 64);
    if (numThreadGroups > 0)
    {
       cmd.cmdDispatchThreadGroups({numThreadGroups});
    }
}

void SceneRenderFeature::ExecuteOpaquePass(Render::internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  EnsureRenderPipelines(ctx);
  if (params.getOpaqueCount() == 0)
  {
    return;
  }
  const Render::FrameData& frameData = ctx.GetFrameData();
  auto& cmd = ctx.GetvkCommandBuffer();
  cmd.cmdBindIndexBuffer(ctx.GetBuffer(RenderResources::INDEX_BUFFER),
    vk::IndexFormat::UI32);
  cmd.cmdBindVertexBuffer(0,ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
  cmd.cmdBindRenderPipeline(m_opaqueRenderPipeline);
  cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Less,
    .isDepthWriteEnabled = true});

  struct RenderConstants
  {
    mat4 viewProj;
    vec3 cameraPos;
    uint32_t _pad;
    uint64_t bufferTransforms;
    uint64_t bufferDrawData;
    uint64_t bufferMaterials;
  } pushConstants = {.viewProj = frameData.projMatrix * frameData.viewMatrix,
      .cameraPos = frameData.cameraPos,
      .bufferTransforms = ctx.GetvkContext().
                              gpuAddress(ctx.GetBuffer("ObjectTransforms")),
      .bufferDrawData = ctx.GetvkContext().
                            gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
      .bufferMaterials = ctx.GetvkContext().gpuAddress(
        ctx.GetBuffer(RenderResources::MATERIAL_BUFFER))};

  cmd.cmdPushConstants(pushConstants);
  cmd.cmdDrawIndexedIndirectCount(ctx.GetBuffer("IndirectBufferOpaque"),
    // indirectBuffer
    sizeof(uint32_t),
    // indirectBufferOffset (skip count)
    ctx.GetBuffer("IndirectBufferOpaque"),
    // countBuffer (same buffer)
    0,
    // countBufferOffset (count at start)
    params.getOpaqueCount(),
    // maxDrawCount
    sizeof(DrawIndexedIndirectCommand) // stride
  );
}

void SceneRenderFeature::extractFrustumPlanes(const mat4& viewProj,
                                              vec4* planes)
{
  // Extract 6 frustum planes from view-projection matrix
  // Left, Right, Bottom, Top, Near, Far
  const float* m = &viewProj[0][0];

  // Left plane
  planes[0] = vec4(m[3] + m[0],m[7] + m[4],m[11] + m[8],m[15] + m[12]);
  // Right plane  
  planes[1] = vec4(m[3] - m[0],m[7] - m[4],m[11] - m[8],m[15] - m[12]);
  // Bottom plane
  planes[2] = vec4(m[3] + m[1],m[7] + m[5],m[11] + m[9],m[15] + m[13]);
  // Top plane
  planes[3] = vec4(m[3] - m[1],m[7] - m[5],m[11] - m[9],m[15] - m[13]);
  // Near plane
  planes[4] = vec4(m[3] + m[2],m[7] + m[6],m[11] + m[10],m[15] + m[14]);
  // Far plane  
  planes[5] = vec4(m[3] - m[2],m[7] - m[6],m[11] - m[10],m[15] - m[14]);

  // Normalize planes
  for (int i = 0; i < 6; ++i)
  {
    float length = glm::length(vec3(planes[i]));
    planes[i] /= length;
  }
}

void SceneRenderFeature::extractFrustumCorners(const mat4& viewProj,
                                               vec4* corners)
{
  // Extract 8 frustum corners from inverse view-projection matrix
  mat4 invViewProj = inverse(viewProj);

  // NDC coordinates for 8 corners
  vec4 ndcCorners[8] = {{-1, -1, -1, 1},
    {1, -1, -1, 1},
    {-1, 1, -1, 1},
    {1, 1, -1, 1},
    // near
    {-1, -1, 1, 1},
    {1, -1, 1, 1},
    {-1, 1, 1, 1},
    {1, 1, 1, 1} // far
  };

  for (int i = 0; i < 8; ++i)
  {
    vec4 worldPos = invViewProj * ndcCorners[i];
    corners[i] = worldPos / worldPos.w;
  }
}

void SceneRenderFeature::EnsureComputePipeline(Render::internal::ExecutionContext& ctx)
{
  if (m_cullingPipeline.empty())
  {
    std::filesystem::path workingDir = std::filesystem::canonical(ST<Filepaths>::Get()->workingDir);
    std::filesystem::path shader_path = workingDir / "Shaders" / "culling.comp";
    m_cullingShader = loadShaderModule(ctx.GetvkContext(),
   shader_path.string().c_str());
    vk::ComputePipelineDesc desc = {.smComp = m_cullingShader,
      .debugName = "SceneCullingPipeline"};
    m_cullingPipeline = ctx.GetvkContext().createComputePipeline(desc);
    if (!m_cullingPipeline.valid())
    {
      throw std::runtime_error("Failed to create culling compute pipeline");
    }
  }
}

void SceneRenderFeature::EnsureRenderPipelines(Render::internal::ExecutionContext& ctx)
{
  if (m_opaqueRenderPipeline.valid())
  {
    return; // Already created
  }
  vk::TextureHandle color = ctx.GetTexture(RenderResources::SCENE_COLOR);
  vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);

  std::filesystem::path workingDir = std::filesystem::canonical(ST<Filepaths>::Get()->workingDir);
  std::filesystem::path vert_path = workingDir / "Shaders" / "opaque.vert";
  std::filesystem::path frag_path = workingDir / "Shaders" / "opaque.frag";

  vertShader = loadShaderModule(ctx.GetvkContext(), vert_path.string().c_str());
  fragShader = loadShaderModule(ctx.GetvkContext(), frag_path.string().c_str());

  // Define vertex input layout (matching your Vertex structure)
  vk::VertexInput vertexInput;
  vertexInput.attributes[0] = {.location = 0,
    .format = vk::VertexFormat::Float3,
    // position
    .offset = offsetof(Vertex,position)};
  vertexInput.attributes[1] = {.location = 1,
    .format = vk::VertexFormat::Float1,
    //normal
    .offset = offsetof(Vertex,uv_x)};
  vertexInput.attributes[2] = {.location = 2,
    .format = vk::VertexFormat::Float3,
    //uv
    .offset = offsetof(Vertex,normal)};
  vertexInput.attributes[3] = {.location = 3,
    .format = vk::VertexFormat::Float1,
    //tangent
    .offset = offsetof(Vertex,uv_y)};
  vertexInput.attributes[4] = {.location = 4,
    .format = vk::VertexFormat::Float4,
    //bitangent
    .offset = offsetof(Vertex,tangent)};

  vertexInput.inputBindings[0].stride = sizeof(Vertex);
  // Create render pipeline descriptor  
  vk::RenderPipelineDesc desc = {.vertexInput = vertexInput,
    .smVert = vertShader,
    .smFrag = fragShader,
    .color = {{.format = ctx.GetvkContext().getFormat(color),
      .blendEnabled = true,
      .srcRGBBlendFactor = vk::BlendFactor::SrcAlpha,
      .dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha,}},

    // Depth attachment (SCENE_DEPTH format) 
    .depthFormat = depth.valid() ? ctx.GetvkContext().getFormat(depth) :
                     vk::Format::Invalid,
    // Rasterization settings
    .cullMode = vk::CullMode::Back,
    // Back-face culling
    // MSAA settings (if using multisampling)
    .samplesCount = 1,
    // Adjust based on your MSAA settings
    .minSampleShading = 0.0f,
    .debugName = "SceneOpaquePipeline"};

  m_opaqueRenderPipeline = ctx.GetvkContext().createRenderPipeline(desc);
  if (!m_opaqueRenderPipeline.valid())
  {
    throw std::runtime_error("Failed to create opaque render pipeline");
  }
}

const char* SceneRenderFeature::GetName() const
{
  return "SceneRenderFeature";
}
