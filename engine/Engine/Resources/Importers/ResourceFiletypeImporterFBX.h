#pragma once
#include "ResourceFiletypeImporterBase.h"
#include "ResourceFilepaths.h"

class ResourceFiletypeImporterFBX : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& relativeFilepath) final;

private:
    bool CheckNumMeshesAndMaterialsAreConsistent(const ResourceFilepaths::FileEntry* fileEntry, size_t numMeshes, size_t numMaterials);
    const ResourceFilepaths::FileEntry* CreateNewFileEntry(const std::filesystem::path& relativeFilepath, size_t numMeshes, size_t numMaterials);

};

