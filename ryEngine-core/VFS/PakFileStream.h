#pragma once

#include "IFileStream.h"

class PakFileStream : public IFileStream
{
public:
    PakFileStream();
    ~PakFileStream() override = default;

    size_t Read(void* buffer, size_t bytesToRead) override;
    void Seek(int64_t offset, SeekOrigin origin) override;
    int64_t Tell() const override;
    int64_t GetSize() const override;
    bool IsEOF() const override;

    size_t Write(const void* buffer, size_t bytesToWrite) override;
    void Flush() override;

private:

};