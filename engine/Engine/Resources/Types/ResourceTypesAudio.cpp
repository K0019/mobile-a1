#include "ResourceTypesAudio.h"
#include "AudioManager.h"

bool ResourceAudio::IsLoaded()
{
	return sound;
}

ResourceAudio::~ResourceAudio()
{
	if (sound)
		// Note: This stops all sounds...
		ST<AudioManager>::Get()->FreeSound(sound);
}
