#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include "IFileStream.h"

class IVFSImpl
{
public:
    virtual ~IVFSImpl() = default;

    //virtual std::unique_ptr<IFileStream> OpenFile(const std::string& path) = 0;
    virtual std::unique_ptr<IFileStream> OpenFile(const std::string& path, FileMode mode) = 0;
    virtual bool ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer) = 0;
    virtual bool DeleteFile(const std::string& path) = 0;
    virtual bool RenameFile(const std::string& oldPath, const std::string& newPath) = 0;
    virtual bool CreateDirectory(const std::string& path) = 0;

    virtual bool FileExists(const std::string& path) const = 0;
    virtual std::vector<std::string> ListDirectory(const std::string& path) = 0;

    virtual std::string GetPhysicalPath(const std::string& path) const = 0;
    virtual std::string GetPhysicalRoot() const = 0;
};