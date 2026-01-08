#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class SeekOrigin
{
    Begin = 0,
    Current,
    End
};

enum class FileMode
{
    Read,           // "rb"  - Open existing for reading.
    Write,          // "wb"  - Create new for writing (or truncate existing).
    Append,         // "ab"  - Open or create for writing at the end.
    ReadWrite,      // "r+b" - Open existing for reading and writing.
    CreateNew       // "wb" with a pre-check - Create for writing, but only if it doesn't already exist.
};

class IFileStream
{
public:

    virtual ~IFileStream() = default;

    virtual size_t Read(void* buffer, size_t bytesToRead) = 0;
    virtual void Seek(int64_t offset, SeekOrigin origin) = 0;
    virtual int64_t Tell() const = 0;
    virtual int64_t GetSize() const = 0;
    virtual bool IsEOF() const = 0;

    virtual size_t Write(const void* buffer, size_t bytesToWrite) = 0;
    virtual void Flush() = 0;

    std::vector<uint8_t> ReadAll()
    {
        Seek(0, SeekOrigin::Begin);
        std::vector<uint8_t> buffer(GetSize());
        Read(buffer.data(), buffer.size());
        return buffer;
    }
};