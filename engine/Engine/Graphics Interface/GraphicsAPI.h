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
#include "renderer.h"
#include "asset_system.h"
#include "imgui_context.h"
#include "GraphicsWindow.h"
#include "GraphicsScene.h"

class GraphicsAssets
{
public:
    bool Init(Context* context);

public:
    AssetLoading::AssetSystem* INTERNAL_GetAssetSystem();

private:
    UPtr<AssetLoading::AssetSystem> assetSystem;

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

