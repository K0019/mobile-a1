#pragma once
#include "ResourceFiletypeImporterBase.h"
#include "ResourceFilepaths.h"

class ResourceFiletypeImporterMeshAsset : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) final;

};

