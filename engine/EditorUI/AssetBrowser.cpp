/******************************************************************************/
/*!
\file   AssetBrowser.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Ryan Cheong (95%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Chan Kuan Fu Ryan (5%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
Asset browser for viewing and managing game assets.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "AssetBrowser.h"
#include "PrefabWindow.h"
#include "SceneManagement.h"
#include "EditorHistory.h"
#include "NameComponent.h"
#include "AudioManager.h"

#include "Filesystem.h"
#include "Import.h"

#include "AssetBrowserCategories.h"
#include "MeshTab.h"
#include "MaterialTab.h"
#include "TextureTab.h"
#include "SpriteTab.h"
#include "AnimationTab.h"
#include "SoundTab.h"
#include "SceneTab.h"
#include "FileBrowserTab.h"
#include "MiscAssetTabs.h"

#include "CSScripting.h"
#include "ScriptManagement.h"
#include "HotReloader.h"

namespace fs = std::filesystem;

AssetBrowser::AssetBrowser() {
#ifdef IMGUI_ENABLED
    // Initialize with default state
    assetCategories.push_back(std::make_unique<MeshTab>());
    assetCategories.push_back(std::make_unique<MaterialTab>());
    assetCategories.push_back(std::make_unique<TextureTab>());
    assetCategories.push_back(std::make_unique<SpriteTab>());
    assetCategories.push_back(std::make_unique<AnimationTab>());
    assetCategories.push_back(std::make_unique<PrefabTab>());
    assetCategories.push_back(std::make_unique<SceneTab>());
    assetCategories.push_back(std::make_unique<SoundTab>());
    assetCategories.push_back(std::make_unique<FontTab>());
    assetCategories.push_back(std::make_unique<ScriptTab>());
    assetCategories.push_back(std::make_unique<ShaderTab>());

    //FileSystem inside AssetBrowser or its own tab?
    auto browser = std::make_unique<FileBrowserTab>(); browser->Initialize(ST<Filepaths>::Get()->workingDir);
    assetCategories.push_back(std::move(browser));
#endif
}

AssetBrowser::~AssetBrowser() = default;

#ifdef IMGUI_ENABLED
void AssetBrowser::Draw(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(1200, 600), ImGuiCond_FirstUseEver);

    if(!ImGui::Begin(ICON_FA_FOLDER" Browser", p_open)) {
        ImGui::End();
        return;
    }

    // Main layout
    RenderSidebar();
    ImGui::SameLine();

    RenderMainView();
    if(ImGui::BeginDragDropTarget())
    {
        // Accept the drag if the payload is of the correct type
        ImGuiPayload const* acceptedPayload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptPeekOnly);
        if(acceptedPayload)
        {
            ecs::EntityHandle draggedEntity = *(ecs::EntityHandle*)acceptedPayload->Data;

            // Drop handling 
            //currentCategory = CATEGORY::PREFABS;
            if(ImGui::IsMouseReleased(0))
            {
                PrefabManager::SavePrefab(draggedEntity, draggedEntity->GetComp<NameComponent>()->GetName());
            }
        }
        ImGui::EndDragDropTarget();
    }
    //ShowSpriteSheetDialog();
    //ShowCreateAnimationDialog();

    ImGui::End();
}

void AssetBrowser::RenderSidebar() {
    ImGui::BeginChild("Sidebar", ImVec2(SIDEBAR_WIDTH, 0), true);

    for (size_t i = 0; i < assetCategories.size(); i++)
    {
        auto& category = assetCategories[i];
        if (ImGui::Selectable(category->GetIdentifier(), currentCategoryIndex == i))
        {
            currentCategoryIndex = i;
        }
    }

    ImGui::EndChild();
}

void AssetBrowser::RenderMainView() {
    ImGui::BeginChild("MainView");

    // Toolbar with filter and breadcrumb
    RenderToolbar();

    assetCategories[currentCategoryIndex]->Render();

    ImGui::EndChild();
}

void AssetBrowser::RenderToolbar()
{
    ImGui::BeginGroup();
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float searchWidth = 300;
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    assetCategories[currentCategoryIndex]->RenderBreadcrumb();


    // Right-aligned search bar
    ImGui::SameLine(windowWidth - searchWidth);
    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS" Search", searchBuffer, std::size(searchBuffer));

    ImGui::EndGroup();
}

void AssetBrowser::DrawConfig()
{
    ImGui::Separator();
    ImGui::Text("Browser Settings");
    ImGui::DragFloat("Thumbnail Size", &THUMBNAIL_SIZE, 10.0f, 50.0f, 200.0f);
    ImGui::DragFloat("Sidebar Width", &SIDEBAR_WIDTH, 10.0f, 150.0f, 250.0f);
}

#endif