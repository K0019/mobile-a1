#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceImporter
{
public:
    static bool Import(const std::filesystem::path& filepath);

private:
    static std::unordered_map<std::string, SPtr<ResourceFiletypeImporterBase>> importers;

};

