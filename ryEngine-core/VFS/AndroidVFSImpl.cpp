#if defined(__ANDROID__)

#include "AndroidVFSImpl.h"
#include "AndroidFileStream.h"
#include <android/asset_manager.h>

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


std::vector<std::string> AndroidVFSImpl::ListDirectory(const std::string& path)
{
    //Unimplemented.
    return std::vector<std::string>();
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