#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceFiletypeImporterImage : public ResourceFiletypeImporterBase
{
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) final;
};
