#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceFiletypeImporterFBX : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) final;

};

