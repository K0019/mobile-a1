#if defined(__ANDROID__)

#include "AndroidFileStream.h"
#include <android/asset_manager.h>

AndroidFileStream::AndroidFileStream(AAsset* assetHandle) : m_AssetHandle(assetHandle) {}

AndroidFileStream::~AndroidFileStream()
{
    if (m_AssetHandle)
    {
        AAsset_close(m_AssetHandle);
    }
}

size_t AndroidFileStream::Read(void* buffer, size_t bytesToRead)
{
    return AAsset_read(m_AssetHandle, buffer, bytesToRead);
}

void AndroidFileStream::Seek(int64_t offset, SeekOrigin origin)
{
    int pos;
    switch (origin)
    {
    case SeekOrigin::Begin:   pos = SEEK_SET; break;
    case SeekOrigin::Current: pos = SEEK_CUR; break;
    case SeekOrigin::End:     pos = SEEK_END; break;
    default: return;
    }
    AAsset_seek64(m_AssetHandle, offset, pos);
}

int64_t AndroidFileStream::Tell() const
{
    return GetSize() - AAsset_getRemainingLength64(m_AssetHandle);
}

int64_t AndroidFileStream::GetSize() const
{
    return AAsset_getLength64(m_AssetHandle);
}

bool AndroidFileStream::IsEOF() const
{
    return AAsset_getRemainingLength64(m_AssetHandle) == 0;
}

#endif