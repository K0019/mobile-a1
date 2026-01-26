#include "Editor/ThumbnailCache.h"
#include "renderer/gfx_renderer.h"
#include "logging/log.h"
#include <rapidjson/document.h>
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
        while (!m_pendingQueue.empty()) m_pendingQueue.pop();
        m_activeRequest.reset();
        m_initialized = false;

        LOG_INFO("[ThumbnailCache] Shutdown");
    }

    void ThumbnailCache::Update()
    {
        if (!m_initialized || !m_renderer) return;

        // TODO: Process async generation for materials/meshes
        // For now, texture thumbnails are loaded synchronously on first request
    }

    uint64_t ThumbnailCache::GetThumbnail(size_t assetHash, AssetType type, const std::string& assetName)
    {
        if (!m_initialized || !m_renderer) return 0;

        // Check if already cached
        auto it = m_cache.find(assetHash);
        if (it != m_cache.end() && it->second.ready)
        {
            return it->second.imguiId;
        }

        // Check if already pending
        if (m_pendingSet.count(assetHash) > 0)
        {
            return 0;  // Still generating
        }

        // Handle based on asset type
        switch (type)
        {
        case AssetType::Texture:
            // Try to load from disk (synchronous for textures)
            if (LoadTextureThumbnail(assetHash, assetName))
            {
                return m_cache[assetHash].imguiId;
            }
            break;

        case AssetType::Material:
            // Try to use the material's baseColor texture thumbnail
            if (LoadMaterialThumbnail(assetHash, assetName))
            {
                return m_cache[assetHash].imguiId;
            }
            break;

        case AssetType::Mesh:
            // Mesh thumbnails not yet implemented - fall back to icons
            // TODO: Future enhancement with headless rendering
            break;
        }

        return 0;  // Not available yet
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

        // Remove from pending set if there
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
        while (!m_pendingQueue.empty()) m_pendingQueue.pop();
        m_activeRequest.reset();
    }

    bool ThumbnailCache::LoadTextureThumbnail(size_t hash, const std::string& assetName)
    {
        if (assetName.empty()) return false;

        // Construct thumbnail path: {name}_thumb.ktx2
        std::filesystem::path thumbPath = GetThumbPath(assetName);

        if (!std::filesystem::exists(thumbPath))
        {
            // Thumbnail doesn't exist - texture may not have been recompiled yet
            return false;
        }

        // Load the thumbnail texture via material system
        auto& matSystem = m_renderer->getMaterialSystem();
        gfx::TextureHandle texHandle = matSystem.loadTexture(thumbPath.string(), true);

        if (!matSystem.isTextureValid(texHandle))
        {
            LOG_WARNING("[ThumbnailCache] Failed to load thumbnail: {}", thumbPath.string());
            return false;
        }

        // Register for ImGui use
        uint64_t imguiId = m_renderer->registerUITexture(texHandle);

        // Cache it
        CacheEntry entry;
        entry.textureHandle = texHandle;
        entry.imguiId = imguiId;
        entry.ready = true;
        m_cache[hash] = entry;

        return true;
    }

    void ThumbnailCache::QueueGeneration(size_t hash, AssetType type)
    {
        // Mark as pending
        m_pendingSet.insert(hash);

        // Queue for generation
        GenerationRequest req;
        req.hash = hash;
        req.type = type;
        req.state = GenerationState::Pending;
        m_pendingQueue.push(req);

        // TODO: Actual async generation will be implemented in Phase 3/4
    }

    std::filesystem::path ThumbnailCache::GetThumbPath(const std::string& assetName) const
    {
        // The thumbnail is stored alongside the texture with _thumb suffix
        // e.g., "colormap" -> "Assets/compiledassets/colormap_thumb.ktx2"
        return m_compiledAssetsDir / (assetName + "_thumb.ktx2");
    }

    bool ThumbnailCache::LoadMaterialThumbnail(size_t hash, const std::string& materialName)
    {
        if (materialName.empty()) return false;

        // Get the baseColor texture name from the material
        std::string baseColorTextureName = GetMaterialBaseColorTextureName(materialName);
        if (baseColorTextureName.empty())
        {
            return false;
        }

        // Use the baseColor texture's thumbnail
        std::filesystem::path thumbPath = GetThumbPath(baseColorTextureName);

        if (!std::filesystem::exists(thumbPath))
        {
            return false;
        }

        // Load the thumbnail texture via material system
        auto& matSystem = m_renderer->getMaterialSystem();
        gfx::TextureHandle texHandle = matSystem.loadTexture(thumbPath.string(), true);

        if (!matSystem.isTextureValid(texHandle))
        {
            LOG_WARNING("[ThumbnailCache] Failed to load material thumbnail: {}", thumbPath.string());
            return false;
        }

        // Register for ImGui use
        uint64_t imguiId = m_renderer->registerUITexture(texHandle);

        // Cache it
        CacheEntry entry;
        entry.textureHandle = texHandle;
        entry.imguiId = imguiId;
        entry.ready = true;
        m_cache[hash] = entry;

        return true;
    }

    std::string ThumbnailCache::GetMaterialBaseColorTextureName(const std::string& materialName) const
    {
        // Material files are in Assets/compiledassets/materials/{name}.material
        std::filesystem::path materialPath = m_compiledAssetsDir / "materials" / (materialName + ".material");

        if (!std::filesystem::exists(materialPath))
        {
            return {};
        }

        // Read the material file
        std::ifstream file(materialPath);
        if (!file.is_open())
        {
            return {};
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Parse JSON
        rapidjson::Document doc;
        if (doc.Parse(content.c_str()).HasParseError())
        {
            return {};
        }

        // Find the baseColor texture in the textures array
        if (!doc.HasMember("textures") || !doc["textures"].IsArray())
        {
            return {};
        }

        const auto& textures = doc["textures"];
        for (rapidjson::SizeType i = 0; i < textures.Size(); ++i)
        {
            const auto& tex = textures[i];
            if (tex.HasMember("key") && tex["key"].IsString() &&
                tex.HasMember("value") && tex["value"].IsString())
            {
                std::string key = tex["key"].GetString();
                if (key == "baseColor")
                {
                    std::string texturePath = tex["value"].GetString();
                    if (texturePath.empty())
                    {
                        return {};
                    }

                    // Extract just the texture name from the path
                    // e.g., "compiledassets/textures/colormap.ktx2" -> "colormap"
                    std::filesystem::path texPath(texturePath);
                    std::string filename = texPath.stem().string();  // removes .ktx2
                    return filename;
                }
            }
        }

        return {};
    }
}
