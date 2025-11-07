// VFS.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include "IFileStream.h"

// Forward declarations
class IVFSImpl;

#ifdef __ANDROID__
class AAssetManager;
#endif



class VFS
{
public:
    struct MountPoint
    {
        std::string virtualPath;
        std::shared_ptr<IVFSImpl> backend;
    };

    VFS() = delete;

    static void Initialize();
    static void Shutdown();

    // ----- Adding and removing directories ----- // 
#ifdef __ANDROID__
    static bool MountAndroidDirectory(const std::string& virtualPath, AAssetManager* assetManager);
#endif
    static bool MountDirectory(const std::string& virtualPath, const std::string& physicalPath);
    static bool MountArchive(const std::string& virtualPath, const std::string& physicalPath);
    static void Unmount(const std::string& virtualPath);


    // ----- Reading, Writing files ----- // 
    static std::unique_ptr<IFileStream> OpenFile(const std::string& path, FileMode mode = FileMode::Read);
    static bool FileExists(const std::string& path);
    static bool ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer);
    static bool ReadFile(const std::string& path, std::vector<char>& outBuffer);
    static bool ReadFile(const std::string& path, std::string& outStr);

    static bool DeleteFile(const std::string& path);
    static bool RenameFile(const std::string& oldPath, const std::string& newPath);
    static bool CreateDirectory(const std::string& path);

    static bool WriteFile(const std::string& path, const std::vector<uint8_t>& buffer);
    static bool WriteFile(const std::string& path, const std::string& text);

    static std::vector<std::string> ListDirectory(const std::string& path);

    // ----- Utility functions ----- //
    // just string checking and manipulation functions...
    static std::string ConvertVirtualToPhysical(const std::string& path);
    static std::string ConvertPhysicalToVirtual(const std::string& path);

    static std::string JoinPath(const std::string& path1, const std::string& path2);
    static std::string GetExtension(const std::string& path);
    static std::string GetFilename(const std::string& path);
    static std::string GetStem(const std::string& path);
    static std::string GetParentPath(const std::string& path);
    static std::string NormalizePath(const std::string& path);

private:
    static bool MountBackend(const std::string& virtualPath, std::shared_ptr<IVFSImpl> backend);

    static std::vector<MountPoint> s_MountPoints;
};



/*
* 
    How use VFS

    VFS::Initialize(), which does nothing

    VFS::Mount(virtualPath, physicalPath);
    Use VFS::MountDirectory for windows, VFS::MountAndroidDirectory for Android.
    Virtual path is the path you will access the assets with. Physical path is the path to the directory.
    VFS::MoundAndroidDirectory assumes assets folder is same location as the android project's root folder, so no physical path parameter.

    If folder structure is as such:
    >android
      -app/src/main
       -assets
        -textures
         -mickey.png
    >Assets
     -textures
      -mickey.png
     -sounds
      -kmmwrihtdykt.wav
    >PackagedAssets
     -...

    Android build would do VFS::MountAndroidDirectory("assets", assetManager)
    Windows build would do VFS::MountDirectory("assets", "Assets")

    File Access:
        VFS::ReadFile(path)
        The first part of the path should match the virtual path used when mounting the directory. 
        Both windows and android would access the file the same way, as long as the internal directory structure is the same:
        VFS::FileExists("assets/textures/mickey.png")
        The VFS will use the according File reader (DirectoryVFSImpl / AndroidVFSImpl / PakFileVFSImpl(unimplemented)) to read these files.

    Priority:
        The LATEST mounted directory has priority, given the same virtualPath.
        VFS::MountArchive("assets", "PackackedAssets")
        VFS::ReadFile("assets/textures/mickey.png") would search the PackagedAssets first.

    AndroidVFSImpl does not support writing files. No renaming, no deleting, no writing.
*/



