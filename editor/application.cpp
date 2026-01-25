#include "application.h"
#include <core/engine/engine.h>
#include "Engine/Engine.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "FilepathConstants.h"
//#include "resource/asset_startup_service.h"
#include "renderer/features/scene_feature.h"
#include "renderer/features/grid_feature.h"
#include "renderer/features/ui2d_render_feature.h"
#include "renderer/gfx_renderer.h"
//#include "Editor/EditorAssetReloadManager.h"

// Editor panel includes
#include <imgui.h>
#include "Editor/Editor.h"
#include "Editor/MainMenuBar.h"
#include "Editor/Popup.h"
#include "Editor/Console.h"
#include "Editor/Performance.h"
#include "Editor/Inspector.h"
#include "Editor/AssetBrowser.h"
#include "Editor/Hierarchy.h"
#include "Editor/Containers/GUIAsECS.h"
#include "Graphics/CustomViewport.h"
#include "Utilities/Serializer.h"
#include <fstream>
#include <sstream>

namespace
{
    void saveEditorState(const char* filename) {
        ecs::SwitchToPool(ecs::POOL::EDITOR_GUI);
        Serializer serializer{ filename };
        serializer.Serialize("show_console", ecs::GetCompsBegin<editor::Console>() != ecs::GetCompsEnd<editor::Console>());
        serializer.Serialize("show_performance", ecs::GetCompsBegin<editor::PerformanceWindow>() != ecs::GetCompsEnd<editor::PerformanceWindow>());
        serializer.Serialize("show_editor", ecs::GetCompsBegin<editor::Inspector>() != ecs::GetCompsEnd<editor::Inspector>());
        serializer.Serialize("show_browser", ecs::GetCompsBegin<editor::AssetBrowser>() != ecs::GetCompsEnd<editor::AssetBrowser>());
        serializer.Serialize("show_hierarchy", ecs::GetCompsBegin<editor::Hierarchy>() != ecs::GetCompsEnd<editor::Hierarchy>());
        ecs::SwitchToPool(ecs::POOL::DEFAULT);
    }

    void loadEditorState(const char* filename) {
        std::ifstream t(filename);
        std::stringstream buffer;
        buffer << t.rdbuf();
        Deserializer deserializer{ buffer.str() };
        if (!deserializer.IsValid())
            return;

        auto LoadWindowOpen{ [&deserializer, b = false]<typename WindowType>(const char* varName) mutable -> void {
            deserializer.DeserializeVar(varName, &b);
            if (b)
                editor::CreateGuiWindow<WindowType>();
        } };

        LoadWindowOpen.template operator()<editor::Console>("show_console");
        LoadWindowOpen.template operator()<editor::PerformanceWindow>("show_performance");
        LoadWindowOpen.template operator()<editor::Inspector>("show_editor");
        LoadWindowOpen.template operator()<editor::AssetBrowser>("show_browser");
        LoadWindowOpen.template operator()<editor::Hierarchy>("show_hierarchy");
    }
}

void Application::Initialize(Context& context)
{
    // Initialize engine immediately - don't wait for compilation
    magicEngine.Init(context);

    // Create render features - Editor gets all features including grid
    InitializeFeatures(context);

    // Register editor-specific systems (must be done here since editor headers
    // are not available when MagicEngine is compiled)
    ecs::AddSystem(ECS_LAYER::PERMANENT_EDITOR, editor::EditorSystem{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_EDITOR, editor::MainMenuBar{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_EDITOR, editor::Popup{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_INPUT, editor::EditorInputSystem{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_RENDER, editor::SelectedEntityBorderDrawSystem{});

    // Load saved editor window state
    loadEditorState("imgui.json");

    // Create default viewport
    auto worldExtents{ IntVec2{ 1920, 1080 } };
    editor::CreateGuiWindow<CustomViewport>(static_cast<unsigned int>(worldExtents.x), static_cast<unsigned int>(worldExtents.y));

}

void Application::InitializeFeatures(Context& context)
{
    auto* graphics = ST<GraphicsMain>::Get();

    // Initialize ImGui for Editor (Game does not call this)
    const std::string fontsFile{ Filepaths::fontsSave + "/Lato-Regular.ttf" };
    graphics->InitializeImGui(fontsFile);

    // Editor creates all features including grid
    sceneFeatureHandle_ = context.renderer->CreateFeature<SceneRenderFeature>(true);  // with object picking
    gridFeatureHandle_ = context.renderer->CreateFeature<GridFeature>();
    ui2dFeatureHandle_ = context.renderer->CreateFeature<Ui2DRenderFeature>();

    // Pass handles to GraphicsMain for compatibility with existing code
    graphics->SetSceneFeatureHandle(sceneFeatureHandle_);
    graphics->SetGridFeatureHandle(gridFeatureHandle_);
    graphics->SetUI2DFeatureHandle(ui2dFeatureHandle_);
    graphics->InitializeUI2DOverlay();

    // Enable grid by default in editor
    graphics->SetGridEnabled(true);
}

void Application::DestroyFeatures(Context& context)
{
    if (context.renderer)
    {
        if (ui2dFeatureHandle_ != 0)
            context.renderer->DestroyFeature(ui2dFeatureHandle_);
        if (gridFeatureHandle_ != 0)
            context.renderer->DestroyFeature(gridFeatureHandle_);
        if (sceneFeatureHandle_ != 0)
            context.renderer->DestroyFeature(sceneFeatureHandle_);
    }
    ui2dFeatureHandle_ = 0;
    gridFeatureHandle_ = 0;
    sceneFeatureHandle_ = 0;
}

void Application::Update(Context& context, RenderFrameData& frame)
{
    const int width = static_cast<int>(frame.surface.presentWidth);
    const int height = static_cast<int>(frame.surface.presentHeight);
    updateViews(context, frame, width, height);

    // Execute the MagicEngine frame (which handles ECS, ImGui, etc.)
    magicEngine.ExecuteFrame(frame);
}

void Application::updateViews(Context& context, RenderFrameData& frame, int width, int height)
{
    // Get feature masks for each registered feature
    if (!context.renderer)
        return;

    const FeatureMask sceneMask = context.renderer->GetFeatureMaskForHandle(sceneFeatureHandle_);
    const FeatureMask gridMask = context.renderer->GetFeatureMaskForHandle(gridFeatureHandle_);
    const FeatureMask uiMask = context.renderer->GetFeatureMaskForHandle(ui2dFeatureHandle_);

    const FeatureMask imguiMask = context.renderer->GetFeatureMaskForHandle(
        ST<GraphicsMain>::Get()->GetImGuiContext().GetRenderFeatureHandle());

    // Editor uses 3 views: game, scene, editor (following ryEngine pattern)
    frame.views.resize(3);
    FrameData& gameView = EnsureView(frame, 0);
    FrameData& sceneView = EnsureView(frame, 1);
    FrameData& editorView = EnsureView(frame, 2);

    // Configure viewport from current camera (CustomViewport uploads camera via SetViewCamera)
    // For now, we apply the feature masks to each view
    // The camera matrices are set via SetViewCamera in CustomViewport::DrawWindow

    // Game view: scene + UI only (no editor overlays)
    gameView.featureMask = sceneMask | uiMask;
    gameView.viewportWidth = static_cast<float>(width);
    gameView.viewportHeight = static_cast<float>(height);

    // Scene view: full feature set (editor overlays)
    sceneView.featureMask = sceneMask | gridMask | uiMask;
    sceneView.viewportWidth = static_cast<float>(width);
    sceneView.viewportHeight = static_cast<float>(height);

    // Editor view: ImGui composition + presentation target
    // ImGui renders to VIEW_OUTPUT, then FinalBlit copies to swapchain
    editorView.featureMask = imguiMask;
    editorView.viewportWidth = static_cast<float>(width);
    editorView.viewportHeight = static_cast<float>(height);

    // The editor view is what presents to the swapchain
    frame.presentedViewId = editorView.viewId;
}

void Application::Shutdown([[maybe_unused]] Context& context)
{
    saveEditorState("imgui.json");

    DestroyFeatures(context);
    magicEngine.shutdown();
}
