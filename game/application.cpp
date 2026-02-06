#include "application.h"
#include <core/engine/engine.h>
#include "Engine/Engine.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "renderer/features/scene_feature.h"
#include "renderer/features/ui2d_render_feature.h"
#ifndef NDEBUG
#include "renderer/features/im3d_feature.h"
#endif
#include "renderer/gfx_renderer.h"
#include "Utilities/Logging.h"
#include "Game/GameSystems.h"
#include <cstdio>

void GameApplication::Initialize(Context& context)
{
#ifndef NDEBUG
    printf("[GameApp] Initializing...\n");
#endif

    // Game starts directly in IN_GAME mode (not EDITOR)
    magicEngine.Init(context, true);  // true = start in game mode

    // Create render features - Game only gets essential features (NO grid)
    InitializeFeatures(context);

#ifndef NDEBUG
    // DEBUG: Log game state after initialization
    printf("[GameApp] Initialized. GameState=%d\n", static_cast<int>(ST<GameSystemsManager>::Get()->GetState()));
    fflush(stdout);
#endif
}

void GameApplication::InitializeFeatures(Context& context)
{
    // Game creates only essential features - NO grid, NO object picking, NO ImGui
    sceneFeatureHandle_ = context.renderer->CreateFeature<SceneRenderFeature>(true);  // no object picking
#ifndef NDEBUG
    im3dFeatureHandle_ = context.renderer->CreateFeature<Im3dRenderFeature>();
#endif
    ui2dFeatureHandle_ = context.renderer->CreateFeature<Ui2DRenderFeature>();

    // Pass handles to GraphicsMain for compatibility with existing code
    auto* graphics = ST<GraphicsMain>::Get();
    graphics->SetSceneFeatureHandle(sceneFeatureHandle_);
#ifndef NDEBUG
    graphics->SetIm3DFeatureHandle(im3dFeatureHandle_);
#endif
    graphics->SetUI2DFeatureHandle(ui2dFeatureHandle_);
    graphics->InitializeUI2DOverlay();
    // Note: No grid handle set - game doesn't have grid
    // Note: No ImGui initialization - game doesn't get ImGui
}

void GameApplication::DestroyFeatures(Context& context)
{
    if (context.renderer)
    {
        if (ui2dFeatureHandle_ != 0)
            context.renderer->DestroyFeature(ui2dFeatureHandle_);
#ifndef NDEBUG
        if (im3dFeatureHandle_ != 0)
            context.renderer->DestroyFeature(im3dFeatureHandle_);
#endif
        if (sceneFeatureHandle_ != 0)
            context.renderer->DestroyFeature(sceneFeatureHandle_);
    }
    ui2dFeatureHandle_ = 0;
#ifndef NDEBUG
    im3dFeatureHandle_ = 0;
#endif
    sceneFeatureHandle_ = 0;
}

void GameApplication::Update(Context& context, RenderFrameData& frame)
{
#ifndef NDEBUG
    // DEBUG: Log periodically to verify frame loop
    static int frameCount = 0;
    if (frameCount++ % 300 == 0) {  // Every ~5 seconds at 60fps
        printf("[GameApp] Frame %d GameState=%d\n", frameCount, static_cast<int>(ST<GameSystemsManager>::Get()->GetState()));
        fflush(stdout);
    }
#endif

    // Setup single-view rendering for game
    const int width = static_cast<int>(frame.surface.presentWidth);
    const int height = static_cast<int>(frame.surface.presentHeight);
    updateViews(context, frame, width, height);

    // Execute the MagicEngine frame
    magicEngine.ExecuteFrame(frame);
}

void GameApplication::updateViews(Context& context, RenderFrameData& frame, int width, int height)
{
    // Game uses single view with scene + UI features only
    if (!context.renderer)
        return;

    const FeatureMask sceneMask = context.renderer->GetFeatureMaskForHandle(sceneFeatureHandle_);
    const FeatureMask uiMask = context.renderer->GetFeatureMaskForHandle(ui2dFeatureHandle_);

    // Game uses single view - direct render to swapchain
    frame.views.resize(1);
    FrameData& gameView = EnsureView(frame, 0);

    // Game view: scene + UI (no grid, no ImGui, no editor overlays)
    gameView.featureMask = sceneMask | uiMask;
    gameView.viewportWidth = static_cast<float>(width);
    gameView.viewportHeight = static_cast<float>(height);

    // The game view presents directly to swapchain
    frame.presentedViewId = gameView.viewId;
}

void GameApplication::Shutdown([[maybe_unused]] Context& context)
{
    DestroyFeatures(context);
    magicEngine.shutdown();
}
