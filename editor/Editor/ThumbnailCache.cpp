#include "Editor/ThumbnailCache.h"
#include "renderer/gfx_renderer.h"
#include "logging/log.h"
#include "Engine/Resources/ResourceManager.h"
#include <fstream>
#include <sstream>

namespace editor
{
    ThumbnailCache& ThumbnailCache::Get()
    {
        static ThumbnailCache instance;
        return instance;
    }

    void ThumbnailCache::Initialize(GfxRenderer* renderer)
    {
        if (m_initialized) return;

        m_renderer = renderer;
        m_initialized = true;

        LOG_INFO("[ThumbnailCache] Initialized");
    }

    void ThumbnailCache::Shutdown()
    {
        if (!m_initialized) return;

        // Destroy all loaded textures
        if (m_renderer)
        {
            auto& matSystem = m_renderer->getMaterialSystem();
            for (auto& [hash, entry] : m_cache)
            {
                if (matSystem.isTextureValid(entry.textureHandle))
                {
                    matSystem.destroyTexture(entry.textureHandle);
                }
            }
        }

        m_cache.clear();
        m_pendingSet.clear();
        m_initialized = false;

        LOG_INFO("[ThumbnailCache] Shutdown");
    }

    void ThumbnailCache::Update()
    {
        // Reserved for future async generation
    }

    uint64_t ThumbnailCache::GetThumbnail(size_t assetHash, AssetType type, const std::string& assetName)
    {
        (void)type;      // Not used - we get path from resource manager
        (void)assetName; // Not used - kept for API compatibility

        if (!m_initialized || !m_renderer) return 0;

        // Check if already cached
        auto it = m_cache.find(assetHash);
        if (it != m_cache.end())
        {
            if (it->second.ready)
            {
                return it->second.imguiId;
            }
            if (it->second.loadAttempted)
            {
                // Already tried and failed, don't retry every frame
                return 0;
            }
        }

        // Try to load
        if (LoadThumbnailFromMeta(assetHash))
        {
            return m_cache[assetHash].imguiId;
        }

        return 0;
    }

    bool ThumbnailCache::IsPending(size_t assetHash) const
    {
        return m_pendingSet.count(assetHash) > 0;
    }

    void ThumbnailCache::Invalidate(size_t assetHash)
    {
        auto it = m_cache.find(assetHash);
        if (it != m_cache.end())
        {
            // Destroy the texture
            if (m_renderer)
            {
                auto& matSystem = m_renderer->getMaterialSystem();
                if (matSystem.isTextureValid(it->second.textureHandle))
                {
                    matSystem.destroyTexture(it->second.textureHandle);
                }
            }
            m_cache.erase(it);
        }

        m_pendingSet.erase(assetHash);
    }

    void ThumbnailCache::ClearAll()
    {
        if (m_renderer)
        {
            auto& matSystem = m_renderer->getMaterialSystem();
            for (auto& [hash, entry] : m_cache)
            {
                if (matSystem.isTextureValid(entry.textureHandle))
                {
                    matSystem.destroyTexture(entry.textureHandle);
                }
            }
        }

        m_cache.clear();
        m_pendingSet.clear();
    }

    bool ThumbnailCache::LoadThumbnailFromMeta(size_t hash)
    {
        // Mark that we attempted to load (to avoid retrying every frame)
        CacheEntry& entry = m_cache[hash];
        entry.loadAttempted = true;

        // Get the asset's compiled file path
        std::filesystem::path assetPath = GetAssetFilePath(hash);
        if (assetPath.empty())
        {
            LOG_DEBUG("[ThumbnailCache] No file path for hash {}", hash);
            return false;
        }

        // Construct meta file path
        std::filesystem::path metaPath = assetPath.string() + ".meta";

        LOG_DEBUG("[ThumbnailCache] Looking for meta: {}", metaPath.string());

        if (!std::filesystem::exists(metaPath))
        {
            LOG_DEBUG("[ThumbnailCache] Meta file not found: {}", metaPath.string());
            return false;
        }

        // Read thumbnailPath from meta
        std::string thumbnailFilename = ReadThumbnailPathFromMeta(metaPath);
        if (thumbnailFilename.empty())
        {
            LOG_DEBUG("[ThumbnailCache] No thumbnailPath in meta: {}", metaPath.string());
            return false;
        }

        // Thumbnail is in the same directory as the asset
        std::filesystem::path thumbPath = assetPath.parent_path() / thumbnailFilename;

        LOG_DEBUG("[ThumbnailCache] Thumbnail path: {}", thumbPath.string());

        if (!std::filesystem::exists(thumbPath))
        {
            LOG_DEBUG("[ThumbnailCache] Thumbnail file not found: {}", thumbPath.string());
            return false;
        }

        // Load the thumbnail texture via material system
        auto& matSystem = m_renderer->getMaterialSystem();
        gfx::TextureHandle texHandle = matSystem.loadTexture(thumbPath.string(), true);

        if (!matSystem.isTextureValid(texHandle))
        {
            LOG_WARNING("[ThumbnailCache] Failed to load thumbnail texture: {}", thumbPath.string());
            return false;
        }

        // Register for ImGui use
        uint64_t imguiId = m_renderer->registerUITexture(texHandle);

        // Update cache entry
        entry.textureHandle = texHandle;
        entry.imguiId = imguiId;
        entry.ready = true;

        LOG_INFO("[ThumbnailCache] Loaded thumbnail for hash {}: {}", hash, thumbPath.string());
        return true;
    }

    std::filesystem::path ThumbnailCache::GetAssetFilePath(size_t hash) const
    {
        auto* resourceManager = ST<MagicResourceManager>::Get();
        if (!resourceManager)
        {
            return {};
        }

        // Get the file entry from the filepaths manager
        auto& filepathsManager = resourceManager->INTERNAL_GetFilepathsManager();
        const auto* fileEntry = filepathsManager.GetFileEntry(hash);

        if (!fileEntry || fileEntry->path.empty())
        {
            return {};
        }

        // The path is relative to compiled assets, prepend the directory
        return m_compiledAssetsDir / fileEntry->path;
    }

    std::string ThumbnailCache::ReadThumbnailPathFromMeta(const std::filesystem::path& metaPath) const
    {
        std::ifstream file(metaPath);
        if (!file.is_open())
        {
            return {};
        }

        std::string line;
        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#')
                continue;

            // Look for "thumbnailPath: <value>"
            const std::string prefix = "thumbnailPath:";
            if (line.rfind(prefix, 0) == 0)
            {
                std::string value = line.substr(prefix.length());
                // Trim leading whitespace
                while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
                    value.erase(0, 1);
                return value;
            }
        }

        return {};
    }
}
