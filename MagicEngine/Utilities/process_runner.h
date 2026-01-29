// utilities/process_runner.h
// Stub for process running functionality - not implemented yet
#pragma once
#include <string>

namespace util
{
    struct ProcessResult
    {
        int exitCode = -1;
        std::string output;
    };

    // Stub - not implemented yet
    inline ProcessResult RunProcess(const std::string& cmd)
    {
        (void)cmd;
        ProcessResult result;
        result.exitCode = -1;
        result.output = "Process runner not implemented";
        return result;
    }
}
