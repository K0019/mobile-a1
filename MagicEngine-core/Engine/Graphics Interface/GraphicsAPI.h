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
#include "graphics/renderer.h"
#include "resource/resource_manager.h"
#include "imgui/base/imgui_context.h"
#include "Engine/Graphics Interface/GraphicsWindow.h"
#include "graphics/features/scene_feature.h"
#include "resource/resource_types.h"
#include "math/camera.h"

class GraphicsMain
{
public:
    ~GraphicsMain();

    void Init(Context inContext);

    void BeginFrame();
#ifdef IMGUI_ENABLED
    void BeginImGuiFrame();
    void EndImGuiFrame();
#endif
    void EndFrame(FrameData* outFrameData);

    void SetPendingShutdown();
    bool GetIsPendingShutdown() const;

    // Graphics scene interface - for camera uploads
    void SetViewCamera(const Camera& camera);

public:
    FrameData& INTERNAL_GetFrameData();

private:
#ifdef IMGUI_ENABLED
    void InitImGui(const std::string& fontfile);
    void SetImGuiStyle();
#endif
    void UploadToPipeline(FrameData* outFrameData);

public:
    // For compatibility with whatever old graphics interfaces are still here
    // Remove if possible once everything settles
#ifdef IMGUI_ENABLED
    editor::ImGuiContext& GetImGuiContext();
#endif
    Resource::ResourceManager& GetAssetSystem();

private:
    friend ST<GraphicsMain>;
    GraphicsMain();

private:
#ifdef IMGUI_ENABLED
    UPtr<editor::ImGuiContext> imguiContext;
#endif

    Context context;

    // Scene rendering
    uint64_t sceneFeatureHandle;
    uint64_t ui2dFeatureHandle;
    FrameData frameData;
    uint64_t gridHandle;
    FontHandle ui2dFontHandle;

};

