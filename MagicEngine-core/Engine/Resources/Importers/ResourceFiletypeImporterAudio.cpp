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

bool ResourceFiletypeImporterAudio::Import(const std::filesystem::path& assetRelativeFilepath)
{
	// Load the file into FMOD
	auto sound{ ST<AudioManager>::Get()->CreateSound(GetExeRelativeFilepath(assetRelativeFilepath).string()) };
	if (!sound)
		return false;

	// Create fileentry
	const auto* fileentry{ GenerateFileEntryForResources<ResourceAudio>(assetRelativeFilepath, 1) };

	// Set the resource to the FMOD sound
	size_t hash{ fileentry->associatedResources[0].hashes[0] };
	auto* resource{ ST<MagicResourceManager>::Get()->INTERNAL_GetAudio().INTERNAL_GetResource(hash, true) };
	resource->sound = sound;
	// Note: Currently no metadata is set for sounds. Perhaps we can read a file associated with the audio here to load its metadata, similar to unity's metadata file method.

	return true;
}
