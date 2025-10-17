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
#include "Engine/Graphics Interface/GraphicsScene.h"

class GraphicsAssets
{
public:
    bool Init(Context* context);

public:
    Resource::ResourceManager* INTERNAL_GetAssetSystem();

private:
    UPtr<Resource::ResourceManager> assetSystem;

};

class GraphicsMain
{
public:
    ~GraphicsMain();

    void Init();

    void BeginFrame();
#ifdef IMGUI_ENABLED
    void BeginImGuiFrame();
    void EndImGuiFrame();
#endif
    void EndFrame();

    void SetPendingShutdown();
    bool GetIsPendingShutdown() const;

public:
    void INTERNAL_OnWindowResized(int width, int height);

private:
#ifdef IMGUI_ENABLED
    void InitImGui(const std::string& fontfile);
    void SetImGuiStyle();
#endif

public:
    // For compatibility with whatever old graphics interfaces are still here
    // Remove if possible once everything settles
#ifdef IMGUI_ENABLED
    editor::ImGuiContext& GetImGuiContext();
#endif

private:
    friend ST<GraphicsMain>;
    GraphicsMain() = default;

private:
    UPtr<Renderer> renderer;
#ifdef IMGUI_ENABLED
    UPtr<editor::ImGuiContext> imguiContext;
#endif

    Context context;

};

