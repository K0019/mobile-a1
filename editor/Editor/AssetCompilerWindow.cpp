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

#ifdef IMGUI_ENABLED
#include <imgui.h>
#endif

#include <filesystem>

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

    // Get output directory - use platform-specific subdirectory
    // Windows: Assets/CompiledAssets/windows/...
    // Android: Assets/CompiledAssets/android/...
    std::filesystem::path baseOutputPath = std::filesystem::absolute(Filepaths::compiledAssets);
    std::filesystem::path platformOutputPath = baseOutputPath / platform;
    std::filesystem::path assetsRoot = std::filesystem::absolute(Filepaths::assets);

    // Ensure output directory for this asset mirrors the source structure
    std::filesystem::path relativePath = std::filesystem::relative(physicalPath.parent_path(), assetsRoot);
    std::filesystem::path assetOutputDir = platformOutputPath / relativePath;
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
                CONSOLE_LOG(LEVEL_INFO) << "Asset compilation completed successfully";
            }
            else
            {
                CONSOLE_LOG(LEVEL_ERROR) << "Asset compilation failed";
            }
        }

        m_pendingUpdates->pop();
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

    if (m_isCompiling)
    {
        // Show current file
        ImGui::Text("Compiling: %s", m_currentFile.c_str());

        // Show current stage
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", m_currentStage.c_str());

        // Progress bar
        ImGui::ProgressBar(m_progress, ImVec2(-1, 0), nullptr);

        // Cancel button
        if (ImGui::Button("Cancel"))
        {
            if (m_processHandle)
                m_processHandle->Cancel();
            m_statusMessage = "Cancelling...";
        }
    }
    else
    {
        // Show last result
        if (m_progress > 0.0f)
        {
            if (m_lastCompileSuccess)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), ICON_FA_CHECK " Compilation successful");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), ICON_FA_XMARK " Compilation failed");
            }

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

} // namespace editor
