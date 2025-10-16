/******************************************************************************/
/*!
\file   ResourceFilepaths.h
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
    const FileEntry* GetFileEntry(size_t resourceHash);
    const FileEntry* GetFileEntry(const std::filesystem::path& filepath) const;
    std::vector<const FileEntry*> GetFileEntries() const;

    // Removes a resource hash from a file entry. Deletes the file entry if there are no remaining associated resources.
    void DisassociateResourceHash(size_t resourceHash, size_t resourceType);

private:

    //! Stores the filepath and its associated resource hashes
    std::unordered_map<size_t, FileEntry> fileEntries;
    //! Maps resource hashes to their FileEntry
    std::unordered_map<size_t, FileEntry*> hashToFileEntry;

};

