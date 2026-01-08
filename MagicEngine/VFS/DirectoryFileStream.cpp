#include "DirectoryFileStream.h"
#include <stdexcept>

DirectoryFileStream::DirectoryFileStream(FILE* fileHandle) : m_FileHandle(fileHandle)
{
    if (!m_FileHandle)
    {

    }
}

DirectoryFileStream::~DirectoryFileStream()
{
    if (m_FileHandle)
    {
        fclose(m_FileHandle);
    }
}

size_t DirectoryFileStream::Read(void* buffer, size_t bytesToRead)
{
    return fread(buffer, 1, bytesToRead, m_FileHandle);
}

void DirectoryFileStream::Seek(int64_t offset, SeekOrigin origin)
{
    int pos;
    switch (origin)
    {
    case SeekOrigin::Begin:   pos = SEEK_SET; break;
    case SeekOrigin::Current: pos = SEEK_CUR; break;
    case SeekOrigin::End:     pos = SEEK_END; break;
    default: return;
    }
    fseek(m_FileHandle, static_cast<long>(offset), pos);
}

int64_t DirectoryFileStream::Tell() const
{
    // On Windows, _ftelli64 can be used to work with files larger than 2 GiB.
    return ftell(m_FileHandle);
}

bool DirectoryFileStream::IsEOF() const
{
    return feof(m_FileHandle) != 0;
}

int64_t DirectoryFileStream::GetSize() const
{
    FILE* handle = const_cast<FILE*>(m_FileHandle);

    // Store current position
    long currentPos = ftell(handle);

    fseek(handle, 0, SEEK_END);
    long size = ftell(handle);

    // Restore original position
    fseek(handle, currentPos, SEEK_SET);

    return size;
}

size_t DirectoryFileStream::Write(const void* buffer, size_t bytesToWrite)
{
    return fwrite(buffer, 1, bytesToWrite, m_FileHandle);
}

void DirectoryFileStream::Flush()
{
    fflush(m_FileHandle);
}
