#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceFiletypeImporterKTX : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) final;

};
