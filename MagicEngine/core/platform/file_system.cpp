// platform/file_system.cpp
#include "file_system.h"
#include "logging/log.h"
#include <cstdio>
#include <sys/stat.h>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
    #include <windows.h>
    #include <direct.h>
#else
    #include <dirent.h>
    #include <unistd.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif 

namespace Core {

std::string FileSystem::GetLocationBasePath(FileLocation location) {
    switch (location) {
        case FileLocation::Assets:
            return "./assets/";
            
        case FileLocation::UserData: {
#if PLATFORM_WINDOWS
            char* appData = nullptr;
            size_t len = 0;
            if (_dupenv_s(&appData, &len, "APPDATA") == 0 && appData) {
                std::string path = std::string(appData) + "/ryEngine/";
                free(appData);
                return path;
            }
            return "./userdata/";
#elif defined(__APPLE__)
            const char* home = getenv("HOME");
            if (home) {
                return std::string(home) + "/Library/Application Support/ryEngine/";
            }
            return "./userdata/";
#else
            const char* home = getenv("HOME");
            if (home) {
                return std::string(home) + "/.local/share/ryEngine/";
            }
            return "./userdata/";
#endif
        }
        
        case FileLocation::Cache: {
#if PLATFORM_WINDOWS
            char* temp = nullptr;
            size_t len = 0;
            if (_dupenv_s(&temp, &len, "TEMP") == 0 && temp) {
                std::string path = std::string(temp) + "/ryEngine/";
                free(temp);
                return path;
            }
            return "./cache/";
#elif defined(__APPLE__)
            const char* home = getenv("HOME");
            if (home) {
                return std::string(home) + "/Library/Caches/ryEngine/";
            }
            return "./cache/";
#else
            const char* home = getenv("HOME");
            if (home) {
                return std::string(home) + "/.cache/ryEngine/";
            }
            return "./cache/";
#endif
        }
        
        case FileLocation::External: {
#if PLATFORM_WINDOWS
            char* userProfile = nullptr;
            size_t len = 0;
            if (_dupenv_s(&userProfile, &len, "USERPROFILE") == 0 && userProfile) {
                std::string path = std::string(userProfile) + "/Documents/ryEngine/";
                free(userProfile);
                return path;
            }
            return "./documents/";
#else
            const char* home = getenv("HOME");
            if (home) {
                return std::string(home) + "/Documents/ryEngine/";
            }
            return "./documents/";
#endif
        }
    }
    
    return "./";
}

std::string FileSystem::GetBasePath(FileLocation location) {
    return GetLocationBasePath(location);
}

std::string FileSystem::GetAbsolutePath(const char* path, FileLocation location) {
    return GetLocationBasePath(location) + path;
}

bool FileSystem::ReadFile(const char* path, FileLocation location, std::vector<uint8_t>& outData) {
    std::string fullPath = GetAbsolutePath(path, location);
    
#ifdef PLATFORM_WINDOWS
    FILE* file;
    fopen_s(&file, fullPath.c_str(), "rb");
#else
    FILE* file = fopen(fullPath.c_str(), "rb");
#endif
    if (!file) {
        LOG_WARNING("Failed to open file for reading: {}", fullPath);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(file);
        return false;
    }
    
    outData.resize(size);
    size_t bytesRead = fread(outData.data(), 1, size, file);
    fclose(file);
    
    return bytesRead == static_cast<size_t>(size);
}

bool FileSystem::ReadTextFile(const char* path, FileLocation location, std::string& outText) {
    std::vector<uint8_t> data;
    if (!ReadFile(path, location, data)) {
        return false;
    }
    
    // Skip UTF-8 BOM if present
    size_t start = 0;
    if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        start = 3;
    }
    
    outText.assign(data.begin() + start, data.end());
    return true;
}

bool FileSystem::CreateDirectoryRecursive(const std::string& path) {
    size_t pos = 0;
    while ((pos = path.find_first_of("/\\", pos)) != std::string::npos) {
        std::string subPath = path.substr(0, pos++);
        if (subPath.empty()) continue;
        
#if PLATFORM_WINDOWS
        _mkdir(subPath.c_str());
#else
        mkdir(subPath.c_str(), 0755);
#endif
    }
    
#if PLATFORM_WINDOWS
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool FileSystem::WriteFile(const char* path, FileLocation location, const void* data, size_t size) {
    std::string fullPath = GetAbsolutePath(path, location);
    
    // Create directory if it doesn't exist
    size_t lastSlash = fullPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dir = fullPath.substr(0, lastSlash);
        CreateDirectoryRecursive(dir);
    }

#ifdef PLATFORM_WINDOWS
    FILE* file;
    fopen_s(&file, fullPath.c_str(), "wb");
#else
    FILE* file = fopen(fullPath.c_str(), "wb");
#endif
    if (!file) {
        LOG_WARNING("Failed to open file for writing: {}", fullPath);
        return false;
    }
    
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    
    return written == size;
}

bool FileSystem::AppendFile(const char* path, FileLocation location, const void* data, size_t size) {
    std::string fullPath = GetAbsolutePath(path, location);

#ifdef PLATFORM_WINDOWS
    FILE* file;
    fopen_s(&file, fullPath.c_str(), "ab");
#else
    FILE* file = fopen(fullPath.c_str(), "ab");
#endif
    if (!file) {
        return false;
    }
    
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    
    return written == size;
}

bool FileSystem::DeleteFile(const char* path, FileLocation location) {
    std::string fullPath = GetAbsolutePath(path, location);
    return remove(fullPath.c_str()) == 0;
}

bool FileSystem::FileExists(const char* path, FileLocation location) {
    std::string fullPath = GetAbsolutePath(path, location);
    
#if PLATFORM_WINDOWS
    struct _stat buffer;
    return _stat(fullPath.c_str(), &buffer) == 0;
#else
    struct stat buffer;
    return stat(fullPath.c_str(), &buffer) == 0;
#endif
}

uint64_t FileSystem::GetFileSize(const char* path, FileLocation location) {
    std::string fullPath = GetAbsolutePath(path, location);
    
#if PLATFORM_WINDOWS
    struct _stat buffer;
    if (_stat(fullPath.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
#else
    struct stat buffer;
    if (stat(fullPath.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
#endif
    
    return 0;
}

uint64_t FileSystem::GetFileModifiedTime(const char* path, FileLocation location) {
    std::string fullPath = GetAbsolutePath(path, location);
    
#if PLATFORM_WINDOWS
    struct _stat buffer;
    if (_stat(fullPath.c_str(), &buffer) == 0) {
        return buffer.st_mtime;
    }
#else
    struct stat buffer;
    if (stat(fullPath.c_str(), &buffer) == 0) {
        return buffer.st_mtime;
    }
#endif
    
    return 0;
}

bool FileSystem::CreateDirectory(const char* path, FileLocation location) {
    std::string fullPath = GetAbsolutePath(path, location);
    return CreateDirectoryRecursive(fullPath);
}

bool FileSystem::ListDirectory(const char* path, FileLocation location, std::vector<FileInfo>& outFiles) {
    std::string fullPath = GetAbsolutePath(path, location);
    
    outFiles.clear();
    
#if PLATFORM_WINDOWS
    WIN32_FIND_DATAA findData;
    std::string searchPath = fullPath + "/*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }
        
        FileInfo info;
        info.name = findData.cFileName;
        info.size = (static_cast<uint64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
        info.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        FILETIME ft = findData.ftLastWriteTime;
        ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;
        info.modifiedTime = ull.QuadPart / 10000000ULL - 11644473600ULL;
        
        outFiles.push_back(info);
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    return true;
#else
    DIR* dir = opendir(fullPath.c_str());
    if (!dir) {
        return false;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        FileInfo info;
        info.name = entry->d_name;
        
        std::string entryPath = fullPath + "/" + entry->d_name;
        struct stat statBuf;
        if (stat(entryPath.c_str(), &statBuf) == 0) {
            info.size = statBuf.st_size;
            info.modifiedTime = statBuf.st_mtime;
            info.isDirectory = S_ISDIR(statBuf.st_mode);
        }
        
        outFiles.push_back(info);
    }
    
    closedir(dir);
    return true;
#endif
}

} // namespace Platform