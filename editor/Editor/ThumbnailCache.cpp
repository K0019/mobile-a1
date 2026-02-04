#include "Editor/ThumbnailCache.h"
#include "renderer/gfx_renderer.h"
#include "logging/log.h"
#include "Assets/AssetManager.h"
#include "resource/resource_manager.h"
#include "Utilities/Singleton.h"
#include "FilepathConstants.h"
#include <fstream>
#include <sstream>
#include <algorithm>

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

        // Cancel any running generation
        if (m_currentGeneration)
        {
            m_currentGeneration->Cancel();
            m_currentGeneration = nullptr;
        }

        // Free all loaded textures through the resource manager
        if (m_renderer)
        {
            Resource::ResourceManager* resourceMngr = m_renderer->getResourceManager();
            if (resourceMngr) {
                for (auto& [hash, entry] : m_cache)
                {
                    if (entry.textureHandle.isValid())
                    {
                        resourceMngr->freeTexture(entry.textureHandle);
                    }
                }
            }
        }

        m_cache.clear();
        m_pendingSet.clear();
        m_missingThumbnails.clear();
        while (!m_generationQueue.empty()) m_generationQueue.pop();
        m_initialized = false;

        LOG_INFO("[ThumbnailCache] Shutdown");
    }

    void ThumbnailCache::Update()
    {
        if (!m_autoGenerateEnabled || !m_initialized) return;

        // Poll for async process callbacks
        util::AsyncProcessRunner::PollCallbacks();

        // Check if current generation completed
        if (m_currentGeneration)
        {
            if (m_currentGeneration->IsReady())
            {
                auto result = m_currentGeneration->Wait();

                LOG_INFO("[ThumbnailCache] Process completed - exitCode: {}, success: {}, cancelled: {}",
                         result.exitCode, result.success, result.cancelled);

                if (!result.output.empty())
                {
                    LOG_INFO("[ThumbnailCache] Process output:\n{}", result.output);
                }

                if (result.success)
                {
                    // Clear the failed cache entry so we can retry loading
                    auto it = m_cache.find(m_currentGenerationHash);
                    if (it != m_cache.end())
                    {
                        it->second.loadAttempted = false;
                    }

                    // Reload the thumbnail
                    if (LoadThumbnailFromMeta(m_currentGenerationHash))
                    {
                        LOG_INFO("[ThumbnailCache] Generated and loaded thumbnail for hash {}", m_currentGenerationHash);
                    }
                    else
                    {
                        LOG_WARNING("[ThumbnailCache] Process succeeded but failed to load thumbnail for hash {}", m_currentGenerationHash);
                    }
                }
                else
                {
                    LOG_WARNING("[ThumbnailCache] Thumbnail generation failed for hash {} (exitCode: {})",
                                m_currentGenerationHash, result.exitCode);
                }

                m_pendingSet.erase(m_currentGenerationHash);
                m_currentGeneration = nullptr;
                m_currentGenerationHash = 0;
            }
        }

        // Start next generation if idle and queue not empty
        if (!m_currentGeneration && !m_generationQueue.empty())
        {
            size_t hash = m_generationQueue.front();
            m_generationQueue.pop();

            auto it = m_missingThumbnails.find(hash);
            if (it != m_missingThumbnails.end())
            {
                StartThumbnailGeneration(it->second);
                m_missingThumbnails.erase(it);
            }
        }

        // Queue new missing thumbnails (rate limited to avoid overwhelming the system)
        const size_t MAX_QUEUED = 10;
        if (m_generationQueue.size() < MAX_QUEUED && !m_missingThumbnails.empty())
        {
            for (auto it = m_missingThumbnails.begin();
                 it != m_missingThumbnails.end() && m_generationQueue.size() < MAX_QUEUED; )
            {
                size_t hash = it->first;

                // Check if already queued or pending
                if (m_pendingSet.count(hash) == 0)
                {
                    m_generationQueue.push(hash);
                    LOG_INFO("[ThumbnailCache] Queued thumbnail generation for: {} (hash: {})",
                             it->second.assetPath.filename().string(), hash);
                }

                ++it;
            }
        }
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
            // Free the texture through resource manager
            if (m_renderer)
            {
                Resource::ResourceManager* resourceMngr = m_renderer->getResourceManager();
                if (resourceMngr && it->second.textureHandle.isValid())
                {
                    resourceMngr->freeTexture(it->second.textureHandle);
                }
            }
            m_cache.erase(it);
        }

        m_pendingSet.erase(assetHash);
        m_missingThumbnails.erase(assetHash);
    }

    void ThumbnailCache::ClearAll()
    {
        // Cancel any running generation
        if (m_currentGeneration)
        {
            m_currentGeneration->Cancel();
            m_currentGeneration = nullptr;
        }

        if (m_renderer)
        {
            Resource::ResourceManager* resourceMngr = m_renderer->getResourceManager();
            if (resourceMngr) {
                for (auto& [hash, entry] : m_cache)
                {
                    if (entry.textureHandle.isValid())
                    {
                        resourceMngr->freeTexture(entry.textureHandle);
                    }
                }
            }
        }

        m_cache.clear();
        m_pendingSet.clear();
        m_missingThumbnails.clear();
        while (!m_generationQueue.empty()) m_generationQueue.pop();
        m_currentGenerationHash = 0;
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
            // Track for background generation if auto-generate is enabled
            if (m_autoGenerateEnabled)
            {
                TrackMissingThumbnail(hash, assetPath, metaPath);
            }
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

        // Load the thumbnail texture through the resource manager (unified path)
        // Note: sRGB=true is correct for PNG/JPEG thumbnails displayed on sRGB framebuffers.
        // The GPU does sRGB->linear on sample, shader operates in linear, framebuffer does linear->sRGB.
        Resource::ResourceManager* resourceMngr = m_renderer->getResourceManager();
        if (!resourceMngr)
        {
            LOG_WARNING("[ThumbnailCache] No resource manager available");
            return false;
        }
        ::TextureHandle texHandle = resourceMngr->loadTextureFromFile(thumbPath.string(), true);
        if (!texHandle.isValid())
        {
            LOG_WARNING("[ThumbnailCache] Failed to load thumbnail texture: {}", thumbPath.string());
            return false;
        }

        // Encode engine handle for ImGui use
        uint64_t imguiId = texHandle.getOpaqueValue();

        // Update cache entry
        entry.textureHandle = texHandle;
        entry.imguiId = imguiId;
        entry.ready = true;

        LOG_INFO("[ThumbnailCache] Loaded thumbnail for hash {}: {}", hash, thumbPath.string());
        return true;
    }

    std::filesystem::path ThumbnailCache::GetAssetFilePath(size_t hash) const
    {
        auto* resourceManager = ST<AssetManager>::Get();
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

        // The path is relative to Assets/ folder, prepend "Assets/"
        return std::filesystem::path("Assets") / fileEntry->path;
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

    ThumbnailCache::AssetType ThumbnailCache::DetermineAssetType(const std::filesystem::path& path) const
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".ktx2" || ext == ".ktx" || ext == ".png" || ext == ".jpg" || ext == ".jpeg")
        {
            return AssetType::Texture;
        }
        else if (ext == ".material")
        {
            return AssetType::Material;
        }
        else if (ext == ".mesh")
        {
            return AssetType::Mesh;
        }

        return AssetType::Texture;  // Default to texture
    }

    void ThumbnailCache::TrackMissingThumbnail(size_t hash, const std::filesystem::path& assetPath,
                                               const std::filesystem::path& metaPath)
    {
        // Don't track if already tracked or pending
        if (m_missingThumbnails.count(hash) > 0 || m_pendingSet.count(hash) > 0)
        {
            return;
        }

        MissingThumbnailInfo info;
        info.assetHash = hash;
        info.assetPath = assetPath;
        info.metaPath = metaPath;
        info.type = DetermineAssetType(assetPath);

        m_missingThumbnails[hash] = info;
        LOG_INFO("[ThumbnailCache] Tracked missing thumbnail for: {} (hash: {}, total missing: {})",
                 assetPath.filename().string(), hash, m_missingThumbnails.size());
    }

    void ThumbnailCache::StartThumbnailGeneration(const MissingThumbnailInfo& info)
    {
        // Get the asset compiler path
        std::filesystem::path compilerPath = Filepaths::GetAssetCompilerPath();

        // Resolve to absolute path for clearer logging
        std::error_code ec;
        std::filesystem::path absoluteCompilerPath = std::filesystem::absolute(compilerPath, ec);

        LOG_INFO("[ThumbnailCache] Looking for AssetCompiler at: {}", absoluteCompilerPath.string());

        if (!std::filesystem::exists(compilerPath))
        {
            LOG_ERROR("[ThumbnailCache] AssetCompiler.exe not found at: {}", absoluteCompilerPath.string());
            LOG_ERROR("[ThumbnailCache] Current working directory: {}", std::filesystem::current_path().string());
            return;
        }

        // Resolve input path to absolute as well
        std::filesystem::path absoluteInputPath = std::filesystem::absolute(info.assetPath, ec);
        std::filesystem::path absoluteOutputDir = absoluteInputPath.parent_path();

        // Build command line for thumbnail-only mode
        std::string cmdLine = "\"" + absoluteCompilerPath.string() + "\"";
        cmdLine += " --thumbnail-only";
        cmdLine += " --input \"" + absoluteInputPath.string() + "\"";
        cmdLine += " --output \"" + absoluteOutputDir.string() + "\"";

        LOG_INFO("[ThumbnailCache] Generating thumbnail for: {}", info.assetPath.filename().string());
        LOG_INFO("[ThumbnailCache] Command: {}", cmdLine);

        // Mark as pending
        m_pendingSet.insert(info.assetHash);
        m_currentGenerationHash = info.assetHash;

        // Start async process
        m_currentGeneration = std::make_shared<util::AsyncProcessHandle>(
            util::AsyncProcessRunner::Start(
                cmdLine,
                nullptr,  // No progress callback needed for thumbnail generation
                nullptr   // Completion handled in Update()
            )
        );

        LOG_INFO("[ThumbnailCache] Async process started for hash {}", info.assetHash);
    }
}
