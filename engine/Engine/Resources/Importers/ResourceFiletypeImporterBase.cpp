#include "ResourceFiletypeImporterBase.h"
#include "ResourceManager.h"

size_t ResourceFiletypeImporterBase::GenerateNewHash()
{
    size_t hash{ util::Rand_UID() };
    while (ST<ResourceManager>::Get()->INTERNAL_GetNamesManager().GetName(hash))
        hash = util::Rand_UID();
    CONSOLE_LOG(LEVEL_INFO) << "Generated " << hash;
    return hash;
}

std::filesystem::path ResourceFiletypeImporterBase::GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath)
{
    return ST<Filepaths>::Get()->assets / assetRelativeFilepath;
}

void ResourceFiletypeImporterBase::GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::filesystem::path& relativeFilepath)
{
    auto& namesManager{ ST<ResourceManager>::Get()->INTERNAL_GetNamesManager() };
    std::string filename{ relativeFilepath.stem().string() };

    for (const auto& resource : resources)
        for (size_t i{}; i < resource.hashes.size(); ++i)
            namesManager.SetName(resource.hashes[i], filename + std::to_string(i));
}
