#include "scene_feature.h"
#include "renderer/animation_update.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "resource/resource_manager.h"
#include "renderer/gpu_data.h"
#include <algorithm>

SceneRenderFeature::SceneRenderFeature(bool enableObjectPicking)
  : m_enableObjectPicking(enableObjectPicking)
{
}

void SceneRenderFeature::UpdateScene(uint64_t renderFeatureID,
                                     Resource::Scene& gameScene,
                                     const Resource::ResourceManager&
                                     asset_system,
                                     Renderer& renderer)
{
  auto params = static_cast<SceneRenderParams*>(renderer.
                                                GetFeatureParameterBlockPtr(renderFeatureID));

  if(!params)
    return;

  params->clear();
  params->reserve(gameScene.objects.size());
  [[maybe_unused]] constexpr uint32_t INVALID_RANGE = SceneRenderParams::AnimatedInstanceParams::INVALID;
  uint32_t animatedVertexCursor = 0;

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
    float radius = meshData->bounds.w;

    // For meshes with morph targets, expand bounds to account for deformation
    const bool hasMorphTargets = meshData->animation.morphDeltaByteOffset != UINT32_MAX;
    if(hasMorphTargets)
    {
      constexpr float MORPH_BOUNDS_EXPANSION = 2.0f;
      radius *= MORPH_BOUNDS_EXPANSION;
    }

    // Extract scale from transform matrix to properly scale the bounding radius
    // Each basis vector's length gives the scale in that dimension
    vec3 scale = vec3(
      length(vec3(obj.transform[0])),
      length(vec3(obj.transform[1])),
      length(vec3(obj.transform[2]))
    );

    // Use maximum scale for spherical bounds to ensure full coverage
    float maxScale = std::max(std::max(scale.x, scale.y), scale.z);

    // Transform center and scale radius to world space
    auto worldCenter = vec3(obj.transform * vec4(center, 1.0f));
    float worldRadius = radius * maxScale;

    params->objectBounds.emplace_back(
      worldCenter - vec3(worldRadius),
      worldCenter + vec3(worldRadius));

    // Build draw command
    DrawIndexedIndirectCommand cmd = {
      .count = meshData->indexCount,
      .instanceCount = 1,
      .firstIndex = static_cast<uint32_t>(meshData->indexByteOffset / sizeof(
        uint32_t)),
      .baseVertex = static_cast<int32_t>(meshData->vertexByteOffset / sizeof(
        CompressedVertex)),
      .baseInstance = static_cast<uint32_t>(objectIndex) };

    DrawData drawData = {
      .transformId = static_cast<uint32_t>(objectIndex),
      .materialId = asset_system.getMaterialIndex(obj.material),
      .meshDecompIndex = static_cast<uint32_t>(meshData->decompressionByteOffset / sizeof(MeshDecompressionData)),
      .objectId = static_cast<uint32_t>(objectIndex) };

    uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.
                                                      size());
    params->drawCommands.push_back(cmd);
    params->drawData.push_back(drawData);

    const SceneObject::AnimBinding* animBinding = obj.anim.has_value() ? &obj.anim.value() : nullptr;
    const bool objectAnimated = animBinding &&
      ((animBinding->jointCount > 0 && !animBinding->skinMatrices.empty()) ||
       (animBinding->morphCount > 0 && !animBinding->morphWeights.empty()));

    params->drawIsAnimated.push_back(objectAnimated ? 1u : 0u);

    if(objectAnimated)
    {
      SceneRenderParams::AnimatedInstanceParams instance{};
      instance.drawIndex = drawCommandIndex;
      instance.srcBaseVertex = static_cast<uint32_t>(meshData->vertexByteOffset / sizeof(CompressedVertex));
      instance.dstBaseVertex = animatedVertexCursor;
      instance.vertexCount = meshData->vertexCount;
      instance.meshDecompIndex = drawData.meshDecompIndex;

      if(meshData->animation.skinningByteOffset != UINT32_MAX)
        instance.skinningOffset = meshData->animation.skinningByteOffset / sizeof(GPUSkinningData);

      if(animBinding && animBinding->jointCount > 0 && !animBinding->skinMatrices.empty())
      {
        instance.boneMatrixOffset = static_cast<uint32_t>(params->boneMatrices.size());
        instance.jointCount = animBinding->jointCount;
        params->boneMatrices.insert(params->boneMatrices.end(),
                                    animBinding->skinMatrices.begin(),
                                    animBinding->skinMatrices.begin() + animBinding->jointCount);
      }

      if(meshData->animation.morphDeltaByteOffset != UINT32_MAX)
        instance.morphDeltaOffset = meshData->animation.morphDeltaByteOffset / sizeof(GPUMorphDelta);
      instance.morphDeltaCount = meshData->animation.morphDeltaCount;

      if(meshData->animation.morphVertexBaseOffset != UINT32_MAX)
        instance.morphVertexBaseOffset = meshData->animation.morphVertexBaseOffset / sizeof(uint32_t);
      if(meshData->animation.morphVertexCountOffset != UINT32_MAX)
        instance.morphVertexCountOffset = meshData->animation.morphVertexCountOffset / sizeof(uint32_t);

      if(animBinding && animBinding->morphCount > 0 && !animBinding->morphWeights.empty())
      {
        instance.morphStateOffset = static_cast<uint32_t>(params->morphWeights.size());
        instance.morphTargetCount = animBinding->morphCount;
        params->morphWeights.insert(params->morphWeights.end(),
                                    animBinding->morphWeights.begin(),
                                    animBinding->morphWeights.begin() + animBinding->morphCount);
      }

      params->drawCommands[drawCommandIndex].baseVertex = static_cast<int32_t>(instance.dstBaseVertex);
      params->animatedInstances.push_back(instance);
      params->hasAnimatedInstances = true;
      animatedVertexCursor += instance.vertexCount;
    }

    if(asset_system.isMaterialTransparent(obj.material))
    {
      params->transparentIndices.push_back(drawCommandIndex);
      if(objectAnimated)
        ++params->transparentAnimatedCount;
      else
        ++params->transparentStaticCount;
    }
    else
    {
      params->opaqueIndices.push_back(drawCommandIndex);
      if(objectAnimated)
        ++params->opaqueAnimatedCount;
      else
        ++params->opaqueStaticCount;
    }
  }

  if(params->animatedInstances.empty())
  {
    params->boneMatrices.clear();
    params->morphWeights.clear();
    params->hasAnimatedInstances = false;
  }
  else
  {
    params->hasAnimatedInstances = true;
  }

  params->usedAnimatedVertexBytes = animatedVertexCursor * sizeof(SkinnedVertex);

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

  // NOTE: Shadow-casting point light selection is done in GraphicsAPI.cpp::UploadToPipeline
  // after lights are populated from the ECS. The shadowPointLightCount, pointLightShadows,
  // and shadowPointLightIndices are set there.
  
  /*LOG_INFO("SceneRenderFeature: Updated {} objects ({} opaque, {} transparent)",
         params->getObjectCount(),
         params->getOpaqueCount(),
         params->getTransparentCount());*/
}



void SceneRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  constexpr size_t kInitialObjectCapacity = 1024;

  const vk::BufferDesc animBoneDesc{
    .usage = vk::BufferUsageBits_Storage,
    .storage = vk::StorageType::HostVisible,
    .size = sizeof(mat4) * 16 };
  const vk::BufferDesc animMorphDesc{
    .usage = vk::BufferUsageBits_Storage,
    .storage = vk::StorageType::HostVisible,
    .size = sizeof(float) * 16 };
  const vk::BufferDesc animInstanceDesc{
    .usage = vk::BufferUsageBits_Storage,
    .storage = vk::StorageType::HostVisible,
    .size = sizeof(GPUAnimatedInstance) * 16 };
  const vk::BufferDesc skinnedVertexBufferDesc{
	.usage = vk::BufferUsageBits_Vertex | vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 1024 * 1024 };


  passBuilder.CreatePass()
    .UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read)
    .DeclareTransientResource(RenderResources::SKINNED_VERTEX_BUFFER, skinnedVertexBufferDesc)
    .UseResource(RenderResources::SKINNED_VERTEX_BUFFER, AccessType::ReadWrite)
    .UseResource(RenderResources::MESH_DECOMPRESSION_BUFFER, AccessType::Read)
    .UseResource(RenderResources::SKINNING_BUFFER, AccessType::Read)
    .UseResource(RenderResources::MORPH_DELTA_BUFFER, AccessType::Read)
    .UseResource(RenderResources::MORPH_VERTEX_BASE_BUFFER, AccessType::Read)
    .UseResource(RenderResources::MORPH_VERTEX_COUNT_BUFFER, AccessType::Read)
    .DeclareTransientResource(RenderResources::ANIM_BONE_BUFFER, animBoneDesc)
    .UseResource(RenderResources::ANIM_BONE_BUFFER, AccessType::Write)
    .DeclareTransientResource(RenderResources::ANIM_MORPH_BUFFER, animMorphDesc)
    .UseResource(RenderResources::ANIM_MORPH_BUFFER, AccessType::Write)
    .DeclareTransientResource(RenderResources::ANIM_INSTANCE_BUFFER, animInstanceDesc)
    .UseResource(RenderResources::ANIM_INSTANCE_BUFFER, AccessType::Write)
    .SetPriority(internal::RenderPassBuilder::PassPriority::EarlySetup)
    .AddGenericPass(
      "MeshDeformation",
      [this](internal::ExecutionContext& ctx)
  {
    ExecuteDeformationPass(ctx);
  });

  // Culling pass
  passBuilder.CreatePass().DeclareTransientResource(
    RenderResources::FRAME_CONSTANTS,
    vk::BufferDesc{
      .usage = vk::BufferUsageBits_Storage,
      .storage = vk::StorageType::HostVisible,
      .size = sizeof(FrameConstantsGPU) }).
    UseResource(RenderResources::FRAME_CONSTANTS, AccessType::Write).
    DeclareTransientResource(
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
      "IndirectBufferOpaqueStatic",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Indirect |
        vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = kInitialObjectCapacity * sizeof(DrawIndexedIndirectCommand) +
        sizeof(uint32_t) }).
    UseResource("IndirectBufferOpaqueStatic", AccessType::Write).
    DeclareTransientResource(
      "IndirectBufferOpaqueAnimated",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Indirect |
        vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = kInitialObjectCapacity * sizeof(DrawIndexedIndirectCommand) +
        sizeof(uint32_t) }).
    UseResource("IndirectBufferOpaqueAnimated", AccessType::Write).
    DeclareTransientResource(
      "IndirectBufferTransparentStatic",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Indirect |
        vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = kInitialObjectCapacity * sizeof(DrawIndexedIndirectCommand) +
        sizeof(uint32_t) }).
    UseResource("IndirectBufferTransparentStatic", AccessType::Write).
    DeclareTransientResource(
      "IndirectBufferTransparentAnimated",
      vk::BufferDesc{
        .usage = vk::BufferUsageBits_Indirect |
        vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = kInitialObjectCapacity * sizeof(DrawIndexedIndirectCommand) +
        sizeof(uint32_t) }).
    UseResource("IndirectBufferTransparentAnimated", AccessType::Write).
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

  passBuilder.CreatePass().UseResource(Lighting::CLUSTER_BOUNDS, AccessType::Write).
      UseResource(Lighting::LIGHTING_BUFFER, AccessType::Read).
      UseResource(RenderResources::SCENE_COLOR, AccessType::Read).SetPriority(
          internal::RenderPassBuilder::PassPriority::LightCulling).AddGenericPass(
              "ClusterBoundsGeneration", [this](internal::ExecutionContext& ctx)
              {
                  executeClusterBoundsGeneration(ctx);
              });
  // Light Culling Pass
  passBuilder.CreatePass().UseResource(Lighting::LIGHT_BUFFER, AccessType::Read).
      UseResource(Lighting::CLUSTER_BOUNDS, AccessType::Read).
      UseResource(Lighting::ITEM_LIST, AccessType::Write).
      UseResource(Lighting::CLUSTER_DATA, AccessType::Write).SetPriority(
          internal::RenderPassBuilder::PassPriority::LightCulling).AddGenericPass(
              "LightCulling", [this](internal::ExecutionContext& ctx)
              {
                  executeLightCulling(ctx);
              });
  // Calculate lighting buffer sizes
  uint32_t lightBufferSize = Parameters::MAX_LIGHTS * sizeof(Lighting::GPULight);
  uint32_t clusterBoundsSize = Lighting::TOTAL_CLUSTERS * sizeof(Lighting::ClusterBounds);
  uint32_t itemListSize = sizeof(uint32_t) + Lighting::MAX_TOTAL_ITEMS * sizeof(uint32_t);
  uint32_t clusterDataSize = Lighting::TOTAL_CLUSTERS * sizeof(Lighting::Cluster);
  uint32_t pointShadowBufferSize = Lighting::MAX_SHADOW_POINT_LIGHTS * sizeof(Lighting::GPUPointLightShadow);

  // Point light shadow cube map texture descriptors
  vk::TextureDesc shadowCubeColorDesc{
      .type = vk::TextureType::TexCube,
      .format = vk::Format::R_F16,  // Linear depth in R16F
      .dimensions = {Lighting::SHADOW_MAP_SIZE, Lighting::SHADOW_MAP_SIZE, 1},
      .numLayers = 6,
      .usage = vk::TextureUsageBits_Attachment | vk::TextureUsageBits_Sampled,
  };

  vk::TextureDesc shadowCubeDepthDesc{
      .type = vk::TextureType::TexCube,
      .format = vk::Format::Z_F32,
      .dimensions = {Lighting::SHADOW_MAP_SIZE, Lighting::SHADOW_MAP_SIZE, 1},
      .numLayers = 6,
      .usage = vk::TextureUsageBits_Attachment,
  };

  // Lighting Setup Pass - initialize lighting data and declare shadow cube maps
  // Shadow cube maps are declared here so LightingSetup can read their bindless indices
  passBuilder.CreatePass().UseResource(RenderResources::SCENE_COLOR, AccessType::Read).
      DeclareTransientResource(Lighting::LIGHT_BUFFER, vk::BufferDesc{
                                 .usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::HostVisible,
                                 .size = lightBufferSize
          }).DeclareTransientResource(Lighting::CLUSTER_BOUNDS, vk::BufferDesc{
                                        .usage = vk::BufferUsageBits_Storage,
                                        .storage = vk::StorageType::Device,
                                        .size = clusterBoundsSize
              }).DeclareTransientResource(
                  Lighting::ITEM_LIST,
                  vk::BufferDesc
                  { .usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = itemListSize }).
      DeclareTransientResource(Lighting::CLUSTER_DATA, vk::BufferDesc{
                                 .usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device,
                                 .size = clusterDataSize
          }).DeclareTransientResource(Lighting::LIGHTING_BUFFER, vk::BufferDesc{
                                        .usage = vk::BufferUsageBits_Storage,
                                        .storage = vk::StorageType::HostVisible,
                                        .size = sizeof(Lighting::LightingBuffer)
              }).DeclareTransientResource(Lighting::POINT_SHADOW_BUFFER, vk::BufferDesc{
                                        .usage = vk::BufferUsageBits_Storage,
                                        .storage = vk::StorageType::HostVisible,
                                        .size = pointShadowBufferSize
              })
              // Declare shadow cube map textures here so they're allocated before shadow passes
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_COLOR_0, shadowCubeColorDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_COLOR_1, shadowCubeColorDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_COLOR_2, shadowCubeColorDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_COLOR_3, shadowCubeColorDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_DEPTH_0, shadowCubeDepthDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_DEPTH_1, shadowCubeDepthDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_DEPTH_2, shadowCubeDepthDesc)
              .DeclareTransientResource(RenderResources::POINT_SHADOW_CUBE_DEPTH_3, shadowCubeDepthDesc)
              .UseResource(Lighting::LIGHT_BUFFER, AccessType::Write)
              .UseResource(Lighting::LIGHTING_BUFFER, AccessType::Write)
              .UseResource(Lighting::POINT_SHADOW_BUFFER, AccessType::Write)
              // Read shadow cube textures to get their bindless indices for shadowMapIndex
              .UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_0, AccessType::Read)
              .UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_1, AccessType::Read)
              .UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_2, AccessType::Read)
              .UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_3, AccessType::Read)
              .SetPriority(internal::RenderPassBuilder::PassPriority::EarlySetup)
              .AddGenericPass(
                  "LightingSetup", [this](internal::ExecutionContext& ctx)
                  {
                      executeLightingSetup(ctx);
                  });

  // Shadow cube map resource names (must match RenderResources constants)
  const char* shadowColorNames[Lighting::MAX_SHADOW_POINT_LIGHTS] = {
      RenderResources::POINT_SHADOW_CUBE_COLOR_0,
      RenderResources::POINT_SHADOW_CUBE_COLOR_1,
      RenderResources::POINT_SHADOW_CUBE_COLOR_2,
      RenderResources::POINT_SHADOW_CUBE_COLOR_3,
  };
  const char* shadowDepthNames[Lighting::MAX_SHADOW_POINT_LIGHTS] = {
      RenderResources::POINT_SHADOW_CUBE_DEPTH_0,
      RenderResources::POINT_SHADOW_CUBE_DEPTH_1,
      RenderResources::POINT_SHADOW_CUBE_DEPTH_2,
      RenderResources::POINT_SHADOW_CUBE_DEPTH_3,
  };

  // Create shadow passes for each potential shadow-casting point light
  for (uint32_t i = 0; i < Lighting::MAX_SHADOW_POINT_LIGHTS; i++)
  {
      PassDeclarationInfo shadowPassInfo;
      shadowPassInfo.framebufferDebugName = "PointShadowPass";
      shadowPassInfo.colorAttachments[0] = {
          .textureName = shadowColorNames[i],
          .loadOp = vk::LoadOp::Clear,
          .storeOp = vk::StoreOp::Store,
          .clearColor = {1000.0f, 1000.0f, 1000.0f, 1000.0f},  // Large value = no occluder
      };
      shadowPassInfo.depthAttachment = {
          .textureName = shadowDepthNames[i],
          .loadOp = vk::LoadOp::Clear,
          .storeOp = vk::StoreOp::DontCare,
          .clearDepth = 1.0f,
      };
      // Enable multiview rendering - all 6 cube faces in one draw
      shadowPassInfo.layerCount = 6;
      shadowPassInfo.viewMask = 0b111111;  // All 6 views active

      passBuilder.CreatePass()
          // Resources already declared in LightingSetup, just use them
          .UseResource(shadowColorNames[i], AccessType::Write)
          .UseResource(shadowDepthNames[i], AccessType::Write)
          .UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read)
          .UseResource(RenderResources::SKINNED_VERTEX_BUFFER, AccessType::Read)
          .UseResource(RenderResources::INDEX_BUFFER, AccessType::Read)
          .UseResource("ObjectTransforms", AccessType::Read)
          .UseResource("DrawDataBuffer", AccessType::Read)
          .UseResource(RenderResources::MATERIAL_BUFFER, AccessType::Read)
          .UseResource("IndirectBufferOpaqueStatic", AccessType::Read)
          .UseResource("IndirectBufferOpaqueAnimated", AccessType::Read)
          .UseResource("IndirectBufferTransparentStatic", AccessType::Read)
          .UseResource("IndirectBufferTransparentAnimated", AccessType::Read)
          .UseResource(RenderResources::MESH_DECOMPRESSION_BUFFER, AccessType::Read)
          .UseResource(Lighting::POINT_SHADOW_BUFFER, AccessType::Read)
          .SetPriority(internal::RenderPassBuilder::PassPriority::ShadowMap)
          .ExecuteAfter("LightingSetup")
          .ExecuteAfter("MeshDeformation")
          .AddGraphicsPass(
              (std::string("PointShadowPass") + std::to_string(i)).c_str(),
              shadowPassInfo,
              [this, i](internal::ExecutionContext& ctx) {
                  ExecutePointShadowPass(ctx, i);
              });
  }

  {
    PassDeclarationInfo depthPrepassInfo;
    depthPrepassInfo.framebufferDebugName = "DepthPrepass";
    depthPrepassInfo.depthAttachment = {
        .textureName = RenderResources::SCENE_DEPTH,
        .loadOp = vk::LoadOp::Clear,
        .storeOp = vk::StoreOp::Store,
        .clearDepth = 1.0f
    };
    depthPrepassInfo.colorAttachments[0] = {
        .textureName = "VisibilityBuffer",
        .loadOp = vk::LoadOp::Clear,
        .storeOp = vk::StoreOp::Store
    };

    passBuilder.CreatePass()
        .DeclareTransientResource(
            "VisibilityBuffer",
            vk::TextureDesc{
                .type = vk::TextureType::Tex2D,
                .format = vk::Format::R_UI32,
                // Use fixed internal resolution to avoid recompilation on window resize
                .dimensions = ResourceProperties::INTERNAL_RESOLUTION_DIMENSIONS,
                .usage = vk::TextureUsageBits_Attachment | vk::TextureUsageBits_Sampled
            })
        .UseResource("ObjectTransforms", AccessType::Read)
        .UseResource("DrawDataBuffer", AccessType::Read)
        .UseResource("IndirectBufferOpaqueStatic", AccessType::Read)
        .UseResource("IndirectBufferOpaqueAnimated", AccessType::Read)
        .UseResource(RenderResources::FRAME_CONSTANTS, AccessType::Read)
        .UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read)
        .UseResource(RenderResources::SKINNED_VERTEX_BUFFER, AccessType::Read)
        .UseResource(RenderResources::INDEX_BUFFER, AccessType::Read)
        .UseResource(RenderResources::MESH_DECOMPRESSION_BUFFER, AccessType::Read)
        .UseResource(RenderResources::SCENE_DEPTH, AccessType::Write)
        .UseResource("VisibilityBuffer", AccessType::Write)
        .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(
            static_cast<int>(internal::RenderPassBuilder::PassPriority::Opaque) - 1))
        .ExecuteAfter("SceneCulling")
        .AddGraphicsPass(
            "DepthPrepass",
            depthPrepassInfo,
            [this](internal::ExecutionContext& ctx) {
                ExecuteDepthPrepass(ctx);
            });
  }

  // OBJECT PICKING READBACK PASS - only registered if object picking is enabled
  if (m_enableObjectPicking)
  {
    passBuilder.CreatePass()
        .UseResource("VisibilityBuffer", AccessType::Read)
        .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(
            static_cast<int>(internal::RenderPassBuilder::PassPriority::Opaque) - 1))
        .ExecuteAfter("DepthPrepass")
        .AddGenericPass(
            "ObjectPickingReadback",
            [this](internal::ExecutionContext& ctx) {
                ProcessPendingPick(ctx);
            });
  }

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
    UseResource("IndirectBufferOpaqueStatic", AccessType::Read).
    UseResource("IndirectBufferOpaqueAnimated", AccessType::Read).
    UseResource(RenderResources::FRAME_CONSTANTS, AccessType::Read).
    UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::SKINNED_VERTEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::INDEX_BUFFER, AccessType::Read).
    UseResource(RenderResources::MATERIAL_BUFFER, AccessType::Read).
    UseResource(RenderResources::MESH_DECOMPRESSION_BUFFER, AccessType::Read).
    UseResource(Lighting::LIGHTING_BUFFER, AccessType::Read).
    UseResource(RenderResources::SCENE_COLOR, AccessType::Write).
    UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite).
    UseResource("VisibilityBuffer", AccessType::Read).
    // Point light shadow cube maps - creates dependency so shadow passes aren't culled
    UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_0, AccessType::Read).
    UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_1, AccessType::Read).
    UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_2, AccessType::Read).
    UseResource(RenderResources::POINT_SHADOW_CUBE_COLOR_3, AccessType::Read).
    SetPriority(internal::RenderPassBuilder::PassPriority::Opaque).
    ExecuteAfter("DepthPrepass").
    AddGraphicsPass(
      "SceneOpaque",
      opaquePass,
      [this](internal::ExecutionContext& ctx)
  {
    ExecuteOpaquePass(ctx);
  });
  passBuilder.CreatePass().UseResource("ObjectTransforms", AccessType::Read)
    .UseResource("DrawDataBuffer", AccessType::Read)
    .UseResource("IndirectBufferTransparentStatic", AccessType::Read)
    .UseResource("IndirectBufferTransparentAnimated", AccessType::Read)
    .UseResource(RenderResources::FRAME_CONSTANTS, AccessType::Read)
    .UseResource(RenderResources::VERTEX_BUFFER, AccessType::Read)
    .UseResource(RenderResources::SKINNED_VERTEX_BUFFER, AccessType::Read)
    .UseResource(RenderResources::INDEX_BUFFER, AccessType::Read)
    .UseResource(RenderResources::MATERIAL_BUFFER, AccessType::Read)
    .UseResource(RenderResources::MESH_DECOMPRESSION_BUFFER, AccessType::Read)
    .UseResource(Lighting::LIGHTING_BUFFER, AccessType::Read)
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Read)
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
      .loadOp = vk::LoadOp::Clear,        // Clear color here
      .storeOp = vk::StoreOp::Store
  };
  skyboxPassInfo.depthAttachment = {
      .textureName = RenderResources::SCENE_DEPTH,
      .loadOp = vk::LoadOp::Clear,        // Clear depth here (before depth prepass)
      .storeOp = vk::StoreOp::Store,
      .clearDepth = 1.0f
  };
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::Write)
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Write) // Write depth
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(static_cast<int>(internal::RenderPassBuilder::PassPriority::Opaque) - 2)) // Before depth prepass
    .AddGraphicsPass(
      "Skybox",
      skyboxPassInfo,
      [this](internal::ExecutionContext& ctx) {
    ExecuteSkyboxPass(ctx);
  });

};

void SceneRenderFeature::ExecuteDeformationPass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());

  if(params.animatedInstances.empty() || params.usedAnimatedVertexBytes == 0)
    return;

  auto& cmd = ctx.GetvkCommandBuffer();

  const size_t requiredBytes = params.usedAnimatedVertexBytes;
  if(ctx.GetBufferSize(RenderResources::SKINNED_VERTEX_BUFFER) < requiredBytes)
    ctx.ResizeBuffer(RenderResources::SKINNED_VERTEX_BUFFER, requiredBytes);

  EnsureDeformationPipeline(ctx);

  const size_t boneMatrixSize = params.boneMatrices.size() * sizeof(mat4);
  if(boneMatrixSize > 0)
  {
    if(boneMatrixSize > ctx.GetBufferSize(RenderResources::ANIM_BONE_BUFFER))
      ctx.ResizeBuffer(RenderResources::ANIM_BONE_BUFFER, boneMatrixSize);

    ctx.GetvkContext().upload(
      ctx.GetBuffer(RenderResources::ANIM_BONE_BUFFER),
      params.boneMatrices.data(),
      boneMatrixSize);
  }

  const size_t morphWeightSize = params.morphWeights.size() * sizeof(float);
  if(morphWeightSize > 0)
  {
    if(morphWeightSize > ctx.GetBufferSize(RenderResources::ANIM_MORPH_BUFFER))
      ctx.ResizeBuffer(RenderResources::ANIM_MORPH_BUFFER, morphWeightSize);

    ctx.GetvkContext().upload(
      ctx.GetBuffer(RenderResources::ANIM_MORPH_BUFFER),
      params.morphWeights.data(),
      morphWeightSize);
  }

  std::vector<GPUAnimatedInstance> gpuInstances;
  gpuInstances.reserve(params.animatedInstances.size());
  for(const auto& instance : params.animatedInstances)
  {
    if(instance.vertexCount == 0)
      continue;

    GPUAnimatedInstance gpu{
      .baseInfo = glm::uvec4(instance.srcBaseVertex,
                             instance.dstBaseVertex,
                             instance.vertexCount,
                             instance.meshDecompIndex),
      .skinningInfo = glm::uvec4(instance.skinningOffset,
                                 instance.boneMatrixOffset,
                                 instance.jointCount,
                                 instance.morphStateOffset),
      .morphInfo = glm::uvec4(instance.morphDeltaOffset,
                              instance.morphVertexBaseOffset,
                              instance.morphVertexCountOffset,
                              packMorphCounts(instance.morphTargetCount, instance.morphDeltaCount)) };
    gpuInstances.push_back(gpu);
  }

  if(gpuInstances.empty())
    return;

  const size_t instanceBufferSize = gpuInstances.size() * sizeof(GPUAnimatedInstance);
  if(instanceBufferSize > ctx.GetBufferSize(RenderResources::ANIM_INSTANCE_BUFFER))
    ctx.ResizeBuffer(RenderResources::ANIM_INSTANCE_BUFFER, instanceBufferSize);

  ctx.GetvkContext().upload(
    ctx.GetBuffer(RenderResources::ANIM_INSTANCE_BUFFER),
    gpuInstances.data(),
    instanceBufferSize);

  struct DeformationPushConstants
  {
    uint64_t bindPose;
    uint64_t skinned;
    uint64_t skinning;
    uint64_t morphDelta;
    uint64_t morphBase;
    uint64_t morphCount;
    uint64_t morphWeights;
    uint64_t boneMatrices;
    uint64_t instanceBuffer;
    uint64_t meshDecomp;
    uint32_t instanceCount;
    uint32_t padding;
  } pushConstants = {
    .bindPose = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::VERTEX_BUFFER)),
    .skinned = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::SKINNED_VERTEX_BUFFER)),
    .skinning = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::SKINNING_BUFFER)),
    .morphDelta = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MORPH_DELTA_BUFFER)),
    .morphBase = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MORPH_VERTEX_BASE_BUFFER)),
    .morphCount = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MORPH_VERTEX_COUNT_BUFFER)),
    .morphWeights = morphWeightSize > 0 ? ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::ANIM_MORPH_BUFFER)) : 0ull,
    .boneMatrices = boneMatrixSize > 0 ? ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::ANIM_BONE_BUFFER)) : 0ull,
    .instanceBuffer = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::ANIM_INSTANCE_BUFFER)),
    .meshDecomp = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MESH_DECOMPRESSION_BUFFER)),
    .instanceCount = static_cast<uint32_t>(gpuInstances.size()),
    .padding = 0u
  };

  cmd.cmdBindComputePipeline(m_deformationPipeline);
  cmd.cmdPushConstants(pushConstants);
  cmd.cmdDispatchThreadGroups({ static_cast<uint32_t>(gpuInstances.size()), 1, 1 });
}

void SceneRenderFeature::ExecuteCullingPass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  EnsureComputePipeline(ctx);

  const size_t objectCount = params.getObjectCount();
  const size_t drawCommandCount = params.getDrawCommandCount();
  const uint32_t opaqueStaticCount = params.opaqueStaticCount;
  const uint32_t opaqueAnimatedCount = params.opaqueAnimatedCount;
  const uint32_t transparentStaticCount = params.transparentStaticCount;
  const uint32_t transparentAnimatedCount = params.transparentAnimatedCount;
  const uint32_t opaqueCount = opaqueStaticCount + opaqueAnimatedCount;
  const uint32_t transparentCount = transparentStaticCount + transparentAnimatedCount;

  auto zeroIndirectBuffer = [&](const char* resourceName)
  {
    const uint32_t zero = 0;
    ctx.GetvkContext().upload(ctx.GetBuffer(resourceName), &zero, sizeof(uint32_t));
  };

  if(objectCount == 0)
  {
    zeroIndirectBuffer("IndirectBufferOpaqueStatic");
    zeroIndirectBuffer("IndirectBufferOpaqueAnimated");
    zeroIndirectBuffer("IndirectBufferTransparentStatic");
    zeroIndirectBuffer("IndirectBufferTransparentAnimated");
    return;
  }

  const size_t requiredTransformsSize = objectCount * sizeof(mat4);
  const size_t requiredBoundsSize = objectCount * sizeof(BoundingBox);
  const size_t requiredDrawDataSize = drawCommandCount * sizeof(DrawData);

  auto ensureIndirectCapacity = [&](const char* resourceName, uint32_t count)
  {
    const size_t requiredSize = sizeof(uint32_t) +
      static_cast<size_t>(count) * sizeof(DrawIndexedIndirectCommand);
    if(requiredSize > ctx.GetBufferSize(resourceName))
      ctx.ResizeBuffer(resourceName, requiredSize);
  };

  ensureIndirectCapacity("IndirectBufferOpaqueStatic", opaqueStaticCount);
  ensureIndirectCapacity("IndirectBufferOpaqueAnimated", opaqueAnimatedCount);
  ensureIndirectCapacity("IndirectBufferTransparentStatic", transparentStaticCount);
  ensureIndirectCapacity("IndirectBufferTransparentAnimated", transparentAnimatedCount);

  // Resize buffers if they are not large enough. The render graph handles the details.
  if(requiredTransformsSize > ctx.GetBufferSize("ObjectTransforms"))
    ctx.ResizeBuffer("ObjectTransforms", requiredTransformsSize);
  if(requiredBoundsSize > ctx.GetBufferSize("ObjectBounds"))
    ctx.ResizeBuffer("ObjectBounds", requiredBoundsSize);
  if(requiredDrawDataSize > ctx.GetBufferSize("DrawDataBuffer"))
    ctx.ResizeBuffer("DrawDataBuffer", requiredDrawDataSize);

  auto& cmd = ctx.GetvkCommandBuffer();

  // Upload frame constants
  const FrameData& frameData = ctx.GetFrameData();
  FrameConstantsGPU frameConstants = {
    .viewProj = frameData.projMatrix * frameData.viewMatrix,
    .cameraPos = vec4(frameData.cameraPos, 1.0f),
    .screenDims = vec2(frameData.screenWidth, frameData.screenHeight),
    .zNear = frameData.zNear,
    .zFar = frameData.zFar
  };
  ctx.GetvkContext().upload(
    ctx.GetBuffer(RenderResources::FRAME_CONSTANTS),
    &frameConstants,
    sizeof(FrameConstantsGPU));

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

  auto uploadIndirectCommands = [&](const char* resourceName,
                                    const std::vector<DrawIndexedIndirectCommand>& commands)
  {
    vk::BufferHandle buffer = ctx.GetBuffer(resourceName);
    const uint32_t count = static_cast<uint32_t>(commands.size());
    ctx.GetvkContext().upload(buffer, &count, sizeof(uint32_t));
    if(count > 0)
    {
      ctx.GetvkContext().upload(
        buffer,
        commands.data(),
        commands.size() * sizeof(DrawIndexedIndirectCommand),
        sizeof(uint32_t));
    }
  };

  std::vector<DrawIndexedIndirectCommand> opaqueStaticCommands;
  std::vector<DrawIndexedIndirectCommand> opaqueAnimatedCommands;
  opaqueStaticCommands.reserve(opaqueStaticCount);
  opaqueAnimatedCommands.reserve(opaqueAnimatedCount);

  for(uint32_t i = 0; i < opaqueCount; ++i)
  {
    const uint32_t drawIndex = params.opaqueIndices[i];
    const DrawIndexedIndirectCommand& drawCmd = params.drawCommands[drawIndex];
    if(params.drawIsAnimated[drawIndex] != 0)
      opaqueAnimatedCommands.push_back(drawCmd);
    else
      opaqueStaticCommands.push_back(drawCmd);
  }

  std::vector<DrawIndexedIndirectCommand> transparentStaticCommands;
  std::vector<DrawIndexedIndirectCommand> transparentAnimatedCommands;
  transparentStaticCommands.reserve(transparentStaticCount);
  transparentAnimatedCommands.reserve(transparentAnimatedCount);

  for(uint32_t i = 0; i < transparentCount; ++i)
  {
    const uint32_t drawIndex = params.transparentIndices[i];
    const DrawIndexedIndirectCommand& drawCmd = params.drawCommands[drawIndex];
    if(params.drawIsAnimated[drawIndex] != 0)
      transparentAnimatedCommands.push_back(drawCmd);
    else
      transparentStaticCommands.push_back(drawCmd);
  }

  uploadIndirectCommands("IndirectBufferOpaqueStatic", opaqueStaticCommands);
  uploadIndirectCommands("IndirectBufferOpaqueAnimated", opaqueAnimatedCommands);
  uploadIndirectCommands("IndirectBufferTransparentStatic", transparentStaticCommands);
  uploadIndirectCommands("IndirectBufferTransparentAnimated", transparentAnimatedCommands);

  CullingData cullingData = {};
  mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
  getFrustumPlanes(viewProj, cullingData.frustumPlanes);
  getFrustumCorners(viewProj, cullingData.frustumCorners);

  const uint64_t drawDataAddress = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("DrawDataBuffer"));
  const uint64_t boundsAddress = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectBounds"));

  cmd.cmdBindComputePipeline(m_cullingPipeline);

  auto dispatchCulling = [&](const char* resourceName, uint32_t commandCount)
  {
    if(commandCount == 0)
      return;

    cullingData.numMeshesToCull = commandCount;
    cullingData.numVisibleMeshes = 0;
    ctx.GetvkContext().upload(ctx.GetBuffer("CullingData"), &cullingData, sizeof(CullingData));
    const uint64_t cullingAddress = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("CullingData"));

    struct
    {
      uint64_t commands;
      uint64_t drawData;
      uint64_t AABBs;
      uint64_t frustum;
    } pushConstants = {
      .commands = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(resourceName)),
      .drawData = drawDataAddress,
      .AABBs = boundsAddress,
      .frustum = cullingAddress
    };

    cmd.cmdPushConstants(pushConstants);
    const uint32_t numThreadGroups = (commandCount + 63) / 64;
    cmd.cmdDispatchThreadGroups({ numThreadGroups });
  };

  dispatchCulling("IndirectBufferOpaqueStatic", opaqueStaticCount);
  dispatchCulling("IndirectBufferOpaqueAnimated", opaqueAnimatedCount);
  dispatchCulling("IndirectBufferTransparentStatic", transparentStaticCount);
  dispatchCulling("IndirectBufferTransparentAnimated", transparentAnimatedCount);

}

void SceneRenderFeature::ExecuteDepthPrepass(internal::ExecutionContext& ctx)
{
    const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());

    EnsureDepthPrepassPipeline(ctx);

    if (!m_depthPrepassPipeline.valid())
        return;

    const uint32_t staticCount = params.opaqueStaticCount;
    const uint32_t animatedCount = params.opaqueAnimatedCount;
    if(staticCount == 0 && animatedCount == 0)
        return;
    if(animatedCount > 0 && !m_depthPrepassAnimatedPipeline.valid())
        return;

    auto& cmd = ctx.GetvkCommandBuffer();

    cmd.cmdBindIndexBuffer(ctx.GetBuffer(RenderResources::INDEX_BUFFER), vk::IndexFormat::UI32);
    cmd.cmdBindRenderPipeline(m_depthPrepassPipeline);
    cmd.cmdBindDepthState({
        .compareOp = vk::CompareOp::Less,
        .isDepthWriteEnabled = true
    });

    PushConstants pushConstants = {
        .frameConstants = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::FRAME_CONSTANTS)),
        .bufferTransforms = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectTransforms")),
        .bufferDrawData = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
        .bufferMaterials = 0,  // Not used in depth prepass
        .meshDecomp = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MESH_DECOMPRESSION_BUFFER)),
        .oitBuffer = 0,
        .lightingBuffer = 0,
        .textureIndices = 0
    };

    if(staticCount > 0)
    {
      cmd.cmdPushConstants(pushConstants);
      cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
      cmd.cmdDrawIndexedIndirectCount(
        ctx.GetBuffer("IndirectBufferOpaqueStatic"),
        sizeof(uint32_t),
        ctx.GetBuffer("IndirectBufferOpaqueStatic"),
        0,
        static_cast<uint32_t>(staticCount),
        sizeof(DrawIndexedIndirectCommand));
    }

    if(animatedCount > 0)
    {
      cmd.cmdBindRenderPipeline(m_depthPrepassAnimatedPipeline);
      cmd.cmdPushConstants(pushConstants);
      cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::SKINNED_VERTEX_BUFFER));
      cmd.cmdDrawIndexedIndirectCount(
        ctx.GetBuffer("IndirectBufferOpaqueAnimated"),
        sizeof(uint32_t),
        ctx.GetBuffer("IndirectBufferOpaqueAnimated"),
        0,
        static_cast<uint32_t>(animatedCount),
        sizeof(DrawIndexedIndirectCommand));
    }
}

void SceneRenderFeature::ExecuteOpaquePass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  EnsureRenderPipelines(ctx);
  if(params.getOpaqueCount() == 0)
    return;

  const uint32_t staticCount = params.opaqueStaticCount;
  const uint32_t animatedCount = params.opaqueAnimatedCount;

  auto& cmd = ctx.GetvkCommandBuffer();

  cmd.cmdBindIndexBuffer(
    ctx.GetBuffer(RenderResources::INDEX_BUFFER),
    vk::IndexFormat::UI32);
  cmd.cmdBindDepthState(
    { .compareOp = vk::CompareOp::LessEqual, .isDepthWriteEnabled = true });

  PushConstants pushConstants = {
    .frameConstants = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::FRAME_CONSTANTS)),
    .bufferTransforms = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectTransforms")),
    .bufferDrawData = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
    .bufferMaterials = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MATERIAL_BUFFER)),
    .meshDecomp = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MESH_DECOMPRESSION_BUFFER)),
    .oitBuffer = 0,  // Not used in opaque pass
    .lightingBuffer = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(Lighting::LIGHTING_BUFFER)),
    .textureIndices = 0  // Reserved for future use
  };

  if(staticCount > 0)
  {
    cmd.cmdBindRenderPipeline(m_opaqueRenderPipeline);
    cmd.cmdPushConstants(pushConstants);
    cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
    cmd.cmdDrawIndexedIndirectCount(
      ctx.GetBuffer("IndirectBufferOpaqueStatic"),
      sizeof(uint32_t),
      ctx.GetBuffer("IndirectBufferOpaqueStatic"),
      0,
      staticCount,
      sizeof(DrawIndexedIndirectCommand));
  }

  if(animatedCount > 0)
  {
    if(!m_opaqueAnimatedRenderPipeline.valid())
      return;
    cmd.cmdBindRenderPipeline(m_opaqueAnimatedRenderPipeline);
    cmd.cmdPushConstants(pushConstants);
    cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::SKINNED_VERTEX_BUFFER));
    cmd.cmdDrawIndexedIndirectCount(
      ctx.GetBuffer("IndirectBufferOpaqueAnimated"),
      sizeof(uint32_t),
      ctx.GetBuffer("IndirectBufferOpaqueAnimated"),
      0,
      animatedCount,
      sizeof(DrawIndexedIndirectCommand));
  }
}

void SceneRenderFeature::ExecuteTransparentPass(internal::ExecutionContext& ctx)
{
  const auto& params = *static_cast<const SceneRenderParams*>(
    GetParameterBlock_RT());
  if(params.getTransparentCount() == 0)
    return;

  const uint32_t staticCount = params.transparentStaticCount;
  const uint32_t animatedCount = params.transparentAnimatedCount;

  auto& cmd = ctx.GetvkCommandBuffer();

  cmd.cmdBindIndexBuffer(
    ctx.GetBuffer(RenderResources::INDEX_BUFFER),
    vk::IndexFormat::UI32);

  cmd.cmdBindDepthState(
    { .compareOp = vk::CompareOp::Less, .isDepthWriteEnabled = false });

  // Same structure as opaque - OIT buffer contains all metadata
  PushConstants pushConstants = {
    .frameConstants = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::FRAME_CONSTANTS)),
    .bufferTransforms = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectTransforms")),
    .bufferDrawData = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
    .bufferMaterials = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MATERIAL_BUFFER)),
    .meshDecomp = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MESH_DECOMPRESSION_BUFFER)),
    .oitBuffer = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(OITResources::OIT_BUFFER)),
    .lightingBuffer = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(Lighting::LIGHTING_BUFFER)),
    .textureIndices = 0  // Reserved for future use
  };

  if(staticCount > 0)
  {
    cmd.cmdBindRenderPipeline(m_transparentRenderPipeline);
    cmd.cmdPushConstants(pushConstants);
    cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
    cmd.cmdDrawIndexedIndirectCount(
      ctx.GetBuffer("IndirectBufferTransparentStatic"),
      sizeof(uint32_t),
      ctx.GetBuffer("IndirectBufferTransparentStatic"),
      0,
      staticCount,
      sizeof(DrawIndexedIndirectCommand));
  }

  if(animatedCount > 0)
  {
    if(!m_transparentAnimatedRenderPipeline.valid())
      return;
    cmd.cmdBindRenderPipeline(m_transparentAnimatedRenderPipeline);
    cmd.cmdPushConstants(pushConstants);
    cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::SKINNED_VERTEX_BUFFER));
    cmd.cmdDrawIndexedIndirectCount(
      ctx.GetBuffer("IndirectBufferTransparentAnimated"),
      sizeof(uint32_t),
      ctx.GetBuffer("IndirectBufferTransparentAnimated"),
      0,
      animatedCount,
      sizeof(DrawIndexedIndirectCommand));
  }
}

void SceneRenderFeature::executeLightingSetup(internal::ExecutionContext& ctx)
{
    const auto& frameData = ctx.GetFrameData();
    auto& params = *const_cast<SceneRenderParams*>(static_cast<const SceneRenderParams*>(GetParameterBlock_RT()));
    // Get all the lighting buffers
    vk::BufferHandle lightBuffer = ctx.GetBuffer(Lighting::LIGHT_BUFFER);
    vk::BufferHandle clusterBounds = ctx.GetBuffer(Lighting::CLUSTER_BOUNDS);
    vk::BufferHandle itemList = ctx.GetBuffer(Lighting::ITEM_LIST);
    vk::BufferHandle clusterData = ctx.GetBuffer(Lighting::CLUSTER_DATA);
    vk::BufferHandle lightingBuffer = ctx.GetBuffer(Lighting::LIGHTING_BUFFER);
    vk::BufferHandle pointShadowBuffer = ctx.GetBuffer(Lighting::POINT_SHADOW_BUFFER);

    if (params.activeLightCount > 0)
    {
        void* mapped = ctx.GetvkContext().getMappedPtr(lightBuffer);
        std::memcpy(mapped, params.lights.data(), params.activeLightCount * sizeof(Lighting::GPULight));
        ctx.GetvkContext().flushMappedMemory(lightBuffer, 0, params.activeLightCount * sizeof(Lighting::GPULight));
    }

    // Upload point light shadow data - first update shadowMapIndex with actual bindless indices
    if (params.shadowPointLightCount > 0)
    {
        // Shadow cube map resource names
        const char* shadowColorNames[Lighting::MAX_SHADOW_POINT_LIGHTS] = {
            RenderResources::POINT_SHADOW_CUBE_COLOR_0,
            RenderResources::POINT_SHADOW_CUBE_COLOR_1,
            RenderResources::POINT_SHADOW_CUBE_COLOR_2,
            RenderResources::POINT_SHADOW_CUBE_COLOR_3,
        };

        // Update shadowMapIndex with actual bindless texture indices
        for (uint32_t i = 0; i < params.shadowPointLightCount; i++)
        {
            vk::TextureHandle shadowTex = ctx.GetTexture(shadowColorNames[i]);
            if (shadowTex.valid())
            {
                params.pointLightShadows[i].shadowMapIndex = shadowTex.index();
            }
        }

        void* mapped = ctx.GetvkContext().getMappedPtr(pointShadowBuffer);
        std::memcpy(mapped, params.pointLightShadows.data(), 
                    params.shadowPointLightCount * sizeof(Lighting::GPUPointLightShadow));
        ctx.GetvkContext().flushMappedMemory(pointShadowBuffer, 0, 
                    params.shadowPointLightCount * sizeof(Lighting::GPUPointLightShadow));
    }

    // Create lighting metadata buffer
    Lighting::LightingBuffer lightingData = {
      .bufferLights = ctx.GetvkContext().gpuAddress(lightBuffer),
      .bufferClusterBounds = ctx.GetvkContext().gpuAddress(clusterBounds),
      .bufferItemList = ctx.GetvkContext().gpuAddress(itemList),
      .bufferClusters = ctx.GetvkContext().gpuAddress(clusterData),
      .bufferPointShadows = ctx.GetvkContext().gpuAddress(pointShadowBuffer),
      .screenDims = vec2(static_cast<float>(frameData.screenWidth),
                         static_cast<float>(frameData.screenHeight)),
      .zNear = frameData.zNear, .zFar = frameData.zFar,
      .totalLightCount = params.activeLightCount,
      .shadowPointLightCount = params.shadowPointLightCount,
      .viewMatrix = frameData.viewMatrix
    };
    // Upload lighting metadata
    void* mapped = ctx.GetvkContext().getMappedPtr(lightingBuffer);
    std::memcpy(mapped, &lightingData, sizeof(Lighting::LightingBuffer));
    ctx.GetvkContext().flushMappedMemory(lightingBuffer, 0, sizeof(Lighting::LightingBuffer));
}


// Add to scene_feature.cpp
void SceneRenderFeature::executeClusterBoundsGeneration(internal::ExecutionContext& ctx)
{
    const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());
    if (params.activeLightCount == 0) return;
    EnsureLightingPipelines(ctx);
    const auto& frameData = ctx.GetFrameData();
    auto& cmd = ctx.GetvkCommandBuffer();
    float fovRadians = glm::radians(frameData.fovY);
    float tanHalfFovY = tan(fovRadians * 0.5f);
    struct ClusterBoundsPushConstants
    {
        uint64_t clusterBounds;
        float screenWidth, screenHeight;
        float zNear;
        float zFar;
        float tanHalfFovY;
    } pushConstants = {
        .clusterBounds = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(Lighting::CLUSTER_BOUNDS)),
        .screenWidth = static_cast<float>(frameData.screenWidth),
        .screenHeight = static_cast<float>(frameData.screenHeight),
        .zNear = frameData.zNear, .zFar = frameData.zFar,
        .tanHalfFovY = tanHalfFovY
    };
    static_assert(sizeof(ClusterBoundsPushConstants) == 32, "ClusterBounds push constants must match GLSL layout");
    cmd.cmdBindComputePipeline(m_clusterBoundsPipeline);
    cmd.cmdPushConstants(pushConstants);
    uint32_t groupsX = (Lighting::CLUSTER_DIM_X + 3) / 4;
    uint32_t groupsY = (Lighting::CLUSTER_DIM_Y + 3) / 4;
    uint32_t groupsZ = (Lighting::CLUSTER_DIM_Z + 3) / 4;
    cmd.cmdDispatchThreadGroups({ groupsX, groupsY, groupsZ });
}


void SceneRenderFeature::executeLightCulling(internal::ExecutionContext& ctx)
{
    const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());
    if (params.activeLightCount == 0) return;
    EnsureLightingPipelines(ctx);
    const auto& frameData = ctx.GetFrameData();
    auto& cmd = ctx.GetvkCommandBuffer();
    // Reset atomic counter
    vk::BufferHandle itemList = ctx.GetBuffer(Lighting::ITEM_LIST);
    cmd.cmdFillBuffer(itemList, 0, sizeof(uint32_t), 0);
    struct alignas(16) LightCullingPushConstants
    {
        uint64_t lights;
        uint64_t clusterBounds;
        uint64_t itemList;
        uint64_t clusters;
        mat4 viewMatrix;
        uint32_t totalLightCount;
    } pushConstants = {
        .lights = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(Lighting::LIGHT_BUFFER)),
        .clusterBounds = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(Lighting::CLUSTER_BOUNDS)),
        .itemList = ctx.GetvkContext().gpuAddress(itemList),
        .clusters = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(Lighting::CLUSTER_DATA)),
        .viewMatrix = frameData.viewMatrix,
        .totalLightCount = params.activeLightCount
    };
    static_assert(sizeof(LightCullingPushConstants) == 112, "LightCulling push constants must match GLSL layout");
    cmd.cmdBindComputePipeline(m_lightCullingPipeline);
    cmd.cmdPushConstants(pushConstants);
    // One thread per cluster
    uint32_t groupsX = (Lighting::CLUSTER_DIM_X + 3) / 4;
    uint32_t groupsY = (Lighting::CLUSTER_DIM_Y + 3) / 4;
    uint32_t groupsZ = (Lighting::CLUSTER_DIM_Z + 3) / 4;
    cmd.cmdDispatchThreadGroups({ groupsX, groupsY, groupsZ });
}


void SceneRenderFeature::EnsureLightingPipelines(internal::ExecutionContext& ctx)
{
    if (m_clusterBoundsPipeline.empty())
    {
        m_clusterBoundsShader = loadShaderModule(ctx.GetvkContext(), "shaders/cluster_bounds.comp");
        vk::ComputePipelineDesc desc = { .smComp = m_clusterBoundsShader, .debugName = "ClusterBoundsPipeline" };
        m_clusterBoundsPipeline = ctx.GetvkContext().createComputePipeline(desc);
    }
    if (m_lightCullingPipeline.empty())
    {
        m_lightCullingShader = loadShaderModule(ctx.GetvkContext(), "shaders/light_culling.comp");
        vk::ComputePipelineDesc desc = { .smComp = m_lightCullingShader, .debugName = "LightCullingPipeline" };
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

    vk::RenderPipelineDesc desc{
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

void SceneRenderFeature::EnsureDepthPrepassPipeline(internal::ExecutionContext& ctx)
{
    if(m_depthPrepassPipeline.valid() && m_depthPrepassAnimatedPipeline.valid())
        return;

    auto& vkCtx = ctx.GetvkContext();

    if(!m_depthPrepassVertShader.valid())
        m_depthPrepassVertShader = loadShaderModule(vkCtx, "shaders/depth_prepass.vert");
    if(!m_depthPrepassSkinnedVertShader.valid())
        m_depthPrepassSkinnedVertShader = loadShaderModule(vkCtx, "shaders/depth_prepass_skinned.vert");
    if(!m_depthPrepassFragShader.valid())
        m_depthPrepassFragShader = loadShaderModule(vkCtx, "shaders/depth_prepass.frag");

    if(!m_depthPrepassVertShader.valid() || !m_depthPrepassSkinnedVertShader.valid() || !m_depthPrepassFragShader.valid())
        return;

    vk::VertexInput staticInput;
    staticInput.attributes[0] = {
    .location = 0,
    .binding = 0,
    .format = vk::VertexFormat::Short3Norm,
        .offset = offsetof(CompressedVertex, pos_x)
    };
    staticInput.attributes[1] = {
    .location = 1,
    .binding = 0,
    .format = vk::VertexFormat::Short1Norm,
        .offset = offsetof(CompressedVertex, normal_x)
    };
    staticInput.attributes[2] = {
    .location = 2,
    .binding = 0,
    .format = vk::VertexFormat::R10G10B10A2_UNORM,
        .offset = offsetof(CompressedVertex, packed)
    };
    staticInput.attributes[3] = {
    .location = 3,
    .binding = 0,
    .format = vk::VertexFormat::UShort2Norm,
        .offset = offsetof(CompressedVertex, uv_x)
    };
    staticInput.inputBindings[0].stride = sizeof(CompressedVertex);

    vk::VertexInput skinnedInput;
    skinnedInput.attributes[0] = {
        .location = 0,
        .binding = 0,
        .format = vk::VertexFormat::Float3,
        .offset = offsetof(SkinnedVertex, position)
    };
    skinnedInput.attributes[1] = {
        .location = 1,
        .binding = 0,
        .format = vk::VertexFormat::UInt1,
        .offset = offsetof(SkinnedVertex, uvPacked)
    };
    skinnedInput.attributes[2] = {
        .location = 2,
        .binding = 0,
        .format = vk::VertexFormat::UInt1,
        .offset = offsetof(SkinnedVertex, normalPacked)
    };
    skinnedInput.attributes[3] = {
        .location = 3,
        .binding = 0,
        .format = vk::VertexFormat::UInt1,
        .offset = offsetof(SkinnedVertex, tangentPacked)
    };
    skinnedInput.inputBindings[0].stride = sizeof(SkinnedVertex);

    if(!m_depthPrepassPipeline.valid())
    {
        vk::RenderPipelineDesc desc = {
            .vertexInput = staticInput,
            .smVert = m_depthPrepassVertShader,
            .smFrag = m_depthPrepassFragShader,
            .color = {{
                .format = vk::Format::R_UI32,
                .blendEnabled = false
            }},
            .depthFormat = vk::Format::Z_F32,
            .cullMode = vk::CullMode::Back,
            .samplesCount = 1,
            .minSampleShading = 0.0f,
            .debugName = "DepthPrepassPipeline"
        };

        m_depthPrepassPipeline = vkCtx.createRenderPipeline(desc);
        if(!m_depthPrepassPipeline.valid())
            throw std::runtime_error("Failed to create depth prepass pipeline");
    }

    if(!m_depthPrepassAnimatedPipeline.valid())
    {
        vk::RenderPipelineDesc desc = {
            .vertexInput = skinnedInput,
            .smVert = m_depthPrepassSkinnedVertShader,
            .smFrag = m_depthPrepassFragShader,
            .color = {{
                .format = vk::Format::R_UI32,
                .blendEnabled = false
            }},
            .depthFormat = vk::Format::Z_F32,
            .cullMode = vk::CullMode::Back,
            .samplesCount = 1,
            .minSampleShading = 0.0f,
            .debugName = "DepthPrepassAnimatedPipeline"
        };

        m_depthPrepassAnimatedPipeline = vkCtx.createRenderPipeline(desc);
        if(!m_depthPrepassAnimatedPipeline.valid())
            throw std::runtime_error("Failed to create animated depth prepass pipeline");
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
  auto& vkCtx = ctx.GetvkContext();

  if(!vertShader.valid())
    vertShader = loadShaderModule(vkCtx, "shaders/object.vert");
  if(!m_skinnedVertShader.valid())
    m_skinnedVertShader = loadShaderModule(vkCtx, "shaders/object_skinned.vert");
  if(!opaquefragShader.valid())
    opaquefragShader = loadShaderModule(vkCtx, "shaders/opaque.frag");
  if(!transparentfragShader.valid())
    transparentfragShader = loadShaderModule(vkCtx, "shaders/transparent.frag");

  vk::TextureHandle color = ctx.GetTexture(RenderResources::SCENE_COLOR);
  vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);

  vk::VertexInput staticInput;
  staticInput.attributes[0] = {
    .location = 0,
    .binding = 0,
    .format = vk::VertexFormat::Short3Norm,
    .offset = offsetof(CompressedVertex, pos_x) };
  staticInput.attributes[1] = {
    .location = 1,
    .binding = 0,
    .format = vk::VertexFormat::Short1Norm,
    .offset = offsetof(CompressedVertex, normal_x) };
  staticInput.attributes[2] = {
    .location = 2,
    .binding = 0,
    .format = vk::VertexFormat::R10G10B10A2_UNORM,
    .offset = offsetof(CompressedVertex, packed) };
  staticInput.attributes[3] = {
    .location = 3,
    .binding = 0,
    .format = vk::VertexFormat::UShort2Norm,
    .offset = offsetof(CompressedVertex, uv_x) };
  staticInput.inputBindings[0].stride = sizeof(CompressedVertex);

  vk::VertexInput animatedInput;
  animatedInput.attributes[0] = {
    .location = 0,
    .binding = 0,
    .format = vk::VertexFormat::Float3,
    .offset = offsetof(SkinnedVertex, position) };
  animatedInput.attributes[1] = {
    .location = 1,
    .binding = 0,
    .format = vk::VertexFormat::UInt1,
    .offset = offsetof(SkinnedVertex, uvPacked) };
  animatedInput.attributes[2] = {
    .location = 2,
    .binding = 0,
    .format = vk::VertexFormat::UInt1,
    .offset = offsetof(SkinnedVertex, normalPacked) };
  animatedInput.attributes[3] = {
    .location = 3,
    .binding = 0,
    .format = vk::VertexFormat::UInt1,
    .offset = offsetof(SkinnedVertex, tangentPacked) };
  animatedInput.inputBindings[0].stride = sizeof(SkinnedVertex);

  if(!m_opaqueRenderPipeline.valid())
  {
    vk::RenderPipelineDesc desc{};
    desc.vertexInput = staticInput;
    desc.smVert = vertShader;
    desc.smFrag = opaquefragShader;
    desc.color[0].format = color.valid() ? vkCtx.getFormat(color) : vk::Format::Invalid;
    desc.depthFormat = depth.valid() ? vkCtx.getFormat(depth) : vk::Format::Invalid;
    desc.cullMode = vk::CullMode::Back;
    desc.samplesCount = sampleCount;
    desc.minSampleShading = 0.0f;
    desc.debugName = "SceneOpaquePipeline";

    m_opaqueRenderPipeline = vkCtx.createRenderPipeline(desc);
    if(!m_opaqueRenderPipeline.valid())
      throw std::runtime_error("Failed to create opaque render pipeline");
  }

  if(!m_opaqueAnimatedRenderPipeline.valid())
  {
    vk::RenderPipelineDesc desc{};
    desc.vertexInput = animatedInput;
    desc.smVert = m_skinnedVertShader;
    desc.smFrag = opaquefragShader;
    desc.color[0].format = color.valid() ? vkCtx.getFormat(color) : vk::Format::Invalid;
    desc.depthFormat = depth.valid() ? vkCtx.getFormat(depth) : vk::Format::Invalid;
    desc.cullMode = vk::CullMode::Back;
    desc.samplesCount = sampleCount;
    desc.minSampleShading = 0.0f;
    desc.debugName = "SceneOpaqueAnimatedPipeline";

    m_opaqueAnimatedRenderPipeline = vkCtx.createRenderPipeline(desc);
    if(!m_opaqueAnimatedRenderPipeline.valid())
      throw std::runtime_error("Failed to create animated opaque pipeline");
  }

  if(!m_transparentRenderPipeline.valid())
  {
    vk::RenderPipelineDesc desc{};
    desc.vertexInput = staticInput;
    desc.smVert = vertShader;
    desc.smFrag = transparentfragShader;
    desc.depthFormat = depth.valid() ? vkCtx.getFormat(depth) : vk::Format::Invalid;
    desc.cullMode = vk::CullMode::Back;
    desc.samplesCount = sampleCount;
    desc.minSampleShading = 0.0f;
    if(color.valid())
    {
      desc.color[0].format = vkCtx.getFormat(color);
      desc.color[0].blendEnabled = false;
      desc.color[0].rgbBlendOp = vk::BlendOp::Add;
      desc.color[0].alphaBlendOp = vk::BlendOp::Add;
      desc.color[0].srcRGBBlendFactor = vk::BlendFactor::SrcAlpha;
      desc.color[0].dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha;
      desc.color[0].srcAlphaBlendFactor = vk::BlendFactor::One;
      desc.color[0].dstAlphaBlendFactor = vk::BlendFactor::OneMinusSrcAlpha;
    }
    desc.debugName = "SceneTransparentPipeline";

    m_transparentRenderPipeline = vkCtx.createRenderPipeline(desc);
    if(!m_transparentRenderPipeline.valid())
      throw std::runtime_error("Failed to create transparent render pipeline");
  }

  if(!m_transparentAnimatedRenderPipeline.valid())
  {
    vk::RenderPipelineDesc desc{};
    desc.vertexInput = animatedInput;
    desc.smVert = m_skinnedVertShader;
    desc.smFrag = transparentfragShader;
    desc.depthFormat = depth.valid() ? vkCtx.getFormat(depth) : vk::Format::Invalid;
    desc.cullMode = vk::CullMode::Back;
    desc.samplesCount = sampleCount;
    desc.minSampleShading = 0.0f;
    if(color.valid())
    {
      desc.color[0].format = vkCtx.getFormat(color);
      desc.color[0].blendEnabled = false;
      desc.color[0].rgbBlendOp = vk::BlendOp::Add;
      desc.color[0].alphaBlendOp = vk::BlendOp::Add;
      desc.color[0].srcRGBBlendFactor = vk::BlendFactor::SrcAlpha;
      desc.color[0].dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha;
      desc.color[0].srcAlphaBlendFactor = vk::BlendFactor::One;
      desc.color[0].dstAlphaBlendFactor = vk::BlendFactor::OneMinusSrcAlpha;
    }
    desc.debugName = "SceneTransparentAnimatedPipeline";

    m_transparentAnimatedRenderPipeline = vkCtx.createRenderPipeline(desc);
    if(!m_transparentAnimatedRenderPipeline.valid())
      throw std::runtime_error("Failed to create animated transparent render pipeline");
  }
}

void SceneRenderFeature::EnsureDeformationPipeline(internal::ExecutionContext& ctx)
{
  if(m_deformationPipeline.empty())
  {
    m_deformationShader = loadShaderModule(
      ctx.GetvkContext(),
      "shaders/skinning.comp");
    vk::ComputePipelineDesc desc = {
      .smComp = m_deformationShader,
      .debugName = "MeshDeformationPipeline" };
    m_deformationPipeline = ctx.GetvkContext().createComputePipeline(desc);
    if(!m_deformationPipeline.valid())
      throw std::runtime_error("Failed to create deformation compute pipeline");
  }
}

const char* SceneRenderFeature::GetName() const
{
  return "SceneRenderFeature";
}

// ============================================================================
// Point Light Shadow Mapping Implementation
// ============================================================================

void SceneRenderFeature::EnsurePointShadowPipeline(internal::ExecutionContext& ctx)
{
    if (m_pointShadowPipeline.valid() && m_pointShadowSkinnedPipeline.valid())
        return;

    // Load shaders if not already loaded
    if (!m_pointShadowVertShader.valid())
        m_pointShadowVertShader = loadShaderModule(ctx.GetvkContext(), "shaders/point_shadow.vert");
    if (!m_pointShadowFragShader.valid())
        m_pointShadowFragShader = loadShaderModule(ctx.GetvkContext(), "shaders/point_shadow.frag");
    if (!m_pointShadowSkinnedVertShader.valid())
        m_pointShadowSkinnedVertShader = loadShaderModule(ctx.GetvkContext(), "shaders/point_shadow_skinned.vert");

    if (!m_pointShadowVertShader.valid() || !m_pointShadowFragShader.valid())
        return;

    // Static mesh vertex input layout (compressed format)
    vk::VertexInput staticVertexInput;
    staticVertexInput.attributes[0] = {
        .location = 0,
        .binding = 0,
        .format = vk::VertexFormat::Short3Norm,
        .offset = offsetof(CompressedVertex, pos_x) };
    staticVertexInput.attributes[1] = {
        .location = 1,
        .binding = 0,
        .format = vk::VertexFormat::Short1Norm,
        .offset = offsetof(CompressedVertex, normal_x) };
    staticVertexInput.attributes[2] = {
        .location = 2,
        .binding = 0,
        .format = vk::VertexFormat::R10G10B10A2_UNORM,
        .offset = offsetof(CompressedVertex, packed) };
    staticVertexInput.attributes[3] = {
        .location = 3,
        .binding = 0,
        .format = vk::VertexFormat::UShort2Norm,
        .offset = offsetof(CompressedVertex, uv_x) };
    staticVertexInput.inputBindings[0].stride = sizeof(CompressedVertex);

    // Create static mesh shadow pipeline
    if (!m_pointShadowPipeline.valid())
    {
        vk::RenderPipelineDesc pipelineDesc{};
        pipelineDesc.topology = vk::Topology::Triangle;
        pipelineDesc.vertexInput = staticVertexInput;
        pipelineDesc.smVert = m_pointShadowVertShader;
        pipelineDesc.smFrag = m_pointShadowFragShader;
        pipelineDesc.color[0].format = vk::Format::R_F16;  // R16F for linear depth
        pipelineDesc.depthFormat = vk::Format::Z_F32;
        pipelineDesc.cullMode = vk::CullMode::None;  // Disable culling for shadow pass
        pipelineDesc.frontFaceWinding = vk::WindingMode::CCW;
        pipelineDesc.debugName = "PointShadowPipeline";

        m_pointShadowPipeline = ctx.GetvkContext().createRenderPipeline(pipelineDesc);
    }

    // Skinned mesh vertex input layout (uncompressed format)
    if (!m_pointShadowSkinnedPipeline.valid() && m_pointShadowSkinnedVertShader.valid())
    {
        vk::VertexInput skinnedVertexInput;
        skinnedVertexInput.attributes[0] = {
            .location = 0,
            .binding = 0,
            .format = vk::VertexFormat::Float3,
            .offset = offsetof(SkinnedVertex, position) };
        skinnedVertexInput.attributes[1] = {
            .location = 1,
            .binding = 0,
            .format = vk::VertexFormat::UInt1,
            .offset = offsetof(SkinnedVertex, uvPacked) };
        skinnedVertexInput.attributes[2] = {
            .location = 2,
            .binding = 0,
            .format = vk::VertexFormat::UInt1,
            .offset = offsetof(SkinnedVertex, normalPacked) };
        skinnedVertexInput.attributes[3] = {
            .location = 3,
            .binding = 0,
            .format = vk::VertexFormat::UInt1,
            .offset = offsetof(SkinnedVertex, tangentPacked) };
        skinnedVertexInput.inputBindings[0].stride = sizeof(SkinnedVertex);

        vk::RenderPipelineDesc pipelineDesc{};
        pipelineDesc.topology = vk::Topology::Triangle;
        pipelineDesc.vertexInput = skinnedVertexInput;
        pipelineDesc.smVert = m_pointShadowSkinnedVertShader;
        pipelineDesc.smFrag = m_pointShadowFragShader;  // Reuse same fragment shader
        pipelineDesc.color[0].format = vk::Format::R_F16;
        pipelineDesc.depthFormat = vk::Format::Z_F32;
        pipelineDesc.cullMode = vk::CullMode::None;
        pipelineDesc.frontFaceWinding = vk::WindingMode::CCW;
        pipelineDesc.debugName = "PointShadowSkinnedPipeline";

        m_pointShadowSkinnedPipeline = ctx.GetvkContext().createRenderPipeline(pipelineDesc);
    }
}

void SceneRenderFeature::ExecutePointShadowPass(internal::ExecutionContext& ctx, uint32_t shadowLightIndex)
{
    const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());

    // Skip if this shadow light index is not active
    if (shadowLightIndex >= params.shadowPointLightCount)
        return;

    // Skip if no geometry to render
    const uint32_t animatedOpaqueCount = params.opaqueAnimatedCount;
    const uint32_t animatedTransparentCount = params.transparentAnimatedCount;
    const uint32_t staticTransparentCount = params.transparentStaticCount;
    if (params.opaqueStaticCount == 0 && animatedOpaqueCount == 0 && 
        staticTransparentCount == 0 && animatedTransparentCount == 0)
        return;

    EnsurePointShadowPipeline(ctx);
    if (!m_pointShadowPipeline.valid())
        return;

    auto& cmd = ctx.GetvkCommandBuffer();

    vk::BufferHandle pointShadowBuffer = ctx.GetBuffer(Lighting::POINT_SHADOW_BUFFER);

    // Push constants layout matches the shadow vertex/fragment shaders
    struct ShadowPushConstants {
        uint64_t shadowFrameConstants;
        uint64_t transforms;
        uint64_t drawData;
        uint64_t materials;
        uint64_t meshDecomp;
        uint64_t unused1;
        uint64_t unused2;
        uint64_t unused3;
    } pushConstants = {
        .shadowFrameConstants = ctx.GetvkContext().gpuAddress(pointShadowBuffer) + 
                                shadowLightIndex * sizeof(Lighting::GPUPointLightShadow),
        .transforms = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("ObjectTransforms")),
        .drawData = ctx.GetvkContext().gpuAddress(ctx.GetBuffer("DrawDataBuffer")),
        .materials = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MATERIAL_BUFFER)),
        .meshDecomp = ctx.GetvkContext().gpuAddress(ctx.GetBuffer(RenderResources::MESH_DECOMPRESSION_BUFFER)),
        .unused1 = 0,
        .unused2 = 0,
        .unused3 = 0,
    };

    // Set viewport and scissor for shadow map size
    vk::Viewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = static_cast<float>(Lighting::SHADOW_MAP_SIZE),
        .height = static_cast<float>(Lighting::SHADOW_MAP_SIZE),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    vk::ScissorRect scissor = {
        .x = 0, .y = 0,
        .width = Lighting::SHADOW_MAP_SIZE,
        .height = Lighting::SHADOW_MAP_SIZE
    };
    cmd.cmdBindViewport(viewport);
    cmd.cmdBindScissorRect(scissor);
    cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Less, .isDepthWriteEnabled = true});
    cmd.cmdBindIndexBuffer(ctx.GetBuffer(RenderResources::INDEX_BUFFER), vk::IndexFormat::UI32);

    // Draw static opaque geometry
    if (params.opaqueStaticCount > 0)
    {
        cmd.cmdBindRenderPipeline(m_pointShadowPipeline);
        cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
        cmd.cmdPushConstants(pushConstants);

        cmd.cmdDrawIndexedIndirectCount(
            ctx.GetBuffer("IndirectBufferOpaqueStatic"),
            sizeof(uint32_t),
            ctx.GetBuffer("IndirectBufferOpaqueStatic"),
            0,
            params.opaqueStaticCount,
            sizeof(DrawIndexedIndirectCommand));
    }

    // Draw static transparent geometry (for meshes incorrectly marked as transparent)
    if (staticTransparentCount > 0)
    {
        cmd.cmdBindRenderPipeline(m_pointShadowPipeline);
        cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::VERTEX_BUFFER));
        cmd.cmdPushConstants(pushConstants);

        cmd.cmdDrawIndexedIndirectCount(
            ctx.GetBuffer("IndirectBufferTransparentStatic"),
            sizeof(uint32_t),
            ctx.GetBuffer("IndirectBufferTransparentStatic"),
            0,
            staticTransparentCount,
            sizeof(DrawIndexedIndirectCommand));
    }

    // Draw animated/skinned opaque geometry
    if (animatedOpaqueCount > 0 && m_pointShadowSkinnedPipeline.valid())
    {
        cmd.cmdBindRenderPipeline(m_pointShadowSkinnedPipeline);
        cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::SKINNED_VERTEX_BUFFER));
        cmd.cmdPushConstants(pushConstants);

        cmd.cmdDrawIndexedIndirectCount(
            ctx.GetBuffer("IndirectBufferOpaqueAnimated"),
            sizeof(uint32_t),
            ctx.GetBuffer("IndirectBufferOpaqueAnimated"),
            0,
            animatedOpaqueCount,
            sizeof(DrawIndexedIndirectCommand));
    }

    // Draw animated/skinned transparent geometry
    if (animatedTransparentCount > 0 && m_pointShadowSkinnedPipeline.valid())
    {
        cmd.cmdBindRenderPipeline(m_pointShadowSkinnedPipeline);
        cmd.cmdBindVertexBuffer(0, ctx.GetBuffer(RenderResources::SKINNED_VERTEX_BUFFER));
        cmd.cmdPushConstants(pushConstants);

        cmd.cmdDrawIndexedIndirectCount(
            ctx.GetBuffer("IndirectBufferTransparentAnimated"),
            sizeof(uint32_t),
            ctx.GetBuffer("IndirectBufferTransparentAnimated"),
            0,
            animatedTransparentCount,
            sizeof(DrawIndexedIndirectCommand));
    }
}

void SceneRenderFeature::RequestObjectPick(int screenX, int screenY)
{
    if (!m_enableObjectPicking)
    {
        LOG_ERROR("Object picking is not enabled for this SceneRenderFeature instance");
        return;
    }

    m_pickRequest.screenX = screenX;
    m_pickRequest.screenY = screenY;
    m_pickRequest.pending = true;
}

SceneRenderFeature::PickResult SceneRenderFeature::GetLastPickResult() const
{
    if (!m_enableObjectPicking)
    {
        LOG_ERROR("Object picking is not enabled for this SceneRenderFeature instance");
        return PickResult{}; // Return invalid result
    }

    std::lock_guard<std::mutex> lock(m_pickResultMutex);
    return m_lastPickResult;
}

void SceneRenderFeature::ClearPickResult()
{
    if (!m_enableObjectPicking)
    {
        return; // Silently ignore if picking is disabled
    }

    std::lock_guard<std::mutex> lock(m_pickResultMutex);
    m_lastPickResult.valid = false;
}

void SceneRenderFeature::ProcessPendingPick(internal::ExecutionContext& ctx)
{
    if (!m_pickRequest.pending)
        return;

    m_pickRequest.pending = false;

    const auto& params = *static_cast<const SceneRenderParams*>(GetParameterBlock_RT());

    auto visibilityTexture = ctx.GetTexture("VisibilityBuffer");
    if (!visibilityTexture.valid())
    {
        return;
    }

    auto& vkCtx = ctx.GetvkContext();

    uint32_t visibilityValue = 0;

    vk::TextureRangeDesc range = {
        .offset = {
            .x = static_cast<int32_t>(m_pickRequest.screenX),
            .y = static_cast<int32_t>(m_pickRequest.screenY),
            .z = 0
        },
        .dimensions = { .width = 1, .height = 1, .depth = 1 },
        .layer = 0,
        .numLayers = 1,
        .mipLevel = 0,
        .numMipLevels = 1
    };

    vk::Result result = vkCtx.download(visibilityTexture, range, &visibilityValue);

    if (!result.isOk())
    {
        return;
    }

    if (visibilityValue == 0)
    {
        std::lock_guard<std::mutex> lock(m_pickResultMutex);
        m_lastPickResult.valid = false;
        return;
    }

    uint32_t drawId, primitiveId;
    VisibilityBuffer::Unpack(visibilityValue, drawId, primitiveId);

    if (drawId >= params.drawData.size())
    {
        std::lock_guard<std::mutex> lock(m_pickResultMutex);
        m_lastPickResult.valid = false;
        return;
    }

    uint32_t sceneObjectIndex = params.drawData[drawId].objectId;

    {
        std::lock_guard<std::mutex> lock(m_pickResultMutex);
        m_lastPickResult.valid = true;
        m_lastPickResult.sceneObjectIndex = sceneObjectIndex;
        m_lastPickResult.primitiveId = primitiveId;
        m_lastPickResult.drawId = sceneObjectIndex;
    }
}
