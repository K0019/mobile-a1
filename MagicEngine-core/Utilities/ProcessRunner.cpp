// ProcessRunner.cpp
#include "pch.h"
#include "ProcessRunner.h"
#include <string>
#include <iostream>

#ifdef GLFW
#include <windows.h>
#endif

ProcessResult RunProcess(const std::string& cmdLine)
{
    ProcessResult result = { -1, "" };
#ifdef GLFW

    // Create a Pipe to capture the child's output
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    // Create the pipe
    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
        return result;

    // Ensure the read handle to the pipe for STDOUT is not inherited
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    // Configure the Process
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hChildStd_OUT_Wr;  // Redirect Stderr to pipe
    si.hStdOutput = hChildStd_OUT_Wr; // Redirect Stdout to pipe
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;         // Hide the black console window

    ZeroMemory(&pi, sizeof(pi));

    // Create a mutable copy of the command line (CreateProcess requires this)
    std::string cmdCopy = cmdLine;

    BOOL success = CreateProcessA(
        NULL,
        const_cast<char*>(cmdCopy.c_str()),
        NULL, NULL, TRUE, 0, NULL, NULL,
        &si, &pi
    );

    if (success)
    {
        // Close the write end of the pipe now that the child has it.
        // If we don't do this, the ReadFile loop below will never finish.
        CloseHandle(hChildStd_OUT_Wr);
        hChildStd_OUT_Wr = NULL; // Mark as closed

        // Read Output while waiting
        DWORD dwRead;
        CHAR chBuf[4096];
        bool bSuccess = FALSE;

        // Read output from the child process's pipe for STDOUT
        while (true)
        {
            bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, 4096, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) break;
            result.output.append(chBuf, dwRead);
        }

        // Wait for exit
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        result.exitCode = static_cast<int>(exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // Cleanup
    if (hChildStd_OUT_Rd) CloseHandle(hChildStd_OUT_Rd);
    if (hChildStd_OUT_Wr) CloseHandle(hChildStd_OUT_Wr); // Safety check

#endif
    return result;
}
