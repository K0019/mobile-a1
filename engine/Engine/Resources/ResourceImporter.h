#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceImporter
{
public:
    static bool Import(const std::filesystem::path& filepath);
    static bool FiletypeSupported(const std::string& extension);

private:
    static std::unordered_map<std::string, SPtr<ResourceFiletypeImporterBase>> importers;

};

