#pragma once
#include "Engine/Resources/Importers/ResourceFiletypeImporterBase.h"
#include "Engine/Resources/ResourceFilepaths.h"

class ResourceFiletypeImporterMaterial : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::string& relativeFilepath) final;

};

