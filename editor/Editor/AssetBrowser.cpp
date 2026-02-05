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
#include "Editor/AssetBrowser.h"
#include "Engine/SceneManagement.h"
#include "Editor/EditorHistory.h"
#include "Components/NameComponent.h"
#include "Managers/AudioManager.h"
#include "Editor/Containers/GUICollection.h"

#include "FilepathConstants.h"
#include "Managers/Filesystem.h"
#include "Editor/Import.h"
#include "Engine/PrefabManager.h"

#include "Editor/AssetBrowserCategories.h"
#include "Editor/MeshTab.h"
#include "Editor/MaterialTab.h"
#include "Editor/TextureTab.h"
#include "Editor/AnimationsTab.h"
#include "Editor/SoundTab.h"
#include "Editor/VideoTab.h"
#include "Editor/SceneTab.h"
#include "Editor/FileBrowserTab.h"
#include "Editor/MiscAssetTabs.h"
#include "Editor/GameTab.h"

#include "Scripting/HotReloader.h"

namespace fs = std::filesystem;

namespace editor {

    // Static member initialization for AssetBrowserSettings
    float AssetBrowserSettings::thumbnailSize = 50.0f;
    AssetBrowserSettings::ViewMode AssetBrowserSettings::viewMode = AssetBrowserSettings::ViewMode::Grid;

    AssetBrowser::AssetBrowser()
        : WindowBase{ ICON_FA_FOLDER" Browser", gui::Vec2{ 1200.0f, 600.0f } }
    {
#ifdef IMGUI_ENABLED
        // Initialize with default state
        assetCategories.push_back(std::make_shared<MeshTab>());
        assetCategories.push_back(std::make_shared<MaterialTab>());
        assetCategories.push_back(std::make_shared<TextureTab>());
        assetCategories.push_back(std::make_shared<AnimationsTab>());
        assetCategories.push_back(std::make_shared<PrefabTab>());
        assetCategories.push_back(std::make_shared<SceneTab>());
        assetCategories.push_back(std::make_shared<SoundTab>());
        assetCategories.push_back(std::make_shared<VideoTab>());
        assetCategories.push_back(std::make_shared<FontTab>());
        assetCategories.push_back(std::make_shared<ScriptTab>());
        assetCategories.push_back(std::make_shared<GameTab>());
        assetCategories.push_back(std::make_shared<ShaderTab>());

        //FileSystem inside AssetBrowser or its own tab?
        auto browser = std::make_shared<FileBrowserTab>();
        browser->Initialize(Filepaths::workingDir);
        assetCategories.push_back(std::move(browser));
#endif
    }

    void AssetBrowser::DrawWindow()
    {
        // Main layout
        RenderSidebar();
        gui::SameLine();

        RenderMainView();
        gui::PayloadTarget<ecs::EntityHandle>("ENTITY", [](ecs::EntityHandle draggedEntity) -> void {
            PrefabManager::SavePrefab(draggedEntity, draggedEntity->GetComp<NameComponent>()->GetName());
        });
    }

    void AssetBrowser::RenderSidebar()
    {
        gui::Child sidebarChild{ "Sidebar", gui::Vec2{ SIDEBAR_WIDTH, 0 }, gui::FLAG_CHILD::BORDERS };

        for (size_t i{}; i < assetCategories.size(); ++i)
        {
            if (gui::Selectable(assetCategories[i]->GetIdentifier(), currentCategoryIndex == i))
            {
                if (currentCategoryIndex != static_cast<int>(i))
                {
                    currentCategoryIndex = static_cast<int>(i);
                    searchBuffer.Clear();  // Clear search when switching tabs
                }
            }
        }
    }

    void AssetBrowser::RenderMainView()
    {
        gui::Child mainViewChild{ "MainView" };

        // Toolbar with filter and breadcrumb
        RenderToolbar();

        assetCategories[currentCategoryIndex]->Render(searchBuffer);
    }

    void AssetBrowser::RenderToolbar()
    {
        gui::Group toolbarGroup{};

        const float windowWidth{ gui::GetAvailableContentRegion().x };

        // Get style values for accurate layout calculations
        const ImGuiStyle& style = ImGui::GetStyle();
        const float itemSpacing = style.ItemSpacing.x;
        const float framePadding = style.FramePadding.x;

        // Widget sizes - account for frame padding where needed
        constexpr float buttonSize = 28;  // Slightly larger for better clickability
        const float sliderWidth = 100;
        const float searchWidth = 200;

        // Get capabilities for current tab
        auto capabilities = assetCategories[currentCategoryIndex]->GetToolbarCapabilities();

        // Always render breadcrumb
        assetCategories[currentCategoryIndex]->RenderBreadcrumb();

        // Allow tab to add custom controls (rendered after breadcrumb)
        assetCategories[currentCategoryIndex]->RenderToolbarExtras();

        // Calculate controls width based on what's visible, including spacing between elements
        float controlsWidth = 0;
        int numElements = 0;

        if (capabilities.showViewToggle)
        {
            controlsWidth += buttonSize * 2 + itemSpacing;  // Two buttons + spacing between them
            numElements++;
        }
        if (capabilities.showThumbnailSlider && AssetBrowserSettings::viewMode == AssetBrowserSettings::ViewMode::Grid)
        {
            controlsWidth += sliderWidth + framePadding * 2;  // Slider with frame padding
            numElements++;
        }
        if (capabilities.showGlobalSearch)
        {
            controlsWidth += searchWidth + framePadding * 2;  // Search with frame padding
            numElements++;
        }

        // Add spacing between elements
        if (numElements > 1)
            controlsWidth += itemSpacing * (numElements - 1);

        // Only position if we have controls to show
        if (controlsWidth > 0)
            gui::SameLine(windowWidth - controlsWidth);

        using ViewMode = AssetBrowserSettings::ViewMode;

        // View mode toggle buttons (if enabled)
        if (capabilities.showViewToggle)
        {
            // Use minimal padding for icon-only buttons so icons are visible
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

            bool isGridMode = (AssetBrowserSettings::viewMode == ViewMode::Grid);
            if (isGridMode) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
            if (ImGui::Button(ICON_FA_GRIP, ImVec2(buttonSize, buttonSize)))
                AssetBrowserSettings::viewMode = ViewMode::Grid;
            if (isGridMode) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Grid View");

            gui::SameLine();

            bool isListMode = (AssetBrowserSettings::viewMode == ViewMode::List);
            if (isListMode) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
            if (ImGui::Button(ICON_FA_LIST, ImVec2(buttonSize, buttonSize)))
                AssetBrowserSettings::viewMode = ViewMode::List;
            if (isListMode) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("List View");

            ImGui::PopStyleVar();  // Restore FramePadding

            gui::SameLine();
        }

        // Thumbnail size slider (if enabled and in grid mode)
        if (capabilities.showThumbnailSlider && AssetBrowserSettings::viewMode == ViewMode::Grid)
        {
            gui::SetNextItemWidth(sliderWidth);
            ImGui::SliderFloat("##size", &AssetBrowserSettings::thumbnailSize,
                AssetBrowserSettings::MIN_THUMBNAIL_SIZE, AssetBrowserSettings::MAX_THUMBNAIL_SIZE, "%.0f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Thumbnail Size");
            gui::SameLine();
        }

        // Search bar (if enabled)
        if (capabilities.showGlobalSearch)
        {
            gui::SetNextItemWidth(searchWidth);
            searchBuffer.Draw("##filter", ICON_FA_MAGNIFYING_GLASS" Search");
        }
    }

}