#pragma once

#include "graphics/interface.h"
#include "graphics/render_feature.h"
#include "graphics/render_graph.h"
#include "graphics/scene.h"
#include "math/utils_math.h"

namespace AssetLoading
{
  class AssetSystem;
  struct Scene;
}

class Renderer;

struct DrawIndexedIndirectCommand
{
  uint32_t count;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t baseVertex;
  uint32_t baseInstance;
};

struct DrawData
{
  uint32_t transformId; // Index into transform arrays
  uint32_t materialId;  // Material index for GPU buffer access
};

struct CullingData
{
  vec4 frustumPlanes[6];
  vec4 frustumCorners[8];
  uint32_t numMeshesToCull = 0;
  uint32_t numVisibleMeshes = 0;
};

struct SceneRenderParams
{
  static constexpr uint32_t MAX_OBJECTS = 1000;

  // Object data
  std::vector<mat4> objectTransforms;
  std::vector<BoundingBox> objectBounds;
  std::vector<uint32_t> materialIndices;

  // Draw commands (all objects)
  std::vector<DrawIndexedIndirectCommand> drawCommands;
  std::vector<DrawData> drawData;

  // Render queues (indices into drawCommands array)
  std::vector<uint32_t> opaqueIndices;
  std::vector<uint32_t> transparentIndices;

  uint32_t getObjectCount() const
  {
    return static_cast<uint32_t>(objectTransforms.size());
  }

  uint32_t getDrawCommandCount() const
  {
    return static_cast<uint32_t>(drawCommands.size());
  }

  uint32_t getOpaqueCount() const
  {
    return static_cast<uint32_t>(opaqueIndices.size());
  }

  uint32_t getTransparentCount() const
  {
    return static_cast<uint32_t>(transparentIndices.size());
  }

  void clear()
  {
    objectTransforms.clear();
    objectBounds.clear();
    materialIndices.clear();
    drawCommands.clear();
    drawData.clear();
    opaqueIndices.clear();
    transparentIndices.clear();
  }

  void reserve(size_t objectCount)
  {
    objectTransforms.reserve(objectCount);
    objectBounds.reserve(objectCount);
    materialIndices.reserve(objectCount);
    drawCommands.reserve(objectCount);
    drawData.reserve(objectCount);
    opaqueIndices.reserve(objectCount);
    transparentIndices.reserve(objectCount);
  }
};

class SceneRenderFeature final : public RenderFeatureBase<SceneRenderParams>
{
  using Parameters = SceneRenderParams;

  public:
    void static UpdateScene(uint64_t renderFeatureID,
                            const AssetLoading::Scene& gameScene,
                            const AssetLoading::AssetSystem& asset_system,
                            Renderer& renderer);

    void SetupPasses(Render::internal::RenderPassBuilder& passBuilder) override;

    const char* GetName() const override;

  private:
    void extractFrustumCorners(const mat4& viewProj, vec4* corners);

    void ExecuteCullingPass(Render::internal::ExecutionContext& ctx);

    void ExecuteOpaquePass(Render::internal::ExecutionContext& ctx);

    void extractFrustumPlanes(const mat4& viewProj, vec4* planes);

    void EnsureComputePipeline(Render::internal::ExecutionContext& ctx);

    void EnsureRenderPipelines(Render::internal::ExecutionContext& ctx);

    vk::Holder<vk::ShaderModuleHandle> m_cullingShader;
    vk::Holder<vk::ComputePipelineHandle> m_cullingPipeline;

    vk::Holder<vk::ShaderModuleHandle> vertShader;
    vk::Holder<vk::ShaderModuleHandle> fragShader;
    vk::Holder<vk::RenderPipelineHandle> m_opaqueRenderPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_transparentRenderPipeline;
};
