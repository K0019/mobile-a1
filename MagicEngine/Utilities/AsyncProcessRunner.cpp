// AsyncProcessRunner.cpp
#include "pch.h"
#include "AsyncProcessRunner.h"

#include <thread>
#include <mutex>
#include <queue>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

// Try to include rapidjson for progress parsing
#include <rapidjson/document.h>

namespace util
{

// ============================================================================
// Callback Queue (thread-safe)
// ============================================================================

namespace
{
    struct PendingCallback
    {
        enum class Type { Progress, Complete };
        Type type;
        ProgressCallback progressCb;
        CompletionCallback completeCb;
        ProcessProgress progress;
        AsyncProcessResult result;
    };

    std::mutex g_callbackMutex;
    std::queue<PendingCallback> g_pendingCallbacks;

    void QueueProgressCallback(ProgressCallback cb, const ProcessProgress& progress)
    {
        if (!cb) return;
        std::lock_guard<std::mutex> lock(g_callbackMutex);
        PendingCallback pending;
        pending.type = PendingCallback::Type::Progress;
        pending.progressCb = std::move(cb);
        pending.progress = progress;
        g_pendingCallbacks.push(std::move(pending));
    }

    void QueueCompletionCallback(CompletionCallback cb, const AsyncProcessResult& result)
    {
        if (!cb) return;
        std::lock_guard<std::mutex> lock(g_callbackMutex);
        PendingCallback pending;
        pending.type = PendingCallback::Type::Complete;
        pending.completeCb = std::move(cb);
        pending.result = result;
        g_pendingCallbacks.push(std::move(pending));
    }

    // Parse a JSON progress line from the compiler
    bool ParseProgressLine(const std::string& line, ProcessProgress& outProgress)
    {
        rapidjson::Document doc;
        doc.Parse(line.c_str());

        if (doc.HasParseError() || !doc.IsObject())
            return false;

        if (!doc.HasMember("type") || !doc["type"].IsString())
            return false;

        std::string type = doc["type"].GetString();
        if (type != "progress")
            return false;

        if (doc.HasMember("percent") && doc["percent"].IsNumber())
            outProgress.percent = doc["percent"].GetFloat();

        if (doc.HasMember("stage") && doc["stage"].IsString())
            outProgress.stage = doc["stage"].GetString();

        if (doc.HasMember("file") && doc["file"].IsString())
            outProgress.currentFile = doc["file"].GetString();

        if (doc.HasMember("message") && doc["message"].IsString())
            outProgress.message = doc["message"].GetString();

        return true;
    }

} // anonymous namespace

// ============================================================================
// AsyncProcessHandle
// ============================================================================

AsyncProcessHandle::~AsyncProcessHandle()
{
    if (m_future.valid())
    {
        Cancel();
        m_future.wait();
    }
}

AsyncProcessHandle::AsyncProcessHandle(AsyncProcessHandle&& other) noexcept
    : m_future(std::move(other.m_future))
    , m_cancelRequested(other.m_cancelRequested.load())
    , m_running(other.m_running.load())
#ifdef _WIN32
    , m_processHandle(other.m_processHandle)
#endif
{
#ifdef _WIN32
    other.m_processHandle = nullptr;
#endif
    other.m_running = false;
}

AsyncProcessHandle& AsyncProcessHandle::operator=(AsyncProcessHandle&& other) noexcept
{
    if (this != &other)
    {
        if (m_future.valid())
        {
            Cancel();
            m_future.wait();
        }

        m_future = std::move(other.m_future);
        m_cancelRequested = other.m_cancelRequested.load();
        m_running = other.m_running.load();
#ifdef _WIN32
        m_processHandle = other.m_processHandle;
        other.m_processHandle = nullptr;
#endif
        other.m_running = false;
    }
    return *this;
}

bool AsyncProcessHandle::IsRunning() const
{
    return m_running.load();
}

void AsyncProcessHandle::Cancel()
{
    m_cancelRequested = true;
#ifdef _WIN32
    if (m_processHandle)
    {
        TerminateProcess(m_processHandle, 1);
    }
#endif
}

AsyncProcessResult AsyncProcessHandle::Wait()
{
    if (m_future.valid())
    {
        return m_future.get();
    }
    return {};
}

bool AsyncProcessHandle::IsReady() const
{
    if (!m_future.valid())
        return true;
    return m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

// ============================================================================
// AsyncProcessRunner
// ============================================================================

AsyncProcessHandle AsyncProcessRunner::Start(
    const std::string& cmdLine,
    ProgressCallback onProgress,
    CompletionCallback onComplete)
{
    AsyncProcessHandle handle;
    handle.m_running = true;

    // Capture callbacks by value for the thread
    auto progressCb = onProgress;
    auto completeCb = onComplete;

    handle.m_future = std::async(std::launch::async, [cmdLine, progressCb, completeCb, &handle]() -> AsyncProcessResult
    {
        AsyncProcessResult result;
        result.exitCode = -1;
        result.success = false;

#ifdef _WIN32
        // Create pipes for stdout
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;

        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
        {
            result.output = "Failed to create pipe";
            QueueCompletionCallback(completeCb, result);
            return result;
        }

        SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

        // Configure process
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.hStdError = hChildStd_OUT_Wr;
        si.hStdOutput = hChildStd_OUT_Wr;
        si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        ZeroMemory(&pi, sizeof(pi));

        std::string cmdCopy = cmdLine;

        BOOL success = CreateProcessA(
            NULL,
            const_cast<char*>(cmdCopy.c_str()),
            NULL, NULL, TRUE, 0, NULL, NULL,
            &si, &pi
        );

        if (!success)
        {
            CloseHandle(hChildStd_OUT_Rd);
            CloseHandle(hChildStd_OUT_Wr);
            result.output = "Failed to create process";
            QueueCompletionCallback(completeCb, result);
            return result;
        }

        // Store process handle for cancellation
        const_cast<AsyncProcessHandle&>(handle).m_processHandle = pi.hProcess;

        // Close write end of pipe
        CloseHandle(hChildStd_OUT_Wr);
        hChildStd_OUT_Wr = NULL;

        // Read output line by line
        std::string lineBuffer;
        CHAR chBuf[256];
        DWORD dwRead;

        while (true)
        {
            // Check for cancellation
            if (handle.m_cancelRequested.load())
            {
                result.cancelled = true;
                break;
            }

            BOOL bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
            if (!bSuccess || dwRead == 0)
                break;

            chBuf[dwRead] = '\0';

            // Process characters, looking for newlines
            for (DWORD i = 0; i < dwRead; ++i)
            {
                char c = chBuf[i];
                if (c == '\n' || c == '\r')
                {
                    if (!lineBuffer.empty())
                    {
                        // Try to parse as progress JSON
                        ProcessProgress progress;
                        if (ParseProgressLine(lineBuffer, progress))
                        {
                            QueueProgressCallback(progressCb, progress);
                        }
                        else
                        {
                            // Regular output line
                            result.output += lineBuffer + "\n";
                        }
                        lineBuffer.clear();
                    }
                }
                else
                {
                    lineBuffer += c;
                }
            }
        }

        // Handle any remaining content
        if (!lineBuffer.empty())
        {
            ProcessProgress progress;
            if (ParseProgressLine(lineBuffer, progress))
            {
                QueueProgressCallback(progressCb, progress);
            }
            else
            {
                result.output += lineBuffer;
            }
        }

        // Wait for process to finish
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        result.exitCode = static_cast<int>(exitCode);
        result.success = (exitCode == 0) && !result.cancelled;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hChildStd_OUT_Rd);

        const_cast<AsyncProcessHandle&>(handle).m_processHandle = nullptr;
#endif

        const_cast<AsyncProcessHandle&>(handle).m_running = false;
        QueueCompletionCallback(completeCb, result);
        return result;
    });

    return handle;
}

void AsyncProcessRunner::PollCallbacks()
{
    std::queue<PendingCallback> toProcess;

    {
        std::lock_guard<std::mutex> lock(g_callbackMutex);
        std::swap(toProcess, g_pendingCallbacks);
    }

    while (!toProcess.empty())
    {
        auto& cb = toProcess.front();
        if (cb.type == PendingCallback::Type::Progress && cb.progressCb)
        {
            cb.progressCb(cb.progress);
        }
        else if (cb.type == PendingCallback::Type::Complete && cb.completeCb)
        {
            cb.completeCb(cb.result);
        }
        toProcess.pop();
    }
}

} // namespace util
