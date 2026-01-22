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
Extended to support asset type queries and source tracking.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/ResourceFilepaths.h"
#include "resource/asset_metadata.h"
#include "VFS/VFS.h"
#include <algorithm>
#include <filesystem>

const ResourceFilepaths::FileEntry* ResourceFilepaths::SetFilepath(const std::string& filepath, std::vector<AssociatedResourceHashes>&& associatedHashes)
{
    FileEntry* fileEntry{};

    // If the file is already registered, overwrite the previous entry.
    size_t filepathHash{ util::GenHash(filepath) };
    const auto existingFileEntryIter{ fileEntries.find(filepathHash) };
    if (existingFileEntryIter != fileEntries.end())
    {
        fileEntry = &existingFileEntryIter->second;

        // Clear old hash mappings
        for (const auto& associatedResource : fileEntry->associatedResources)
            for (size_t hash : associatedResource.hashes)
                hashToFileEntry.erase(hash);

        // Clear old source mapping if it exists
        if (!fileEntry->sourcePath.empty())
        {
            size_t sourceHash = util::GenHash(fileEntry->sourcePath);
            auto range = sourceToFileEntries.equal_range(sourceHash);
            for (auto it = range.first; it != range.second; )
            {
                if (it->second == fileEntry)
                    it = sourceToFileEntries.erase(it);
                else
                    ++it;
            }
        }

        fileEntry->associatedResources = std::move(associatedHashes);
    }
    else
    {
        // Otherwise, create a new FileEntry
        fileEntry = &fileEntries.try_emplace(filepathHash, filepath, std::move(associatedHashes)).first->second;
    }

    // Set resource hashes to point to this FileEntry
    for (const auto& associatedResource : fileEntry->associatedResources)
        for (size_t hash : associatedResource.hashes)
            hashToFileEntry[hash] = fileEntry;

    // Load metadata from .meta file if available
    LoadMetadataForEntry(*fileEntry);

    // If asset type is still unknown, infer from extension
    if (fileEntry->assetType == AssetFormat::AssetType::Unknown)
        fileEntry->assetType = InferAssetType(filepath);

    // Update source mapping
    if (!fileEntry->sourcePath.empty())
    {
        size_t sourceHash = util::GenHash(fileEntry->sourcePath);
        sourceToFileEntries.emplace(sourceHash, fileEntry);
    }

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
    if (associatedResourcesVecIter == fileEntry->associatedResources.end())
        return;  // Resource type not found - nothing to disassociate

    std::erase(associatedResourcesVecIter->hashes, resourceHash);
    if (associatedResourcesVecIter->hashes.empty())
        fileEntry->associatedResources.erase(associatedResourcesVecIter);
    // If there are still other resources associated with the file entry, leave the file entry be
    if (!fileEntry->associatedResources.empty())
        return;

    // Remove source mapping if exists
    if (!fileEntry->sourcePath.empty())
    {
        size_t sourceHash = util::GenHash(fileEntry->sourcePath);
        auto range = sourceToFileEntries.equal_range(sourceHash);
        for (auto it = range.first; it != range.second; )
        {
            if (it->second == fileEntry)
                it = sourceToFileEntries.erase(it);
            else
                ++it;
        }
    }

    // Remove the file entry
    size_t fileEntryHash{ util::GenHash(fileEntry->path) };
    fileEntries.erase(fileEntryHash);
}

// ============================================================================
// Extended Query Methods
// ============================================================================

std::vector<const ResourceFilepaths::FileEntry*> ResourceFilepaths::GetFileEntriesByType(AssetFormat::AssetType type) const
{
    std::vector<const FileEntry*> result;
    for (const auto& [hash, entry] : fileEntries)
    {
        if (entry.assetType == type)
            result.push_back(&entry);
    }
    return result;
}

const ResourceFilepaths::FileEntry* ResourceFilepaths::GetFileEntryBySourcePath(const std::string& sourcePath) const
{
    size_t sourceHash = util::GenHash(sourcePath);
    auto it = sourceToFileEntries.find(sourceHash);
    if (it != sourceToFileEntries.end())
        return it->second;
    return nullptr;
}

std::vector<const ResourceFilepaths::FileEntry*> ResourceFilepaths::GetFileEntriesFromSource(const std::string& sourcePath) const
{
    std::vector<const FileEntry*> result;
    size_t sourceHash = util::GenHash(sourcePath);
    auto range = sourceToFileEntries.equal_range(sourceHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        result.push_back(it->second);
    }
    return result;
}

// ============================================================================
// Metadata Management
// ============================================================================

void ResourceFilepaths::LoadMetadataForEntry(FileEntry& entry)
{
    // Try to load .meta sidecar file
    std::filesystem::path metaPath = Resource::AssetMetadata::getMetaPath(
        VFS::ConvertVirtualToPhysical(entry.path));

    Resource::AssetMetadata meta;
    if (meta.loadFromFile(metaPath))
    {
        entry.assetType = meta.assetType;
        entry.sourcePath = meta.sourcePath;
        entry.sourceTimestamp = meta.sourceTimestamp;
        entry.compiledTimestamp = meta.compiledTimestamp;
        entry.formatVersion = meta.formatVersion;
    }
}

void ResourceFilepaths::RefreshAllMetadata()
{
    // Clear source mappings - will be rebuilt
    sourceToFileEntries.clear();

    for (auto& [hash, entry] : fileEntries)
    {
        LoadMetadataForEntry(entry);

        // Infer type if still unknown
        if (entry.assetType == AssetFormat::AssetType::Unknown)
            entry.assetType = InferAssetType(entry.path);

        // Update source mapping
        if (!entry.sourcePath.empty())
        {
            size_t sourceHash = util::GenHash(entry.sourcePath);
            sourceToFileEntries.emplace(sourceHash, &entry);
        }
    }
}

void ResourceFilepaths::UpdateEntryMetadata(const std::string& filepath, AssetFormat::AssetType type,
                                            const std::string& sourcePath, uint64_t sourceTs,
                                            uint64_t compiledTs, uint32_t version)
{
    size_t filepathHash = util::GenHash(filepath);
    auto it = fileEntries.find(filepathHash);
    if (it == fileEntries.end())
        return;

    FileEntry& entry = it->second;

    // Clear old source mapping
    if (!entry.sourcePath.empty() && entry.sourcePath != sourcePath)
    {
        size_t oldSourceHash = util::GenHash(entry.sourcePath);
        auto range = sourceToFileEntries.equal_range(oldSourceHash);
        for (auto sit = range.first; sit != range.second; )
        {
            if (sit->second == &entry)
                sit = sourceToFileEntries.erase(sit);
            else
                ++sit;
        }
    }

    // Update fields
    entry.assetType = type;
    entry.sourcePath = sourcePath;
    entry.sourceTimestamp = sourceTs;
    entry.compiledTimestamp = compiledTs;
    entry.formatVersion = version;

    // Add new source mapping
    if (!sourcePath.empty())
    {
        size_t sourceHash = util::GenHash(sourcePath);
        sourceToFileEntries.emplace(sourceHash, &entry);
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

AssetFormat::AssetType ResourceFilepaths::InferAssetType(const std::string& filepath)
{
    std::string ext = VFS::GetExtension(filepath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Map compiled extensions to asset types
    if (ext == ".mesh") return AssetFormat::AssetType::Mesh;
    if (ext == ".anim") return AssetFormat::AssetType::Animation;
    if (ext == ".material") return AssetFormat::AssetType::Material;
    if (ext == ".ktx" || ext == ".ktx2" || ext == ".dds") return AssetFormat::AssetType::Texture;
    if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") return AssetFormat::AssetType::Audio;

    // Also check source extensions
    return AssetFormat::getAssetTypeFromExtension(ext);
}
