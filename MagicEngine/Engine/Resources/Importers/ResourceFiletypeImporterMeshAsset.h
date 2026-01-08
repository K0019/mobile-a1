#pragma once
#include "Engine/Resources/Importers/ResourceFiletypeImporterBase.h"
#include "Engine/Resources/ResourceFilepaths.h"

class ResourceFiletypeImporterMeshAsset : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::string& assetRelativeFilepath) final;

};

