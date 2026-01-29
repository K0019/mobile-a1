/******************************************************************************/
/*!
\file   AssetCompilerWindow.cpp
\par    Project: Kuro Mahou
\brief  Editor window for asset compilation with progress bar.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Editor/AssetCompilerWindow.h"
#include "FilepathConstants.h"
#include "VFS/VFS.h"
#include "Utilities/Logging.h"
#include "resource/asset_compiler_interface.h"
#include "resource/asset_metadata.h"

#ifdef IMGUI_ENABLED
#include <imgui.h>
#endif

#include <filesystem>
#include <algorithm>
#include <cctype>

namespace editor
{

AssetCompilerWindow* AssetCompilerWindow::s_instance = nullptr;

AssetCompilerWindow::AssetCompilerWindow()
    : WindowBase{ ICON_FA_HAMMER " Asset Compiler", gui::Vec2{ 400, 200 } }
    , m_progressMutex(std::make_shared<std::mutex>())
    , m_pendingUpdates(std::make_shared<std::queue<ProgressUpdate>>())
{
    s_instance = this;
}

AssetCompilerWindow::~AssetCompilerWindow()
{
    if (s_instance == this)
        s_instance = nullptr;

    // Cancel any running compilation
    if (m_isCompiling && m_processHandle)
    {
        m_processHandle->Cancel();
    }
}

void AssetCompilerWindow::CompileAsset(const std::string& vfsPath, const std::string& platform)
{
    if (m_isCompiling)
    {
        CONSOLE_LOG(LEVEL_WARNING) << "Compilation already in progress";
        return;
    }

    // Convert VFS path to physical path
    std::filesystem::path physicalPath = std::filesystem::absolute(VFS::ConvertVirtualToPhysical(vfsPath));

    if (!std::filesystem::exists(physicalPath))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Asset file not found: " << physicalPath.string();
        m_statusMessage = "Error: File not found";
        return;
    }

    // Get compiler path
    std::filesystem::path compilerPath = Filepaths::GetAssetCompilerPath();
    if (!std::filesystem::exists(compilerPath))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "AssetCompiler.exe not found at: " << compilerPath.string();
        m_statusMessage = "Error: AssetCompiler.exe not found";
        return;
    }

    // Get output directory
    // Windows: compiledassets/ (direct)
    // Android: compiledassets_android/ (separate folder, copied to APK as compiledassets/)
    std::filesystem::path assetsRoot = std::filesystem::absolute(Filepaths::assets);
    std::filesystem::path baseOutputPath;
    if (platform == "android")
    {
        baseOutputPath = assetsRoot / "compiledassets_android";
    }
    else
    {
        baseOutputPath = std::filesystem::absolute(Filepaths::compiledAssets);
    }

    // Ensure output directory for this asset mirrors the source structure
    // For models (FBX, GLB), create a subdirectory with the asset name since they produce multiple outputs
    std::filesystem::path relativePath = std::filesystem::relative(physicalPath.parent_path(), assetsRoot);
    std::string ext = physicalPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::filesystem::path assetOutputDir;
    if (ext == ".fbx" || ext == ".glb" || ext == ".gltf")
    {
        // Models produce multiple files, so create a subdirectory with the asset name
        assetOutputDir = baseOutputPath / relativePath / physicalPath.stem();
    }
    else
    {
        // Textures and other single-output assets go directly in the relative directory
        assetOutputDir = baseOutputPath / relativePath;
    }
    std::filesystem::create_directories(assetOutputDir);

    // Build command line
    std::string cmdLine = "\"" + compilerPath.string() + "\"";
    cmdLine += " --input \"" + physicalPath.string() + "\"";
    cmdLine += " --output \"" + assetOutputDir.string() + "\"";
    cmdLine += " --root \"" + assetsRoot.string() + "\"";
    cmdLine += " --platform " + platform;

    CONSOLE_LOG(LEVEL_INFO) << "Starting asset compilation: " << physicalPath.filename().string();
    CONSOLE_LOG(LEVEL_DEBUG) << "Command: " << cmdLine;

    // Reset state
    m_isCompiling = true;
    m_progress = 0.0f;
    m_currentStage = "Starting...";
    m_currentFile = physicalPath.filename().string();
    m_statusMessage.clear();
    m_lastCompileSuccess = false;

    // Capture shared_ptr by value for thread safety
    auto progressMutex = m_progressMutex;
    auto pendingUpdates = m_pendingUpdates;

    // Start async compilation
    m_processHandle = std::make_shared<util::AsyncProcessHandle>(
        util::AsyncProcessRunner::Start(
            cmdLine,
            // Progress callback - captures shared state
            [progressMutex, pendingUpdates](const util::ProcessProgress& progress) -> bool
            {
                std::lock_guard<std::mutex> lock(*progressMutex);
                ProgressUpdate update;
                update.progress = progress.percent;
                update.stage = progress.stage;
                update.file = progress.currentFile;
                update.message = progress.message;
                update.isComplete = false;
                update.success = false;
                pendingUpdates->push(update);
                return true; // Continue (return false to cancel)
            },
            // Completion callback - captures shared state
            [progressMutex, pendingUpdates](const util::AsyncProcessResult& result)
            {
                std::lock_guard<std::mutex> lock(*progressMutex);
                ProgressUpdate update;
                update.progress = 1.0f;
                update.stage = result.cancelled ? "Cancelled" : (result.success ? "Success" : "Failed");
                update.isComplete = true;
                update.success = result.success;
                pendingUpdates->push(update);
            }
        )
    );
}

void AssetCompilerWindow::QueueProgressUpdate(const util::ProcessProgress& progress)
{
    std::lock_guard<std::mutex> lock(*m_progressMutex);
    ProgressUpdate update;
    update.progress = progress.percent;
    update.stage = progress.stage;
    update.file = progress.currentFile;
    update.message = progress.message;
    update.isComplete = false;
    update.success = false;
    m_pendingUpdates->push(update);
}

void AssetCompilerWindow::QueueCompletionUpdate(bool success, const std::string& message)
{
    std::lock_guard<std::mutex> lock(*m_progressMutex);
    ProgressUpdate update;
    update.progress = 1.0f;
    update.stage = message;
    update.isComplete = true;
    update.success = success;
    m_pendingUpdates->push(update);
}

void AssetCompilerWindow::ProcessPendingUpdates()
{
    bool shouldCompileNext = false;

    {
        std::lock_guard<std::mutex> lock(*m_progressMutex);

        while (!m_pendingUpdates->empty())
        {
            const auto& update = m_pendingUpdates->front();

            m_progress = update.progress;
            if (!update.stage.empty())
                m_currentStage = update.stage;
            if (!update.file.empty())
                m_currentFile = update.file;
            if (!update.message.empty())
                m_statusMessage = update.message;

            if (update.isComplete)
            {
                m_isCompiling = false;
                m_lastCompileSuccess = update.success;

                if (update.success)
                {
                    CONSOLE_LOG(LEVEL_INFO) << "Asset compilation completed successfully: " << m_currentFile;
                    if (m_isBatchCompiling)
                        m_compiledAssets++;
                }
                else
                {
                    CONSOLE_LOG(LEVEL_ERROR) << "Asset compilation failed: " << m_currentFile;
                    if (m_isBatchCompiling)
                        m_failedAssets++;
                }

                // If we're in batch mode, schedule next compilation
                if (m_isBatchCompiling)
                {
                    shouldCompileNext = true;
                }
            }

            m_pendingUpdates->pop();
        }
    }

    // Compile next asset outside the lock to avoid deadlock
    if (shouldCompileNext)
    {
        CompileNextInQueue();
    }
}

void AssetCompilerWindow::DrawWindow()
{
#ifdef IMGUI_ENABLED
    // Poll for async callbacks
    util::AsyncProcessRunner::PollCallbacks();

    // Process any pending updates from the async thread
    ProcessPendingUpdates();

    // Window content
    ImGui::Text("Asset Compiler");
    ImGui::Separator();

    if (m_isCompiling || m_isBatchCompiling)
    {
        // Batch compilation progress
        if (m_isBatchCompiling)
        {
            int processed = m_compiledAssets + m_failedAssets + m_skippedAssets;
            float batchProgress = m_totalAssets > 0 ? static_cast<float>(processed) / m_totalAssets : 0.0f;

            ImGui::Text("Batch Compiling for %s", m_batchPlatform.c_str());
            ImGui::Text("Progress: %d / %d assets", processed, m_totalAssets);

            // Overall batch progress bar
            char progressLabel[64];
            snprintf(progressLabel, sizeof(progressLabel), "%d/%d (%.0f%%)", processed, m_totalAssets, batchProgress * 100.0f);
            ImGui::ProgressBar(batchProgress, ImVec2(-1, 0), progressLabel);

            ImGui::Separator();
        }

        // Current file being compiled
        ImGui::Text("Current: %s", m_currentFile.c_str());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", m_currentStage.c_str());

        // Individual file progress bar
        ImGui::ProgressBar(m_progress, ImVec2(-1, 0), nullptr);

        // Stats during batch
        if (m_isBatchCompiling)
        {
            ImGui::Text("Compiled: %d | Skipped: %d | Failed: %d",
                m_compiledAssets, m_skippedAssets, m_failedAssets);
        }

        // Cancel button
        if (ImGui::Button("Cancel"))
        {
            if (m_processHandle)
                m_processHandle->Cancel();

            // Clear the queue to stop batch compilation
            while (!m_assetQueue.empty())
                m_assetQueue.pop();

            m_isBatchCompiling = false;
            m_statusMessage = "Cancelled";
        }
    }
    else
    {
        // Show last result
        if (m_progress > 0.0f || !m_statusMessage.empty())
        {
            if (m_lastCompileSuccess)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), ICON_FA_CHECK " Compilation successful");
            }
            else if (m_failedAssets > 0)
            {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), ICON_FA_XMARK " Compilation had failures");
            }
            else if (!m_statusMessage.empty() && m_statusMessage.find("up-to-date") != std::string::npos)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), ICON_FA_CHECK " All assets up-to-date");
            }

            if (!m_currentFile.empty())
                ImGui::Text("Last file: %s", m_currentFile.c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No compilation in progress");
        }

        // Show status message if any
        if (!m_statusMessage.empty())
        {
            ImGui::TextWrapped("%s", m_statusMessage.c_str());
        }
    }
#endif
}

std::vector<std::string> AssetCompilerWindow::FindAllSourceAssets()
{
    std::vector<std::string> assets;
    std::filesystem::path assetsRoot = std::filesystem::absolute(Filepaths::assets);

    // Supported source extensions
    std::vector<std::string> modelExtensions = { ".fbx", ".glb", ".gltf" };
    std::vector<std::string> textureExtensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };

    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    };

    try
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsRoot))
        {
            if (!entry.is_regular_file())
                continue;

            std::string path = entry.path().string();
            std::string pathLower = toLower(path);

            // Skip compiled assets folders
            if (pathLower.find("compiledassets") != std::string::npos)
                continue;

            std::string ext = toLower(entry.path().extension().string());

            // Check if it's a model file
            bool isModel = std::find(modelExtensions.begin(), modelExtensions.end(), ext) != modelExtensions.end();

            // Check if it's a texture file (only in images/ or textures/ directories)
            bool isTexture = false;
            if (std::find(textureExtensions.begin(), textureExtensions.end(), ext) != textureExtensions.end())
            {
                // Only compile standalone textures in specific directories
                std::string parentDir = toLower(entry.path().parent_path().filename().string());
                if (parentDir == "images" || parentDir == "textures")
                {
                    isTexture = true;
                }
            }

            if (isModel || isTexture)
            {
                // Convert to VFS path
                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), assetsRoot);
                assets.push_back(relativePath.generic_string());
            }
        }
    }
    catch (const std::exception& e)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Error scanning for assets: " << e.what();
    }

    return assets;
}

bool AssetCompilerWindow::ShouldCompileAsset(const std::string& sourcePath, const std::string& platform, bool forceRecompile)
{
    if (forceRecompile)
        return true;

    std::filesystem::path assetsRoot = std::filesystem::absolute(Filepaths::assets);
    std::filesystem::path physicalSourcePath = assetsRoot / sourcePath;

    if (!std::filesystem::exists(physicalSourcePath))
        return false;

    // Determine output directory
    std::filesystem::path outputBase;
    if (platform == "android")
    {
        outputBase = assetsRoot / "compiledassets_android";
    }
    else
    {
        outputBase = std::filesystem::absolute(Filepaths::compiledAssets);
    }

    // Get asset name without extension
    std::filesystem::path relativePath = std::filesystem::path(sourcePath);
    std::string baseName = relativePath.stem().string();
    std::filesystem::path assetOutputDir = outputBase / relativePath.parent_path() / baseName;

    // Check if output directory exists
    if (!std::filesystem::exists(assetOutputDir))
        return true;

    // Look for metadata files
    bool foundValidMeta = false;
    uint64_t sourceTimestamp = static_cast<uint64_t>(
        std::filesystem::last_write_time(physicalSourcePath).time_since_epoch().count());

    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(assetOutputDir))
        {
            if (entry.path().extension() == ".meta")
            {
                Resource::AssetMetadata meta;
                if (meta.loadFromFile(entry.path()))
                {
                    // Check platform matches
                    if (meta.platform != platform)
                        return true;

                    // Check timestamp matches
                    if (meta.sourceTimestamp != sourceTimestamp)
                        return true;

                    foundValidMeta = true;
                }
            }
        }
    }
    catch (const std::exception&)
    {
        return true;
    }

    return !foundValidMeta;
}

void AssetCompilerWindow::CompileAllAssets(const std::string& platform, bool forceRecompile)
{
    if (m_isCompiling || m_isBatchCompiling)
    {
        CONSOLE_LOG(LEVEL_WARNING) << "Compilation already in progress";
        return;
    }

    CONSOLE_LOG(LEVEL_INFO) << "Scanning for source assets...";

    // Find all source assets
    std::vector<std::string> allAssets = FindAllSourceAssets();

    // Filter to only assets that need compilation
    std::queue<std::string> assetsToCompile;
    int skipped = 0;

    for (const auto& asset : allAssets)
    {
        if (ShouldCompileAsset(asset, platform, forceRecompile))
        {
            assetsToCompile.push(asset);
        }
        else
        {
            skipped++;
        }
    }

    m_skippedAssets = skipped;
    m_totalAssets = static_cast<int>(assetsToCompile.size()) + skipped;
    m_compiledAssets = 0;
    m_failedAssets = 0;

    CONSOLE_LOG(LEVEL_INFO) << "Found " << allAssets.size() << " source assets, "
                            << assetsToCompile.size() << " need compilation, "
                            << skipped << " up-to-date";

    if (assetsToCompile.empty())
    {
        m_statusMessage = "All assets are up-to-date!";
        return;
    }

    m_assetQueue = std::move(assetsToCompile);
    m_batchPlatform = platform;
    m_isBatchCompiling = true;
    m_statusMessage.clear();

    // Start compiling the first asset
    CompileNextInQueue();
}

void AssetCompilerWindow::CompileNextInQueue()
{
    if (m_assetQueue.empty())
    {
        // Batch compilation complete
        m_isBatchCompiling = false;
        m_isCompiling = false;

        std::string summary = "Batch compilation complete: " +
            std::to_string(m_compiledAssets) + " compiled, " +
            std::to_string(m_skippedAssets) + " skipped, " +
            std::to_string(m_failedAssets) + " failed";

        CONSOLE_LOG(LEVEL_INFO) << summary;
        m_statusMessage = summary;
        return;
    }

    std::string nextAsset = m_assetQueue.front();
    m_assetQueue.pop();

    // Use the single-asset compile function
    CompileAsset(nextAsset, m_batchPlatform);
}

} // namespace editor
