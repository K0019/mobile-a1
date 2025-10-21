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
#include "Editor/AssetBrowserCategories.h"
#include "Engine/Resources/ResourceManager.h"
#include "Managers/Filesystem.h"

class AssetBrowser {
    friend class ST<AssetBrowser>;
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
    
    /**
     * @brief Destroy the Asset Browser object.
     */
    ~AssetBrowser();

    //FileSystem file_system;

#ifdef IMGUI_ENABLED

    /**
     * @brief Draw the asset browser UI.
     * @param p_open Pointer to a boolean indicating if the window is open.
     */
    void Draw(bool* p_open = nullptr);

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

    /**
     * @brief Draw the configuration UI for the asset browser.
     */
    void DrawConfig();

private:
    std::vector<std::unique_ptr<editor::BaseAssetCategory>> assetCategories;
    int currentCategoryIndex = 0;

public:
    // Constants
    char searchBuffer[256] = ""; /**< Buffer for search input */
#endif // IMGUI_ENABLED
    static constexpr float THUMBNAIL_SIZE = 50.0f; /**< Size of thumbnails */
    static constexpr float SIDEBAR_WIDTH = 150.0f; /**< Width of the sidebar */

};
