/******************************************************************************/
/*!
\file   ResourceFiletypeImporterAudio.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/27/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A resource importer for audio files supported by FMOD.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Importers/ResourceFiletypeImporterAudio.h"
#include "Engine/Resources/Types/ResourceTypesAudio.h"
#include "Managers/AudioManager.h"

bool ResourceFiletypeImporterAudio::Import(const std::string& assetRelativeFilepath)
{
	// Load the file into FMOD
	//auto sound{ ST<AudioManager>::Get()->CreateSound(GetExeRelativeFilepath(assetRelativeFilepath).string()) };
	std::vector<uint8_t> fileData;
	if (!VFS::ReadFile(assetRelativeFilepath, fileData))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "VFS failed to read audio file: " << assetRelativeFilepath;
		return false;
	}

	auto sound{ ST<AudioManager>::Get()->CreateSoundFromData(reinterpret_cast<const char*>(fileData.data()), fileData.size()) };
	if (!sound)
		return false;

	// Create fileentry
	if (const ResourceFilepaths::FileEntry* existingFileEntry{ ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(assetRelativeFilepath) })
		return existingFileEntry;

	std::vector<AssociatedResourceHashes> resourceHashes{ 1 };
	resourceHashes[0].resourceTypeHash = util::ConsistentHash<ResourceAudio>();
	resourceHashes[0].hashes.push_back(util::GenHash(VFS::GetStem(VFS::NormalizePath(assetRelativeFilepath))));
	GenerateNamesForResources(resourceHashes, assetRelativeFilepath);
	const auto* fileentry{ ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(assetRelativeFilepath, std::move(resourceHashes)) };

	// Set the resource to the FMOD sound
	size_t hash{ fileentry->associatedResources[0].hashes[0] };
	auto* resource{ ST<MagicResourceManager>::Get()->INTERNAL_GetAudio().INTERNAL_GetResource(hash, true) };
	resource->sound = sound;
	// Note: Currently no metadata is set for sounds. Perhaps we can read a file associated with the audio here to load its metadata, similar to unity's metadata file method.

	return true;
}
