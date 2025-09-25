#pragma once

struct AssociatedResourceHashes
{
    size_t resourceTypeHash;
    std::vector<size_t> hashes;
};

class ResourceFilepaths
{
public:
    struct FileEntry
    {
        //! The relative filepath from the assets folder to the file
        std::filesystem::path path;
        //! The resources that can be loaded by loading this file
        std::vector<AssociatedResourceHashes> associatedResources;
    };

public:
    const FileEntry* SetFilepath(const std::filesystem::path& filepath, std::vector<AssociatedResourceHashes>&& associatedHashes);

    // Gets the FileEntry associated with a resource hash
    const FileEntry* GetFileEntry(size_t hash);
    const FileEntry* GetFileEntry(const std::filesystem::path& filepath) const;
    std::vector<const FileEntry*> GetFileEntries() const;

private:

    //! Stores the filepath and its associated resource hashes
    std::unordered_map<size_t, FileEntry> fileEntries;
    //! Maps resource hashes to their FileEntry
    std::unordered_map<size_t, FileEntry*> hashToFileEntry;

};

