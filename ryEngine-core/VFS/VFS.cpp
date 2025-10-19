// VFS.cpp
#include "VFS.h"
#include "IVFSImpl.h"
#include "IFileStream.h"

#include "DirectoryVFSImpl.h"
#include "PakFileVFSImpl.h"
#if defined(__ANDROID__)
#include "AndroidVFSImpl.h"
#endif

#include <algorithm>
#include <utility>



//Helper
static std::string NormalizePath(const std::string& path)
{
    std::string lowerPath = path;
    // Convert to lowercase
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // Also replace backslashes with forward slashes for consistency
    std::replace(lowerPath.begin(), lowerPath.end(), '\\', '/');

    return lowerPath;
}

static std::string AddTrailingSlash(std::string& path)
{
    if (!path.empty() && path.back() != '/')
        path.push_back('/');
    return path;
}


std::vector<VFS::MountPoint> VFS::s_MountPoints;

void VFS::Initialize()
{
    return;
}

void VFS::Shutdown()
{
    s_MountPoints.clear();
}

#if defined(__ANDROID__)
bool VFS::MountAndroidDirectory(const std::string& virtualPath, AAssetManager* assetManager)
{
    auto directoryBackend = std::make_shared<AndroidVFSImpl>(assetManager);
    return MountBackend(virtualPath, std::move(directoryBackend));
}
#endif

bool VFS::MountDirectory(const std::string& virtualPath, const std::string& physicalPath)
{
    std::string normalizedPath = NormalizePath(virtualPath);
    AddTrailingSlash(normalizedPath);

    auto directoryBackend = std::make_shared<DirectoryVFSImpl>(physicalPath);
    return MountBackend(normalizedPath, std::move(directoryBackend));
}

bool VFS::MountArchive(const std::string& virtualPath, const std::string& physicalPath)
{
    //Implementation not ready
    return false;
}

void VFS::Unmount(const std::string& virtualPath)
{
    std::string normalizedPath = NormalizePath(virtualPath);

    auto it = std::remove_if(s_MountPoints.begin(), s_MountPoints.end(),
        [&](const MountPoint& mp) {
            return mp.virtualPath == normalizedPath;
        });
    s_MountPoints.erase(it, s_MountPoints.end());
}

// ----- File Access ----- //
std::unique_ptr<IFileStream> VFS::OpenFile(const std::string& path, FileMode mode)
{
    std::string normalizedPath = NormalizePath(path);

    // Iterate through mount points in reverse order (last-mounted is highest priority).
    for (auto it = s_MountPoints.rbegin(); it != s_MountPoints.rend(); ++it)
    {
        const MountPoint& mp = *it;

        // Check if the requested path starts with this mount point's virtual path.
        if (normalizedPath.rfind(mp.virtualPath, 0) == 0)
        {
            std::string relativePath = normalizedPath.substr(mp.virtualPath.length());

            // Trim leading slashes from the relative path if they exist.
            if (!relativePath.empty() && (relativePath[0] == '/' || relativePath[0] == '\\'))
            {
                relativePath = relativePath.substr(1);
            }

            // Ask the backend to open the file.
            auto stream = mp.backend->OpenFile(relativePath, mode);
            if (stream)
            {
                return stream;
            }
        }
    }
    // File not found in any mount point
    return nullptr;
}

bool VFS::FileExists(const std::string& path)
{
    std::string normalizedPath = NormalizePath(path);

    for (auto it = s_MountPoints.rbegin(); it != s_MountPoints.rend(); ++it)
    {
        const MountPoint& mp = *it;
        if (normalizedPath.rfind(mp.virtualPath, 0) == 0)
        {
            std::string relativePath = normalizedPath.substr(mp.virtualPath.length());
            if (!relativePath.empty() && (relativePath[0] == '/' || relativePath[0] == '\\'))
            {
                relativePath = relativePath.substr(1);
            }

            if (mp.backend->FileExists(relativePath))
            {
                return true; // Found za file
            }
        }
    }
    return false;
}

bool VFS::ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer)
{
    std::string normalizedPath = NormalizePath(path);

    for (auto it = s_MountPoints.rbegin(); it != s_MountPoints.rend(); ++it)
    {
        const MountPoint& mp = *it;
        if (normalizedPath.rfind(mp.virtualPath, 0) == 0)
        {
            std::string relativePath = normalizedPath.substr(mp.virtualPath.length());
            if (!relativePath.empty() && (relativePath[0] == '/' || relativePath[0] == '\\'))
            {
                relativePath = relativePath.substr(1);
            }
            if (mp.backend->ReadFile(relativePath, outBuffer))
            {
                return true;
            }
        }
    }
    return false;
}

bool VFS::ReadFile(const std::string& path, std::string& outStr)
{
    std::vector<uint8_t> buffer;
    if (!ReadFile(path, buffer))
    {
        return false;
    }
    outStr.assign(buffer.begin(), buffer.end());
    return true;
}

bool VFS::WriteFile(const std::string& path, const std::vector<uint8_t>& buffer)
{
    if (auto stream = VFS::OpenFile(path, FileMode::Write))
    {
        size_t bytesWritten = stream->Write(buffer.data(), buffer.size());
        return bytesWritten == buffer.size();
    }

    return false;
}

bool VFS::WriteFile(const std::string& path, const std::string& text)
{    
    if (auto stream = VFS::OpenFile(path, FileMode::Write))
    {
        size_t bytesWritten = stream->Write(text.data(), text.length());
        return bytesWritten == text.length();
    }
    return false;
}


// ----- Directories ----- //
std::vector<std::string> VFS::ListDirectory(const std::string& path)
{
    std::string normalizedPath = NormalizePath(path);

    for (auto it = s_MountPoints.rbegin(); it != s_MountPoints.rend(); ++it)
    {
        const MountPoint& mp = *it;
        if (normalizedPath.rfind(mp.virtualPath, 0) == 0)
        {
            std::string relativePath = normalizedPath.substr(mp.virtualPath.length());
            if (!relativePath.empty() && (relativePath[0] == '/' || relativePath[0] == '\\'))
            {
                relativePath = relativePath.substr(1);
            }

            auto directory = mp.backend->ListDirectory(relativePath);
            if (!directory.empty())
            {
                return directory;
            }
        }
    }
    return std::vector<std::string>();
}

// ----- Helper ----- //

bool VFS::MountBackend(const std::string& virtualPath, std::shared_ptr<IVFSImpl> backend)
{
    if (!backend)
    {
        return false;
    }

    s_MountPoints.emplace_back(MountPoint{ virtualPath, std::move(backend) });
    return true;
}




// ----- Utility ----- //
std::string VFS::JoinPath(const std::string& path1, const std::string& path2)
{
    if (path1.empty())
    {
        return path2;
    }
    if (path2.empty())
    {
        return path1;
    }

    // Accounting for cases like "/assets/" + "/textures"
    bool p1_has_slash = path1.back() == '/';
    bool p2_has_slash = path2.front() == '/';

    if (p1_has_slash && p2_has_slash)
    {
        // Remove the slash from path2
        return path1 + path2.substr(1);
    }
    else if (!p1_has_slash && !p2_has_slash)
    {
        // Neither has a slash. Add one in the middle
        return path1 + "/" + path2;
    }
    else
    {
        return path1 + path2;
    }
}

std::string VFS::GetExtension(const std::string& path)
{
    size_t dot_pos = path.find_last_of('.');
    size_t slash_pos = path.find_last_of('/');

    // Ensure the dot comes after the last slash
    if (dot_pos != std::string::npos && (slash_pos == std::string::npos || dot_pos > slash_pos))
    {
        return path.substr(dot_pos);
    }

    return ""; // No extension
}

std::string VFS::GetFilename(const std::string& path)
{
    size_t slash_pos = path.find_last_of('/');

    if (slash_pos != std::string::npos)
    {
        return path.substr(slash_pos + 1);
    }
    return path;
}

std::string VFS::GetStem(const std::string& path)
{
    std::string filename = GetFilename(path);
    size_t dot_pos = filename.find_last_of('.');

    if (dot_pos != std::string::npos)
    {
        return filename.substr(0, dot_pos);
    }

    return filename;
}

std::string VFS::GetParentPath(const std::string& path)
{
    size_t slash_pos = path.find_last_of('/');

    if (slash_pos != std::string::npos)
    {
        if (slash_pos == 0)
        {
            // Special case: path is "/filename.txt", parent is "/"
            return "/";
        }
        // Return everything before the last slash
        return path.substr(0, slash_pos);
    }

    return "";
}
