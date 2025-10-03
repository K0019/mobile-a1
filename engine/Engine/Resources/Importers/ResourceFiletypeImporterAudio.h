#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceFiletypeImporterAudio : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) override;
};

