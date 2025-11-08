#include "PakFileStream.h"
#include "PakFileVFSImpl.h"

PakFileStream::PakFileStream()
{

}

size_t PakFileStream::Read([[maybe_unused]] void* buffer, [[maybe_unused]] size_t bytesToRead)
{
    return 0;
}

void PakFileStream::Seek([[maybe_unused]] int64_t offset, [[maybe_unused]] SeekOrigin origin)
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

size_t PakFileStream::Write([[maybe_unused]] const void* buffer, [[maybe_unused]] size_t bytesToWrite)
{
    // Pak is readonly
    return 0;
}

void PakFileStream::Flush()
{
    // Do nothing
}
