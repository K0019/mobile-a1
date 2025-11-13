/******************************************************************************/
/*!
\file   AssetBrowser.h
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

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Editor/Containers/GUIAsECS.h"
#include "Editor/AssetBrowserCategories.h"
#include "Engine/Resources/ResourceManager.h"
#include "Managers/Filesystem.h"

namespace editor {

    class AssetBrowser : public WindowBase<AssetBrowser>
    {
    public:
        enum class CATEGORY {
            NONE,       /**< No category */
            SPRITES,    /**< Sprite assets */
            ANIMATIONS, /**< Animation assets */
            SOUNDS,     /**< Sound assets */
            FONTS,      /**< Font assets */
            PREFABS,    /**< Prefab assets */
            SCENES,     /**< Scene assets */
            SCRIPTS,    /**< Script assets */
            SHADERS,    /**< Shader assets */
            FILESYSTEM  /**< Filesystem assets */
        };

        /**
         * @brief Construct a new Asset Browser object.
         */
        AssetBrowser();

        void DrawWindow() override;

        /**
         * @brief Render the sidebar of the asset browser.
         */
        void RenderSidebar();

        /**
         * @brief Render the main view of the asset browser.
         */
        void RenderMainView();

        /**
         * @brief Render the toolbar of the asset browser.
         */
        void RenderToolbar();

    private:
        std::vector<SPtr<BaseAssetCategory>> assetCategories;
        int currentCategoryIndex = 0;

        gui::TextBoxWithFilter searchBuffer;

    public:
        // Constants
        static constexpr float THUMBNAIL_SIZE = 50.0f; /**< Size of thumbnails */
        static constexpr float SIDEBAR_WIDTH = 150.0f; /**< Width of the sidebar */

    };

}
