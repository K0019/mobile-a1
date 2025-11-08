#define _CRT_SECURE_NO_WARNINGS
#include "DirectoryVFSImpl.h"
#include "DirectoryFileStream.h"
#include <iostream>

DirectoryVFSImpl::DirectoryVFSImpl(const std::string& rootPath) : m_RootPath(rootPath) {}

std::unique_ptr<IFileStream> DirectoryVFSImpl::OpenFile(const std::string& path, FileMode mode)
{
    std::filesystem::path fullPath = m_RootPath / path;

    // Ensure the directory exists before trying to write a file
    if (mode == FileMode::Write || mode == FileMode::Append)
    {
        if (fullPath.has_parent_path())
        {
            std::filesystem::create_directories(fullPath.parent_path());
        }
    }

    const char* modeStr = "rb";
    switch (mode)
    {
    case FileMode::Read:        modeStr = "rb";  break;
    case FileMode::Write:       modeStr = "wb";  break;
    case FileMode::Append:      modeStr = "ab";  break;
    case FileMode::ReadWrite:   modeStr = "r+b"; break;
    case FileMode::CreateNew:
        if (FileExists(path)) return nullptr; // Fail if it exists
        modeStr = "wb";
        break;
    }

    FILE* fileHandle = fopen(fullPath.string().c_str(), modeStr);

    if (!fileHandle)
    {
        return nullptr;
    }
    return std::make_unique<DirectoryFileStream>(fileHandle);
}

bool DirectoryVFSImpl::ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer)
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

bool DirectoryVFSImpl::DeleteFile(const std::string& path)
{
    return std::filesystem::remove(m_RootPath / path);
}

bool DirectoryVFSImpl::RenameFile(const std::string& oldPath, const std::string& newPath)
{
    try
    {
        std::filesystem::rename(m_RootPath / oldPath, m_RootPath / newPath);
    }
    catch(const std::filesystem::filesystem_error&)
    {
        return false;
    }
    return true;
}

bool DirectoryVFSImpl::CreateDirectory(const std::string& path)
{
    return std::filesystem::create_directory(m_RootPath / path);
}


bool DirectoryVFSImpl::FileExists(const std::string& path) const
{
    std::filesystem::path fullPath = m_RootPath / path;

    return std::filesystem::exists(fullPath);
    //return std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath);
}


std::vector<std::string> DirectoryVFSImpl::ListDirectory(const std::string& path)
{
    std::vector<std::string> entries;
    std::filesystem::path fullPath = m_RootPath / path;

    for (const auto& entry : std::filesystem::directory_iterator(fullPath))
    {
        entries.push_back(entry.path().filename().string());
    }
    return entries;
}

std::string DirectoryVFSImpl::GetPhysicalPath(const std::string& path) const
{
    return (m_RootPath / path).string();
}

std::string DirectoryVFSImpl::GetPhysicalRoot() const
{
    return m_RootPath.string();
}
