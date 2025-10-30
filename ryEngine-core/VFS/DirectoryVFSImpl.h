#pragma once

#include "IVFSImpl.h"
#include <string>
#include <filesystem>

class DirectoryVFSImpl : public IVFSImpl
{
public:
    explicit DirectoryVFSImpl(const std::string& rootPath);

    // IVFSImpl overrides
    std::unique_ptr<IFileStream> OpenFile(const std::string& path, FileMode mode) override;
    bool ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer) override;
    bool DeleteFile(const std::string& path) override;
    bool RenameFile(const std::string& oldPath, const std::string& newPath) override;
    bool CreateDirectory(const std::string& path) override;

    bool FileExists(const std::string& path) const override;
    std::vector<std::string> ListDirectory(const std::string& path) override;

    std::string GetPhysicalPath(const std::string& path) const override;
    std::string GetPhysicalRoot() const override;

private:
    std::filesystem::path m_RootPath;
};