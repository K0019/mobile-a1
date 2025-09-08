#pragma once
/******************************************************************************/
/*!
\file   Filesystem.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Class that handles file system operations for the editor. Can be extended to any class that might need to handle filesystem operations, but is currently just works in the Asset Browser for now.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

class FileSystem {
public:
    // File entry information structure
    struct FileEntry {
        std::filesystem::path fullPath;        // Full path to file/directory
        std::string filename;     // Name of file/directory
        std::string extension;    // File extension (empty for directories)
        bool isDirectory;         // Whether entry is a directory
        std::string fileType;     // General type (texture, audio, font, etc.)

        FileEntry();
    };

    FileSystem();
    ~FileSystem();
    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    // Initialization
    void Initialize(const std::filesystem::path& initialPath);
    
    // Core operations
    std::vector<FileEntry> ScanDirectory(const std::filesystem::path& directory);

    void NavigateTo(const std::filesystem::path& path);
    void NavigateBack();
    void NavigateForward();
    void NavigateUp();
    bool CanNavigateBack() const { return currentHistoryIndex > 1; }
    bool CanNavigateForward() const { return currentHistoryIndex < navigationHistory.size() - 1; }
    
    // Current state accessors
    std::filesystem::path GetCurrentPath() const { return currentPath; }
    std::filesystem::path GetRootPath() const { return currentPath.root_path(); }
    
    // Path operations
    std::filesystem::path GetAbsolutePath(const std::filesystem::path& relativePath) const;
    
    // Utility functions
    static std::string NormalizePath(const std::string& path);
    static bool FileExists(const std::filesystem::path& path);
    static std::string GetFileType(const std::filesystem::path& path);
    
private:
    static std::string DetermineFileType(const std::filesystem::path& path);
    void AddToHistory(const std::filesystem::path& path);

    std::filesystem::path currentPath;

    std::vector<std::filesystem::path> navigationHistory;
    size_t currentHistoryIndex = 0;
    
    // File type mapping
    static const inline std::unordered_map<std::string, std::string> fileTypeMap = {
        {".png", "texture"},
        {".jpg", "texture"},
        {".jpeg", "texture"},
        {".bmp", "texture"},
        {".wav", "audio"},
        {".mp3", "audio"},
        {".ogg", "audio"},
        {".ttf", "font"},
        {".otf", "font"}
    };
};