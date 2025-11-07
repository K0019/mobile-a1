// im3d_feature.h
#pragma once
#include "graphics/interface.h"
#include "graphics/render_graph.h"
#include <im3d.h>

struct Im3dRenderParams {
    bool enabled = true;
};

class Im3dFeature : public RenderFeatureBase<Im3dRenderParams> {
public:
    Im3dFeature() = default;
    ~Im3dFeature() override = default;
    
    const char* GetName() const override { return "Im3d"; }
    void* GetGTParameterBlock_RT() override { return nullptr; }
    
    void SetupPasses(internal::RenderPassBuilder& builder) override;

private:
    void RenderIm3dPass(internal::ExecutionContext& ctx);
    void EnsurePipelineCreated(internal::ExecutionContext& context);
    
    // Pipelines for different primitive types
    vk::Holder<vk::RenderPipelineHandle> m_linesPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_trianglesPipeline;
    
    // Shader modules
    vk::Holder<vk::ShaderModuleHandle> m_vertShader;
    vk::Holder<vk::ShaderModuleHandle> m_geomShader;
    vk::Holder<vk::ShaderModuleHandle> m_fragShader;
    
    bool m_resourcesCreated = false;
};