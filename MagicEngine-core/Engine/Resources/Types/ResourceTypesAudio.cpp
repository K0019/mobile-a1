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
