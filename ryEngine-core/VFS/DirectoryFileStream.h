#pragma once

#include "IFileStream.h"
#include <cstdio>

class DirectoryFileStream : public IFileStream
{
public:
    explicit DirectoryFileStream(FILE* fileHandle);

    ~DirectoryFileStream() override;

    DirectoryFileStream(const DirectoryFileStream&) = delete;
    DirectoryFileStream& operator=(const DirectoryFileStream&) = delete;

    // IFileStream overrides
    size_t Read(void* buffer, size_t bytesToRead) override;
    void Seek(int64_t offset, SeekOrigin origin) override;
    int64_t Tell() const override;
    int64_t GetSize() const override;
    bool IsEOF() const override;

    size_t Write(const void* buffer, size_t bytesToWrite) override;
    void Flush() override;

private:
    FILE* m_FileHandle;
};