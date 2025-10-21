#include "PakFileStream.h"
#include "PakFileVFSImpl.h"

PakFileStream::PakFileStream()
{

}

size_t PakFileStream::Read(void* buffer, size_t bytesToRead)
{
    return 0;
}

void PakFileStream::Seek(int64_t offset, SeekOrigin origin)
{
    return;
}

int64_t PakFileStream::Tell() const
{
    return 0;
}

int64_t PakFileStream::GetSize() const
{
    return 0;
}

bool PakFileStream::IsEOF() const
{
    return false;
}

size_t PakFileStream::Write(const void* buffer, size_t bytesToWrite)
{
    // Pak is readonly
    return 0;
}

void PakFileStream::Flush()
{
    // Do nothing
}
