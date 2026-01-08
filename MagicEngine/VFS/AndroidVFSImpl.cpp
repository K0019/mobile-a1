#if defined(__ANDROID__)
#include "VFS.h"
#include "AndroidVFSImpl.h"
#include "AndroidFileStream.h"

#include <android/asset_manager.h>
#include <android/log.h>

#include <sstream>

AndroidVFSImpl::AndroidVFSImpl(AAssetManager* assetManager) : m_AssetManager(assetManager) {}


std::unique_ptr<IFileStream> AndroidVFSImpl::OpenFile(const std::string& path, FileMode mode)
{
    // APK assets are read only
    if (mode != FileMode::Read)
    {
        return nullptr;
    }

    AAsset* assetHandle = AAssetManager_open(m_AssetManager, path.c_str(), AASSET_MODE_RANDOM);
    if (!assetHandle)
    {
        return nullptr;
    }
    return std::make_unique<AndroidFileStream>(assetHandle);
}

bool AndroidVFSImpl::FileExists(const std::string& path) const
{
    // The NDK has no "exists" function. 
    // Try to open the asset and see if it succeeds
    AAsset* assetHandle = AAssetManager_open(m_AssetManager, path.c_str(), AASSET_MODE_UNKNOWN);
    if (assetHandle)
    {
        AAsset_close(assetHandle);
        return true;
    }

    // TODO: a proper isdirectory... 
    // This fake way uses assetsmanifest to do so
    std::string manifestContent;

    if (!VFS::ReadFile("asset_manifest.txt", manifestContent))
    {
        // WHERE IS MY MANIFEST????!!!!.
        return false;
    }

    // Since this part searched for directories, we append / in case
    std::string dirPath = path;
    if (!dirPath.empty() && dirPath.back() != '/')
    {
        dirPath += '/';
    }

    std::stringstream ss(manifestContent);
    std::string line;

    while (std::getline(ss, line, '\n'))
    {
        if (line.rfind(dirPath, 0) == 0)
        {
            // Found at least one file inside this path,
            // so the directory exists.
            return true;
        }
    }


    return false;
}

bool AndroidVFSImpl::ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer)
{
    auto stream = OpenFile(path, FileMode::Read);
    if (!stream)
    {
        return false;
    }
    size_t fileSize = stream->GetSize();
    outBuffer.resize(fileSize);
    size_t bytesRead = stream->Read(outBuffer.data(), fileSize);
    return bytesRead == fileSize;
}


std::vector<std::string> AndroidVFSImpl::ListDirectory(const std::string & path)
{
    std::vector<std::string> entries;
    std::string manifestContent;

    // Manifest is always inside android/app/src/main/assets/
    if (!VFS::ReadFile("asset_manifest.txt", manifestContent))
    {
        return entries;
    }

    std::stringstream ss(manifestContent);
    std::string line;
    std::string normalizedPath = path;
    // Ensure the path ends with a slash for easier comparison
    if (!normalizedPath.empty() && normalizedPath.back() != '/')
    {
        normalizedPath += '/';
    }

    while (std::getline(ss, line, '\n'))
    {

        if (line.rfind(normalizedPath, 0) == 0)
        {
            std::string subPath = line.substr(normalizedPath.length());

            // Find the next slash. If there isn't one, it's a file.
            size_t nextSlash = subPath.find('/');
            if (nextSlash == std::string::npos)
            {
                entries.push_back(subPath);
            }
        }
    }
    return entries;
}

std::string AndroidVFSImpl::GetPhysicalPath(const std::string& path) const
{
    return "";  // idea of physical path not supported on android assets mount. it's just "", where the assets directory lives.
}

std::string AndroidVFSImpl::GetPhysicalRoot() const
{
    return "";
}



// ALL these are not supported. APK should be read only.
// Use a DirectoryFileStream if you want to write.
size_t AndroidFileStream::Write(const void* buffer, size_t bytesToWrite)
{
    return 0;
}
void AndroidFileStream::Flush() 
{ 
}
bool AndroidVFSImpl::DeleteFile(const std::string& path)
{
    return false;
}
bool AndroidVFSImpl::RenameFile(const std::string& oldPath, const std::string& newPath)
{
    return false;
}
bool AndroidVFSImpl::CreateDirectory(const std::string& path)
{
    return false;
}
#endif