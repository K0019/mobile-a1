#pragma once
#include <string>
#include <functional>
#include <future>
#include <atomic>

namespace util
{

/**
 * @brief Progress information from a running process.
 */
struct ProcessProgress
{
    float percent = 0.0f;           // 0.0 to 1.0
    std::string stage;              // Current operation (e.g., "Compressing texture")
    std::string currentFile;        // File being processed
    std::string message;            // Optional message
};

/**
 * @brief Result of an async process execution.
 */
struct AsyncProcessResult
{
    int exitCode = -1;
    std::string output;
    bool success = false;
    bool cancelled = false;
};

/**
 * @brief Callback for progress updates.
 * @param progress Current progress information
 * @return Return false to request cancellation
 */
using ProgressCallback = std::function<bool(const ProcessProgress&)>;

/**
 * @brief Callback for process completion.
 * @param result Final result of the process
 */
using CompletionCallback = std::function<void(const AsyncProcessResult&)>;

/**
 * @brief Handle to a running async process.
 */
class AsyncProcessHandle
{
public:
    AsyncProcessHandle() = default;
    ~AsyncProcessHandle();

    // Non-copyable, movable
    AsyncProcessHandle(const AsyncProcessHandle&) = delete;
    AsyncProcessHandle& operator=(const AsyncProcessHandle&) = delete;
    AsyncProcessHandle(AsyncProcessHandle&& other) noexcept;
    AsyncProcessHandle& operator=(AsyncProcessHandle&& other) noexcept;

    /**
     * @brief Check if the process is still running.
     */
    bool IsRunning() const;

    /**
     * @brief Request cancellation of the process.
     */
    void Cancel();

    /**
     * @brief Wait for the process to complete and get the result.
     */
    AsyncProcessResult Wait();

    /**
     * @brief Check if result is ready without blocking.
     */
    bool IsReady() const;

private:
    friend class AsyncProcessRunner;

    std::future<AsyncProcessResult> m_future;
    std::atomic<bool> m_cancelRequested{ false };
    std::atomic<bool> m_running{ false };

#ifdef _WIN32
    void* m_processHandle = nullptr;  // HANDLE
#endif
};

/**
 * @brief Runs processes asynchronously with progress reporting.
 *
 * The child process should output JSON progress lines to stdout:
 *   {"type":"progress","percent":0.5,"stage":"Compressing","file":"texture.ktx2"}
 *   {"type":"complete","success":true}
 *
 * Non-JSON lines are captured as regular output.
 */
class AsyncProcessRunner
{
public:
    /**
     * @brief Start a process asynchronously.
     *
     * @param cmdLine Command line to execute
     * @param onProgress Called on main thread when progress updates arrive
     * @param onComplete Called on main thread when process completes
     * @return Handle to the running process
     */
    static AsyncProcessHandle Start(
        const std::string& cmdLine,
        ProgressCallback onProgress = nullptr,
        CompletionCallback onComplete = nullptr
    );

    /**
     * @brief Poll for pending callbacks (call from main thread each frame).
     *
     * This processes any queued progress/completion callbacks on the calling thread.
     */
    static void PollCallbacks();
};

} // namespace util
