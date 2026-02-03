/******************************************************************************/
/*!
\file   GraphicsAPI.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Abstracts the interface to the graphics pipeline.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "renderer/gfx_renderer.h"
#include "resource/resource_manager.h"
#include "imgui/base/imgui_context.h"
#include "Engine/Graphics Interface/GraphicsWindow.h"
#include "renderer/features/scene_feature.h"
#include "renderer/ui/ui_immediate.h"
#include "resource/resource_types.h"
#include "math/camera.h"
#include "renderer/frame_data.h"

class GraphicsMain
{
public:
    ~GraphicsMain();

    void Init(Context inContext);

    void BeginFrame();
    void BeginImGuiFrame();
    void EndImGuiFrame();
    void EndFrame(FrameData* outFrameData);
    void EndFrame(RenderFrameData* outRenderFrameData);  // Populates ALL views with camera matrices

    void SetPendingShutdown();
    bool GetIsPendingShutdown() const;

    // Graphics scene interface - for camera uploads
    void SetViewCamera(const Camera& camera);

    // Grid control
    void SetGridEnabled(bool enabled);

    // Feature handle setters - called by Application after creating features
    void SetSceneFeatureHandle(uint64_t handle) { sceneFeatureHandle = handle; }
    void SetGridFeatureHandle(uint64_t handle) { gridHandle = handle; }
    void SetUI2DFeatureHandle(uint64_t handle) { ui2dFeatureHandle = handle; }
    void SetIm3DFeatureHandle(uint64_t handle) { im3dHandle = handle; }

    // Feature handle getters - for code that needs to reference features
    uint64_t GetSceneFeatureHandle() const { return sceneFeatureHandle; }
    uint64_t GetGridFeatureHandle() const { return gridHandle; }
    uint64_t GetUI2DFeatureHandle() const { return ui2dFeatureHandle; }
    uint64_t GetIm3DFeatureHandle() const { return im3dHandle; }

    // Initialize UI2D overlay (call after setting UI2D feature handle)
    void InitializeUI2DOverlay();

    // Initialize ImGui - call from Editor Application only (not for Game)
    void InitializeImGui(const std::string& fontfile);

    // Scene view texture - returns ViewOutput texture ID for ImGui viewport display
    uint64_t GetSceneViewTextureId() const {
        if (context.renderer) {
            return context.renderer->getViewOutputTextureId(ViewId::Scene);
        }
        return 0;
    }

public:
    FrameData& INTERNAL_GetFrameData();
    bool RequestObjPick(int mouseX, int mouseY);
    ecs::EntityHandle PreviousPick();

private:
    void InitImGui(const std::string& fontfile);
    void SetImGuiStyle();
    void InitFont(const std::string& fontfile);
    void UploadToPipeline(FrameData* outFrameData);

    static void OnTogglePlayMode();

public:
    // For compatibility with whatever old graphics interfaces are still here
    // Remove if possible once everything settles
    editor::ImGuiContext& GetImGuiContext();
    Resource::ResourceManager& GetAssetSystem();
    ui::ImmediateGui& GetImmediateGui();

private:
    friend ST<GraphicsMain>;
    GraphicsMain();

    void InitDefaultSkybox();

private:
    UPtr<editor::ImGuiContext> imguiContext;

    Context context;

    // Scene rendering
    uint64_t sceneFeatureHandle;
    uint64_t ui2dFeatureHandle;
    FrameData frameData;
    uint64_t gridHandle;
    uint64_t im3dHandle;
    FontHandle ui2dFontHandle;
    UPtr<ui::ImmediateGui> overlayGui;
    uint64_t lastPickedObjectIndex;
    std::unordered_map<uint64_t, ecs::EntityHandle> mapIdxToId;

    // Default skybox
    uint32_t defaultSkyboxBindlessIndex = 0;
    TextureHandle defaultSkyboxHandle;

public:
    // Renderer access
    GfxRenderer* GetRenderer() const { return context.renderer; }

    // Skybox texture access
    TextureHandle GetDefaultSkyboxHandle() const { return defaultSkyboxHandle; }

    // Scene view feature mask (toggled via viewport UI)
    FeatureMask& GetSceneViewFeatureMask() { return sceneViewFeatureMask; }
    const FeatureMask& GetSceneViewFeatureMask() const { return sceneViewFeatureMask; }

private:
    FeatureMask sceneViewFeatureMask = ~FeatureMask(0); // all features on by default

};

