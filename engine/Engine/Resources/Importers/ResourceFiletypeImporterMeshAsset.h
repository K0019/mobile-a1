#pragma once
#include "ResourceFiletypeImporterBase.h"
#include "ResourceFilepaths.h"

class ResourceFiletypeImporterMeshAsset : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& relativeFilepath) final;

private:
    const ResourceFilepaths::FileEntry* CreateNewFileEntry(const std::filesystem::path& relativeFilepath/*, size_t numMaterials*/);

};

