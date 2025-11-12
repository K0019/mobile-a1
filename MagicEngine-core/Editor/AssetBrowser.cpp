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
#include "Editor/SoundTab.h"
#include "Editor/SceneTab.h"
#include "Editor/FileBrowserTab.h"
#include "Editor/MiscAssetTabs.h"

#include "Scripting/HotReloader.h"

namespace fs = std::filesystem;

namespace editor {

    AssetBrowser::AssetBrowser()
        : WindowBase{ ICON_FA_FOLDER" Browser", gui::Vec2{ 1200.0f, 600.0f } }
    {
#ifdef IMGUI_ENABLED
        // Initialize with default state
        assetCategories.push_back(std::make_shared<MeshTab>());
        assetCategories.push_back(std::make_shared<MaterialTab>());
        assetCategories.push_back(std::make_shared<TextureTab>());
        assetCategories.push_back(std::make_shared<PrefabTab>());
        assetCategories.push_back(std::make_shared<SceneTab>());
        assetCategories.push_back(std::make_shared<SoundTab>());
        assetCategories.push_back(std::make_shared<FontTab>());
        assetCategories.push_back(std::make_shared<ScriptTab>());
        //assetCategories.push_back(std::make_shared<ShaderTab>());

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
            if (gui::Selectable(assetCategories[i]->GetIdentifier(), currentCategoryIndex == i))
                currentCategoryIndex = static_cast<int>(i);
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
        constexpr float searchWidth = 300;

        assetCategories[currentCategoryIndex]->RenderBreadcrumb();

        // Right-aligned search bar
        gui::SameLine(windowWidth - searchWidth);
        gui::SetNextItemWidth(searchWidth);
        searchBuffer.Draw("##filter", ICON_FA_MAGNIFYING_GLASS" Search");
    }

}