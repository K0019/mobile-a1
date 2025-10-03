#include "ResourceFiletypeImporterAudio.h"
#include "ResourceTypesAudio.h"
#include "AudioManager.h"

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
	auto* resource{ ST<ResourceManager>::Get()->INTERNAL_GetAudio().INTERNAL_GetResource(hash, true) };
	resource->sound = sound;
	// Note: Currently no metadata is set for sounds. Perhaps we can read a file associated with the audio here to load its metadata, similar to unity's metadata file method.

	return true;
}
