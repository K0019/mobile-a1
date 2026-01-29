#pragma once
#include <cstdint>
#include <vector>
#include "math/utils_math.h"

// Feature mask for controlling which render features execute per view
using RenderFeatureId = uint8_t;
using FeatureMask = uint64_t;

constexpr FeatureMask FeatureBit(RenderFeatureId feature)
{
    return FeatureMask(1) << static_cast<uint8_t>(feature);
}

constexpr FeatureMask AllFeatureBits()
{
    return ~FeatureMask(0);
}

// Well-known feature bits
namespace RenderFeatures
{
    constexpr RenderFeatureId Scene         = 0;
    constexpr RenderFeatureId Grid          = 1;
    constexpr RenderFeatureId Gizmos        = 2;
    constexpr RenderFeatureId DebugOverlays = 3;
    constexpr RenderFeatureId PostProcess   = 4;
    constexpr RenderFeatureId Shadows       = 5;
    constexpr RenderFeatureId UI            = 6;

    // Preset combinations
    constexpr FeatureMask SceneViewMask = FeatureBit(Scene) | FeatureBit(Grid) | FeatureBit(Gizmos) |
                                          FeatureBit(DebugOverlays) | FeatureBit(Shadows);
    constexpr FeatureMask GameViewMask  = FeatureBit(Scene) | FeatureBit(PostProcess) | FeatureBit(Shadows);
    constexpr FeatureMask AllMask       = AllFeatureBits();
}

// Output texture binding info for a rendered view (for display/sampling)
struct ViewOutputInfo
{
    uint32_t colorTextureId = 0;  // UI texture ID for ImGui display
};

// Resolution settings for rendering
struct RenderResolution
{
    uint32_t width = 0;    // Explicit width (0 = use scale)
    uint32_t height = 0;   // Explicit height (0 = use scale)
    float scale = 1.0f;    // Scale factor when width/height = 0
};

// Render quality settings
struct RenderSettings
{
    RenderResolution resolution;
    uint32_t msaaSamples = 1;
    bool useHDR = true;
};

// Surface/swapchain info
struct RenderSurfaceInfo
{
    uint32_t presentWidth = 1;
    uint32_t presentHeight = 1;
    uint32_t renderWidth = 1;
    uint32_t renderHeight = 1;
};

// Global frame info (shared across all views)
struct FrameGlobalInfo
{
    uint64_t frameNumber = 0;
    float deltaTime = 0.0f;
};

// Per-view frame data (camera, settings, output)
struct FrameData
{
    // View identification
    uint32_t viewId = 0;

    // Camera matrices
    mat4 viewMatrix = mat4(1.0f);
    mat4 projMatrix = mat4(1.0f);
    vec3 cameraPos = vec3(0.0f);
    vec3 cameraDir = vec3(0.0f, 0.0f, -1.0f);

    // Viewport dimensions
    float viewportWidth = 1.0f;
    float viewportHeight = 1.0f;

    // Frustum settings
    float zNear = 0.1f;
    float zFar = 1000.0f;
    float fovY = 45.0f;

    // Feature mask - controls which features render for this view
    FeatureMask featureMask = RenderFeatures::AllMask;

    // Output texture info for this view (for display after rendering)
    ViewOutputInfo outputInfo;

    // Legacy fields for compatibility
    uint64_t frameNumber = 0;
    float deltaTime = 0.0f;
    uint32_t screenWidth = 1;
    uint32_t screenHeight = 1;
    uint32_t depth = 1;
};

// Container for all views to be rendered in a frame
struct RenderFrameData
{
    FrameGlobalInfo frameInfo;
    RenderSurfaceInfo surface;
    RenderSettings settings;
    std::vector<FrameData> views;
    uint32_t presentedViewId = 0;  // Which view should present to swapchain
};

// Helper functions
inline FrameData& AddView(RenderFrameData& renderFrame, uint32_t viewId)
{
    FrameData view{};
    view.viewId = viewId;
    renderFrame.views.push_back(view);
    return renderFrame.views.back();
}

inline FrameData& EnsureView(RenderFrameData& renderFrame, uint32_t viewId)
{
    if (renderFrame.views.size() <= viewId)
    {
        renderFrame.views.resize(viewId + 1);
    }
    FrameData& view = renderFrame.views[viewId];
    view.viewId = viewId;
    return view;
}
