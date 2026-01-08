#pragma once
#include "Engine/Resources/Importers/ResourceFiletypeImporterBase.h"

class ResourceFiletypeImporterImage : public ResourceFiletypeImporterBase
{
    virtual bool Import(const std::string& assetRelativeFilepath) final;
};
