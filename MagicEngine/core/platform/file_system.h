// platform/file_system.h
#pragma once

#include "platform_types.h"
#include <vector>
#include <string>
#include <cstddef>

namespace Core {

class FileSystem {
public:
    static bool ReadFile(const char* path, FileLocation location, std::vector<uint8_t>& outData);
    static bool ReadTextFile(const char* path, FileLocation location, std::string& outText);
    static bool WriteFile(const char* path, FileLocation location, const void* data, size_t size);
    static bool AppendFile(const char* path, FileLocation location, const void* data, size_t size);
    static bool DeleteFile(const char* path, FileLocation location);
    static bool FileExists(const char* path, FileLocation location);
    static uint64_t GetFileSize(const char* path, FileLocation location);
    static uint64_t GetFileModifiedTime(const char* path, FileLocation location);
    static bool CreateDirectory(const char* path, FileLocation location);
    static bool ListDirectory(const char* path, FileLocation location, std::vector<FileInfo>& outFiles);
    static std::string GetBasePath(FileLocation location);
    static std::string GetAbsolutePath(const char* path, FileLocation location);

private:
    static std::string GetLocationBasePath(FileLocation location);
    static bool CreateDirectoryRecursive(const std::string& path);
};

} // namespace Core
