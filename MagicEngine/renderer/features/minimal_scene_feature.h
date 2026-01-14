/**
 * @file minimal_scene_feature.h
 * @brief Minimal scene rendering feature - placeholder for RenderGraph hookup.
 *
 * This is a stripped-down version that demonstrates the RenderGraph connection.
 * Actual rendering will be done by GfxRenderer; this feature just provides
 * the pass structure and execution hooks.
 */

#pragma once

#include "renderer/render_feature.h"
#include "renderer/gfx_interface.h"
#include "math/utils_math.h"

/**
 * @brief Parameters for minimal scene rendering.
 */
struct MinimalSceneParams {
    bool enabled = true;
    uint32_t sampleCount = 1;
};

/**
 * @brief Minimal scene render feature.
 *
 * Registers passes with the RenderGraph for:
 * - Clear pass (scene color/depth)
 * - Opaque geometry pass
 *
 * Actual mesh rendering is delegated to GfxRenderer.
 */
class MinimalSceneFeature final : public RenderFeatureBase<MinimalSceneParams> {
public:
    MinimalSceneFeature() = default;
    ~MinimalSceneFeature() override = default;

    const char* GetName() const override { return "MinimalSceneFeature"; }

    void SetupPasses(internal::RenderPassBuilder& passBuilder) override;
};
