#pragma once
struct ProcessResult
{
    int exitCode;
    std::string output;
};

ProcessResult RunProcess(const std::string& cmdLine);


// Self notes:
//https://learn.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output
//https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
//https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa



