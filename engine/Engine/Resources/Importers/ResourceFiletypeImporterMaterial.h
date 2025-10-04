#pragma once
#include "ResourceFiletypeImporterBase.h"
#include "ResourceFilepaths.h"

class ResourceFiletypeImporterMaterial : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& relativeFilepath) final;

private:
    const ResourceFilepaths::FileEntry* CreateNewFileEntry(const std::filesystem::path& relativeFilepath);

};

