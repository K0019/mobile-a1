/******************************************************************************/
/*!
\file   AssetCompilerWindow.h
\par    Project: Kuro Mahou
\brief  Editor window for asset compilation with progress bar.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

#include "Editor/Containers/GUIAsECS.h"
#include "Utilities/AsyncProcessRunner.h"
#include <string>
#include <queue>
#include <mutex>
#include <memory>

namespace editor
{

/**
 * @brief Editor window for displaying asset compilation progress.
 *
 * This window shows a progress bar and status messages when assets
 * are being compiled via the AssetCompiler.exe subprocess.
 */
class AssetCompilerWindow : public WindowBase<AssetCompilerWindow, false>
{
public:
    AssetCompilerWindow();
    ~AssetCompilerWindow();

    /**
     * @brief Start compiling an asset.
     * @param vfsPath VFS path to the asset (e.g., "assets/models/player.glb")
     * @param platform Target platform ("windows" or "android")
     */
    void CompileAsset(const std::string& vfsPath, const std::string& platform = "windows");

    /**
     * @brief Check if compilation is currently in progress.
     */
    bool IsCompiling() const { return m_isCompiling; }

    /**
     * @brief Get the singleton instance (for status bar access).
     */
    static AssetCompilerWindow* GetInstance() { return s_instance; }

private:
    void DrawWindow() override;

    // Compilation state
    bool m_isCompiling = false;
    float m_progress = 0.0f;
    std::string m_currentStage;
    std::string m_currentFile;
    std::string m_statusMessage;
    bool m_lastCompileSuccess = false;

    // Async process handle (shared_ptr for ECS copyability)
    std::shared_ptr<util::AsyncProcessHandle> m_processHandle;

    // Thread-safe message queue for progress updates
    struct ProgressUpdate
    {
        float progress = 0.0f;
        std::string stage;
        std::string file;
        std::string message;
        bool isComplete = false;
        bool success = false;
    };
    std::shared_ptr<std::mutex> m_progressMutex;
    std::shared_ptr<std::queue<ProgressUpdate>> m_pendingUpdates;

    // Queue a progress update from the async callback
    void QueueProgressUpdate(const util::ProcessProgress& progress);
    void QueueCompletionUpdate(bool success, const std::string& message);

    // Process queued updates (called from DrawWindow on main thread)
    void ProcessPendingUpdates();

    static AssetCompilerWindow* s_instance;
};

} // namespace editor
