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
#include "ResourceManager.h"
#include "Filesystem.h"

class AssetBrowser {
    friend class ST<AssetBrowser>;
    public:
    /**
     * @brief Enum representing different categories of assets.
     */
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
    ~AssetBrowser() = default;

    /**
     * @brief Draw the asset browser UI.
     * @param p_open Pointer to a boolean indicating if the window is open.
     */
#ifdef IMGUI_ENABLED
    void Draw(bool* p_open = nullptr);

    /**
     * @brief Draw the configuration UI for the asset browser.
     */
    void DrawConfig();
#endif

    FileSystem file_system;

    private:
#ifdef IMGUI_ENABLED
    // Core state
    CATEGORY currentCategory = CATEGORY::NONE; /**< Current category of assets being viewed */

    //SPRITE RELATED CODE
    /**
     * @brief Import a file as a sprite.
     * @param path The file path to import.
     * @param name The name of the sprite.
     */
    void ImportAsSprite(const std::filesystem::path& path, const std::string& name);

    /**
     * @brief Import a file as a sprite sheet.
     * @param path The file path to import.
     * @param spriteCount The number of sprites in the sheet.
     * @param baseName The base name for the sprites.
     */
    void ImportAsSpriteSheet(const std::filesystem::path& path, int spriteCount, const std::string& baseName);

    /*****************************************************************//*!
    \brief
        Imports a file into working directory if outside of it.
    \param file
        The file to import into working directory.
    \return
        The relative filepath to the imported asset.
    *//******************************************************************/
    std::filesystem::path CopyIntoWorkingDir(const std::filesystem::path& file);

    /**
     * @brief Show the sprite sheet import dialog.
     */
    void ShowSpriteSheetDialog();

    /**
     * @brief Render the sprite grid.
     */
    void RenderSpriteGrid();
    /**
     * @brief Load a thumbnail for a given file path.
     * @param path The file path to load the thumbnail for.
     */
    void LoadThumbnail(const std::filesystem::path& path);

    /**
     * @brief Get the thumbnail descriptor for a given file path.
     * @param path The file path to get the thumbnail descriptor for.
     * @return VkDescriptorSet The thumbnail descriptor.
     */

    VkDescriptorSet GetThumbnailDescriptor(std::filesystem::path::iterator::reference path);
    struct SpriteImportConfig {
        int spriteCount = 1; /**< Number of sprites in the sheet */
        std::filesystem::path targetPath; /**< Target path for the sprite import */
        bool showDialog = false; /**< Flag to show the import dialog */
        std::string spriteName; /**< Name of the sprite */
        bool isSpriteSheet = false; /**< Flag indicating if the import is a sprite sheet */
    };
    SpriteImportConfig spriteConfig;
#endif
    struct ThumbnailCache {
        std::unordered_map<std::string, VkDescriptorSet> textureDescriptors; /**< Cache of texture descriptors */
        std::unordered_map<std::string, bool> loadingStatus; /**< Cache of loading statuses */
    };
    ThumbnailCache thumbnailCache;
#ifdef IMGUI_ENABLED
    //ANIMATION RELATED CODE
    /**
     * @brief Render the sprite selection grid for animations.
     */
    void RenderSpriteSelectionGrid();

    /**
     * @brief Render the list of frames for an animation.
     */
    void RenderFrameList();

    /**
     * @brief Render the animation grid.
     */
    void RenderAnimationGrid();

    /**
     * @brief Render the sound table.
     */
    void RenderSoundTable();

    /**
     * @brief Render context menu when a sound is right-clicked.
     */
    void RenderSoundContextMenu(std::string const& name, bool isGrouped);

    /**
     * @brief Render the bottom part of the create animation UI.
     */
    void RenderCreateAnimationBottom();

    /**
     * @brief Render the animation preview.
     */
    void RenderAnimationPreview();

    /**
     * @brief Show the create animation dialog.
     */
    void ShowCreateAnimationDialog();

    struct AnimationCreateConfig {
        bool showDialog = false; /**< Flag to show the create animation dialog */
        std::string animationName; /**< Name of the animation */
        std::vector<FrameData> frames; /**< Frames of the animation */
        bool isPlaying = false; /**< Flag indicating if the animation is playing */
        float previewTime = 0.0f; /**< Preview time for the animation */
        size_t currentFrame = 0; /**< Current frame of the animation */
        char spriteSearchBuffer[256] = ""; /**< Buffer for sprite search */
        std::string warningMessage; /**< Current warning message */
        bool hasWarning = false; /**< Flag indicating if there is a warning */

        /**
         * @brief Reset the animation create configuration.
         */
        void Reset();
    };

    AnimationCreateConfig animConfig;

    struct AnimationPreviewState {
        bool isPlaying = false; /**< Flag indicating if the animation preview is playing */
        float previewTime = 0.0f; /**< Preview time for the animation */
        size_t currentFrame = 0; /**< Current frame of the animation preview */
    };

    //FILESYSTEM RELATED CODE
    /**
     * @brief Render the navigation bar for the filesystem.
     */
    void RenderNavigationBar();

    /**
     * @brief Render the label for a grid item.
     * @param filename The name of the file.
     */
    void RenderItemLabel(const std::string& filename);

    /**
     * @brief Render the context menu for a file item.
     * @param entry The file entry to render the context menu for.
     */
    void RenderItemContextMenu(const FileSystem::FileEntry& entry);

    /**
     * @brief Render a directory item.
     * @param entry The directory entry to render.
     * @return true If the directory item was rendered successfully.
     * @return false If the directory item was not rendered.
     */
    bool RenderDirectoryItem(const FileSystem::FileEntry& entry);

    /**
     * @brief Render a file item.
     * @param entry The file entry to render.
     * @return true If the file item was rendered successfully.
     * @return false If the file item was not rendered.
     */
    bool RenderFileItem(const FileSystem::FileEntry& entry);

    /**
     * @brief Render the filesystem view.
     */
    void RenderFileSystem();

    //MAIN VIEW RELATED CODE
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
     * @brief Draw the prefab window. //Done by matthew
     */
    void RenderPrefabWindow();

    /**
     * @brief Render the scene window. //Done by Kendrick
     */
    void RenderSceneWindow();

    void RenderScriptWindow();

    void RenderShadersWindow();

    void RenderFontWindow();
    // Helpers
    /**
     * @brief Check if a name matches the current filter.
     * @param name The name to check.
     * @return true If the name matches the filter.
     * @return false If the name does not match the filter.
     */
    bool MatchesFilter(const std::string& name) const;

    public:
    // Constants
    char searchBuffer[256] = ""; /**< Buffer for search input */
    float THUMBNAIL_SIZE = 50.0f; /**< Size of thumbnails */
    float SIDEBAR_WIDTH = 150.0f; /**< Width of the sidebar */
    float lastFrameTime = 0.0f; /**< Time of the last frame */
    float accumulatedTime = 0.0f; /**< Accumulated time for animation preview */
#endif
};