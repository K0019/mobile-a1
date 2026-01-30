/******************************************************************************/
/*!
\file   ResourceTypesAudio.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Defines the audio resource.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Types/ResourceTypesAudio.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Managers/AudioManager.h"

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

bool ResourceAudioGroup::IsLoaded()
{
	return !audio.empty();
}

const ResourceAudio* ResourceAudioGroup::PickRandomAudio() const
{
	if (audio.empty())
		return nullptr;
	return UserResourceHandle<ResourceAudio>{ audio[util::RandomRange(0, audio.size() - 1)] }.GetResource();
}
