/**
 * @file minimal_scene_feature.cpp
 * @brief Minimal scene rendering feature implementation.
 */

#include "minimal_scene_feature.h"
#include "renderer/render_graph.h"

void MinimalSceneFeature::SetupPasses(internal::RenderPassBuilder& passBuilder) {
    const MinimalSceneParams& params = *static_cast<const MinimalSceneParams*>(GetParameterBlock_RT());

    if (!params.enabled) {
        return;
    }

    // Opaque pass - renders to SCENE_COLOR with depth testing
    PassDeclarationInfo opaquePassInfo;
    opaquePassInfo.framebufferDebugName = "MinimalOpaque";
    opaquePassInfo.colorAttachments[0] = {
        .textureName = RenderResources::SCENE_COLOR,
        .loadOp = gfx::LoadOp::Clear,
        .storeOp = gfx::StoreOp::Store,
        .clearColor = {0.1f, 0.1f, 0.1f, 1.0f}
    };
    opaquePassInfo.depthAttachment = {
        .textureName = RenderResources::SCENE_DEPTH,
        .loadOp = gfx::LoadOp::Clear,
        .storeOp = gfx::StoreOp::Store,
        .clearDepth = 1.0f
    };

    passBuilder.CreatePass()
        .UseResource(RenderResources::SCENE_COLOR, AccessType::Write)
        .UseResource(RenderResources::SCENE_DEPTH, AccessType::Write)
        .SetPriority(internal::RenderPassBuilder::PassPriority::Opaque)
        .AddGraphicsPass("MinimalOpaque", opaquePassInfo,
            [this](internal::ExecutionContext& ctx) {
                // For now, this is a placeholder
                // Actual rendering will be done by GfxRenderer
                // This just ensures the pass is registered and executed
                (void)ctx;
            });
}
