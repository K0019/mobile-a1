#pragma once

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <optional>
#include "renderer/gfx_material_system.h"

// Forward declarations
class GfxRenderer;
class MagicResourceManager;

namespace editor
{
    /**
     * ThumbnailCache - Manages asset thumbnail previews for the editor UI.
     *
     * Loads thumbnails by reading the .meta sidecar files to find thumbnailPath.
     * Thumbnails are generated at asset compile time.
     */
    class ThumbnailCache
    {
    public:
        enum class AssetType { Texture, Material, Mesh };

        static ThumbnailCache& Get();

        void Initialize(GfxRenderer* renderer);
        void Shutdown();

        // Called every frame (currently a no-op, reserved for async generation)
        void Update();

        /**
         * Get thumbnail ImGui texture ID for an asset.
         * Returns 0 if not available.
         * @param assetHash The resource hash
         * @param type The asset type
         * @param assetName Unused, kept for API compatibility
         */
        uint64_t GetThumbnail(size_t assetHash, AssetType type, const std::string& assetName = {});

        // Check if thumbnail is being generated (reserved for future async)
        bool IsPending(size_t assetHash) const;

        // Invalidate a cached thumbnail (e.g., when asset is recompiled)
        void Invalidate(size_t assetHash);

        // Clear all cached thumbnails
        void ClearAll();

    private:
        ThumbnailCache() = default;
        ~ThumbnailCache() = default;
        ThumbnailCache(const ThumbnailCache&) = delete;
        ThumbnailCache& operator=(const ThumbnailCache&) = delete;

        struct CacheEntry {
            gfx::TextureHandle textureHandle;
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

        GfxRenderer* m_renderer = nullptr;
        std::unordered_map<size_t, CacheEntry> m_cache;
        std::unordered_set<size_t> m_pendingSet;

        std::filesystem::path m_compiledAssetsDir = "Assets/compiledassets";
        bool m_initialized = false;
    };
}
