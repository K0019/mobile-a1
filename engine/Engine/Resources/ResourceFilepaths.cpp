#include "ResourceFilepaths.h"

const ResourceFilepaths::FileEntry* ResourceFilepaths::SetFilepath(const std::filesystem::path& filepath, std::vector<AssociatedResourceHashes>&& associatedHashes)
{
    FileEntry* fileEntry{};

    // If the file is already registered, overwrite the previous entry.
    size_t filepathHash{ util::GenHash(filepath.string()) };
    const auto existingFileEntryIter{ filepathToFileEntry.find(filepathHash) };
    if (existingFileEntryIter != filepathToFileEntry.end())
    {
        fileEntry = existingFileEntryIter->second;
        for (const auto& associatedResource : fileEntry->associatedResources)
            for (size_t hash : associatedResource.hashes)
                hashToFileEntry.erase(hash);
        fileEntry->associatedResources = std::move(associatedHashes);
    }
    else
    {
        // Otherwise, create a new FileEntry
        fileEntry = &fileEntries.emplace_back(filepath, std::move(associatedHashes));
        filepathToFileEntry.try_emplace(filepathHash, fileEntry);
    }

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

const ResourceFilepaths::FileEntry* ResourceFilepaths::GetFileEntry(const std::filesystem::path& filepath) const
{
    auto fileEntryLookupIter{ filepathToFileEntry.find(util::GenHash(filepath.string())) };
    return (fileEntryLookupIter != filepathToFileEntry.end() ? fileEntryLookupIter->second : nullptr);
}

const std::vector<ResourceFilepaths::FileEntry>& ResourceFilepaths::GetFileEntries() const
{
    return fileEntries;
}
