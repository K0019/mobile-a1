#pragma once

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <optional>
#include <memory>
#include "renderer/gfx_material_system.h"
#include "resource/resource_types.h"
#include "Utilities/AsyncProcessRunner.h"

// Forward declarations
class GfxRenderer;
class AssetManager;

namespace editor
{
    /**
     * ThumbnailCache - Manages asset thumbnail previews for the editor UI.
     *
     * Loads thumbnails by reading the .meta sidecar files to find thumbnailPath.
     * Thumbnails are generated at asset compile time, or on-demand via background
     * AssetCompiler.exe --thumbnail-only subprocess.
     */
    class ThumbnailCache
    {
    public:
        enum class AssetType { Texture, Material, Mesh };

        // Info about an asset missing a thumbnail
        struct MissingThumbnailInfo {
            size_t assetHash;
            std::filesystem::path assetPath;
            std::filesystem::path metaPath;
            AssetType type;
        };

        static ThumbnailCache& Get();

        void Initialize(GfxRenderer* renderer);
        void Shutdown();

        // Called every frame - processes background thumbnail generation
        void Update();

        /**
         * Get thumbnail ImGui texture ID for an asset.
         * Returns 0 if not available.
         * @param assetHash The resource hash
         * @param type The asset type
         * @param assetName Unused, kept for API compatibility
         */
        uint64_t GetThumbnail(size_t assetHash, AssetType type, const std::string& assetName = {});

        // Check if thumbnail is being generated
        bool IsPending(size_t assetHash) const;

        // Invalidate a cached thumbnail (e.g., when asset is recompiled)
        void Invalidate(size_t assetHash);

        // Clear all cached thumbnails
        void ClearAll();

        // Enable/disable automatic thumbnail generation for missing thumbnails
        void SetAutoGenerateEnabled(bool enabled) { m_autoGenerateEnabled = enabled; }
        bool IsAutoGenerateEnabled() const { return m_autoGenerateEnabled; }

        // Get number of missing thumbnails queued for generation
        size_t GetMissingCount() const { return m_missingThumbnails.size(); }
        size_t GetQueuedCount() const { return m_generationQueue.size(); }

    private:
        ThumbnailCache() = default;
        ~ThumbnailCache() = default;
        ThumbnailCache(const ThumbnailCache&) = delete;
        ThumbnailCache& operator=(const ThumbnailCache&) = delete;

        struct CacheEntry {
            ::TextureHandle textureHandle;
            uint64_t imguiId = 0;
            bool ready = false;
            bool loadAttempted = false;  // Avoid repeated failed loads
        };

        // Load thumbnail using the meta file's thumbnailPath
        bool LoadThumbnailFromMeta(size_t hash);

        // Get the asset's file path from the resource manager
        std::filesystem::path GetAssetFilePath(size_t hash) const;

        // Read thumbnailPath from a .meta file
        std::string ReadThumbnailPathFromMeta(const std::filesystem::path& metaPath) const;

        // Determine asset type from file extension
        AssetType DetermineAssetType(const std::filesystem::path& path) const;

        // Start background thumbnail generation for an asset
        void StartThumbnailGeneration(const MissingThumbnailInfo& info);

        // Track an asset as missing a thumbnail (will be queued for generation)
        void TrackMissingThumbnail(size_t hash, const std::filesystem::path& assetPath,
                                   const std::filesystem::path& metaPath);

        GfxRenderer* m_renderer = nullptr;
        std::unordered_map<size_t, CacheEntry> m_cache;
        std::unordered_set<size_t> m_pendingSet;

        // Background thumbnail generation state
        std::unordered_map<size_t, MissingThumbnailInfo> m_missingThumbnails;
        std::queue<size_t> m_generationQueue;
        std::shared_ptr<util::AsyncProcessHandle> m_currentGeneration;
        size_t m_currentGenerationHash = 0;  // Hash of asset currently being generated
        bool m_autoGenerateEnabled = true;

        std::filesystem::path m_compiledAssetsDir = "Assets/compiledassets";
        bool m_initialized = false;
    };
}
