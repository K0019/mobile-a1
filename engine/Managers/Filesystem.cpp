/******************************************************************************/
/*!
\file   Filesystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of Filesystem class.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "FileSystem.h"

namespace fs = std::filesystem;

FileSystem::FileEntry::FileEntry() : isDirectory(false) {}

FileSystem::FileSystem() {}

FileSystem::~FileSystem() {}

void FileSystem::Initialize(const fs::path& initialPath) {
    try {
        currentPath = fs::absolute(initialPath);
        navigationHistory.clear();
        AddToHistory(currentPath);
    }
    catch (const fs::filesystem_error& e) {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to initialize filesystem: " << e.what();
    }
}

std::vector<FileSystem::FileEntry> FileSystem::ScanDirectory(const fs::path& directory) 
{
    std::vector<FileEntry> results;
    
    try {
        if(!exists(directory) || !is_directory(directory)) {
            CONSOLE_LOG(LEVEL_ERROR) << "Invalid directory path: " << directory.string();
            return results;
        }

        for(const auto& entry : fs::directory_iterator(directory)) {
            FileEntry& fileEntry{ results.emplace_back() };
            fileEntry.fullPath = entry.path();
            fileEntry.filename = entry.path().filename().string();
            fileEntry.extension = entry.path().extension().string();
            fileEntry.isDirectory = entry.is_directory();
            fileEntry.fileType = (!fileEntry.isDirectory ? DetermineFileType(fileEntry.fullPath) : "");
        }

        // Sort: directories first, then files alphabetically
        std::ranges::sort(results, [](const FileEntry& a, const FileEntry& b) {
            if(a.isDirectory != b.isDirectory) {
                return a.isDirectory > b.isDirectory;
            }
            return a.filename < b.filename;
        });
    } 
    catch(const fs::filesystem_error& e) {
        CONSOLE_LOG(LEVEL_ERROR) << "Filesystem error while scanning directory: " << e.what();
    }

    return results;
}

void FileSystem::NavigateTo(const fs::path& path) {
    fs::path targetPath = absolute(path);

    if(!exists(targetPath) || !is_directory(targetPath)) {
        CONSOLE_LOG(LEVEL_ERROR) << "Invalid navigation target: " << targetPath.string();
        return;
    }

    currentPath = targetPath;
    AddToHistory(currentPath);
}

void FileSystem::NavigateBack() {
    if(CanNavigateBack()) {
        currentHistoryIndex--;
        currentPath = navigationHistory[currentHistoryIndex];
    }
}

void FileSystem::NavigateForward() {
    if(CanNavigateForward()) {
        currentHistoryIndex++;
        currentPath = navigationHistory[currentHistoryIndex];
    }
}

void FileSystem::NavigateUp() {
    if(currentPath.has_parent_path()) {
        NavigateTo(currentPath.parent_path());
    }
}

fs::path FileSystem::GetAbsolutePath(const fs::path& relativePath) const {
    return absolute(relativePath);
}

std::string FileSystem::NormalizePath(const std::string& path) {
    fs::path normalized = path;
    return normalized.make_preferred().string();
}

bool FileSystem::FileExists(const fs::path& path) {
    try {
        return exists(path) && is_regular_file(path);
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

std::string FileSystem::GetFileType(const fs::path& path) {
    return DetermineFileType(path);
}

std::string FileSystem::DetermineFileType(const fs::path& path) {
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), util::ToLower);
    
    auto it = fileTypeMap.find(ext);
    return it != fileTypeMap.end() ? it->second : "unknown";
}

void FileSystem::AddToHistory(const fs::path& path) {
    if(currentHistoryIndex < navigationHistory.size() - 1) {
        navigationHistory.resize(currentHistoryIndex + 1);
    }
    
    // Add new path if it's different from current
    if(navigationHistory.empty() || navigationHistory.back() != path) {
        navigationHistory.push_back(path);
        currentHistoryIndex = navigationHistory.size() - 1;
    }
}


