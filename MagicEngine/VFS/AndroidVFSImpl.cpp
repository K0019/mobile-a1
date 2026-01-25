#if defined(__ANDROID__)
#include "VFS.h"
#include "AndroidVFSImpl.h"
#include "AndroidFileStream.h"

#include <android/asset_manager.h>
#include <android/log.h>

#include <sstream>
#include <algorithm>
#include <cctype>

AndroidVFSImpl::AndroidVFSImpl(AAssetManager* assetManager) : m_AssetManager(assetManager) {}

// Convert string to lowercase for case-insensitive comparison
std::string AndroidVFSImpl::ToLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Build lookup table from manifest (lowercase path -> actual path)
void AndroidVFSImpl::BuildPathLookup() const
{
    if (m_PathLookupBuilt) return;

    // Read manifest directly from asset manager to avoid recursion
    AAsset* manifestAsset = AAssetManager_open(m_AssetManager, "asset_manifest.txt", AASSET_MODE_BUFFER);
    if (!manifestAsset)
    {
        m_PathLookupBuilt = true;
        return;
    }

    const char* data = static_cast<const char*>(AAsset_getBuffer(manifestAsset));
    off_t size = AAsset_getLength(manifestAsset);
    std::string manifestContent(data, size);
    AAsset_close(manifestAsset);

    std::stringstream ss(manifestContent);
    std::string line;
    while (std::getline(ss, line, '\n'))
    {
        // Trim whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty()) continue;

        std::string lowerPath = ToLower(line);
        m_PathLookup[lowerPath] = line;
    }

    m_PathLookupBuilt = true;
    __android_log_print(ANDROID_LOG_INFO, "MagicEngine", "Built path lookup with %zu entries", m_PathLookup.size());
}

// Resolve a path case-insensitively
std::string AndroidVFSImpl::ResolvePath(const std::string& path) const
{
    // First try exact match (fast path)
    AAsset* asset = AAssetManager_open(m_AssetManager, path.c_str(), AASSET_MODE_UNKNOWN);
    if (asset)
    {
        AAsset_close(asset);
        return path;
    }

    // Build lookup if not already done
    BuildPathLookup();

    // Try case-insensitive lookup
    std::string lowerPath = ToLower(path);
    auto it = m_PathLookup.find(lowerPath);
    if (it != m_PathLookup.end())
    {
        return it->second;
    }

    // Return original path (will fail, but preserves error reporting)
    return path;
}

std::unique_ptr<IFileStream> AndroidVFSImpl::OpenFile(const std::string& path, FileMode mode)
{
    // APK assets are read only
    if (mode != FileMode::Read)
    {
        return nullptr;
    }

    std::string resolvedPath = ResolvePath(path);
    AAsset* assetHandle = AAssetManager_open(m_AssetManager, resolvedPath.c_str(), AASSET_MODE_RANDOM);
    if (!assetHandle)
    {
        return nullptr;
    }
    return std::make_unique<AndroidFileStream>(assetHandle);
}

bool AndroidVFSImpl::FileExists(const std::string& path) const
{
    // Try case-insensitive file lookup first
    std::string resolvedPath = ResolvePath(path);
    AAsset* assetHandle = AAssetManager_open(m_AssetManager, resolvedPath.c_str(), AASSET_MODE_UNKNOWN);
    if (assetHandle)
    {
        AAsset_close(assetHandle);
        return true;
    }

    // Check if it's a directory by looking for files under this path
    BuildPathLookup();

    std::string lowerDirPath = ToLower(path);
    if (!lowerDirPath.empty() && lowerDirPath.back() != '/')
    {
        lowerDirPath += '/';
    }

    for (const auto& entry : m_PathLookup)
    {
        if (entry.first.rfind(lowerDirPath, 0) == 0)
        {
            // Found at least one file inside this path, so directory exists
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