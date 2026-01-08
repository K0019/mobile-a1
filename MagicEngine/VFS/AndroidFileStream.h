#pragma once
#if defined(__ANDROID__)

#include "IFileStream.h"

struct AAsset;

class AndroidFileStream : public IFileStream
{
public:
    explicit AndroidFileStream(AAsset* assetHandle);
    ~AndroidFileStream() override;

    AndroidFileStream(const AndroidFileStream&) = delete;
    AndroidFileStream& operator=(const AndroidFileStream&) = delete;

    size_t Read(void* buffer, size_t bytesToRead) override;
    void Seek(int64_t offset, SeekOrigin origin) override;
    int64_t Tell() const override;
    int64_t GetSize() const override;
    bool IsEOF() const override;

    size_t Write(const void* buffer, size_t bytesToWrite) override;
    void Flush() override;

private:
    AAsset* m_AssetHandle;
};
#endif