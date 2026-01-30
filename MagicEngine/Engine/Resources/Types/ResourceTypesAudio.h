/******************************************************************************/
/*!
\file   ResourceTypesAudio.h
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

#pragma once
#include "Engine/Resources/Types/ResourceTypes.h"

// Forward declaration
namespace FMOD {
	class Sound;
}

enum class AudioType : char
{
	BGM,
	SFX,
	END
};

// Audio metadata
struct AudioData
{
	std::string name = "";
	bool clip_is_3d = false;
	AudioType type = AudioType::END; // Undetermined is not an error state, just means the audio is not categorized
};

struct ResourceAudio : public ResourceBase
{
	AudioData data;
	FMOD::Sound* sound = nullptr;

	virtual bool IsLoaded() final;
	~ResourceAudio();
};

struct ResourceAudioGroup : public ResourceBase
{
	std::vector<size_t> audio;

	virtual bool IsLoaded() final;

	const ResourceAudio* PickRandomAudio() const;
};
