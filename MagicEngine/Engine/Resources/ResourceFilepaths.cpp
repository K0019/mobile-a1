/******************************************************************************/
/*!
\file   ResourceFilepaths.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Stores the associations of resource hashes with the files that will load them.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/ResourceFilepaths.h"

const ResourceFilepaths::FileEntry* ResourceFilepaths::SetFilepath(const std::string& filepath, std::vector<AssociatedResourceHashes>&& associatedHashes)
{
    FileEntry* fileEntry{};

    // If the file is already registered, overwrite the previous entry.
    size_t filepathHash{ util::GenHash(filepath) };
    const auto existingFileEntryIter{ fileEntries.find(filepathHash) };
    if (existingFileEntryIter != fileEntries.end())
    {
        fileEntry = &existingFileEntryIter->second;
        for (const auto& associatedResource : fileEntry->associatedResources)
            for (size_t hash : associatedResource.hashes)
                hashToFileEntry.erase(hash);
        fileEntry->associatedResources = std::move(associatedHashes);
    }
    else
        // Otherwise, create a new FileEntry
        fileEntry = &fileEntries.try_emplace(filepathHash, filepath, std::move(associatedHashes)).first->second;

    // Set resource hashes to point to this FileEntry
    for (const auto& associatedResource : fileEntry->associatedResources)
        for (size_t hash : associatedResource.hashes)
            hashToFileEntry[hash] = fileEntry;

    return fileEntry;
}

const ResourceFilepaths::FileEntry* ResourceFilepaths::GetFileEntry(size_t hash)
{
    auto fileEntryIter{ hashToFileEntry.find(hash) };
    return (fileEntryIter != hashToFileEntry.end() ? fileEntryIter->second : nullptr);
}

const ResourceFilepaths::FileEntry* ResourceFilepaths::GetFileEntry(const std::string& filepath) const
{
    auto fileEntryLookupIter{ fileEntries.find(util::GenHash(filepath)) };
    return (fileEntryLookupIter != fileEntries.end() ? &fileEntryLookupIter->second : nullptr);
}

std::vector<const ResourceFilepaths::FileEntry*> ResourceFilepaths::GetFileEntries() const
{
    // This is only called when saving so it's ok to be a bit slow here
    auto sortedFileEntries{ util::ToSortedVectorOfRefs(fileEntries) };
    std::vector<const FileEntry*> toReturn;
    std::transform(sortedFileEntries.begin(), sortedFileEntries.end(), std::back_inserter(toReturn), [](const auto& pair) -> const FileEntry* {
        return &pair.second.get();
    });
    return toReturn;
}

void ResourceFilepaths::DisassociateResourceHash(size_t resourceHash, size_t resourceType)
{
    auto fileEntryIter{ hashToFileEntry.find(resourceHash) };
    if (fileEntryIter == hashToFileEntry.end())
        return;

    // Remove the resource hash mapping.
    FileEntry* fileEntry{ fileEntryIter->second };
    hashToFileEntry.erase(fileEntryIter);

    // Remove the association with the file entry.
    auto associatedResourcesVecIter{ std::find_if(fileEntry->associatedResources.begin(), fileEntry->associatedResources.end(), [resourceType](const AssociatedResourceHashes& association) -> bool {
        return association.resourceTypeHash == resourceType;
    }) };
    std::erase(associatedResourcesVecIter->hashes, resourceHash);
    if (associatedResourcesVecIter->hashes.empty())
        fileEntry->associatedResources.erase(associatedResourcesVecIter);
    // If there are still other resources associated with the file entry, leave the file entry be
    if (!fileEntry->associatedResources.empty())
        return;

    // Remove the file entry
    size_t fileEntryHash{ util::GenHash(fileEntry->path) };
    fileEntries.erase(fileEntryHash);
}
