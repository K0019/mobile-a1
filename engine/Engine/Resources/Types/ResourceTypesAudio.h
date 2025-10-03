#pragma once
#include "ResourceTypes.h"

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
using ResourceContainerAudio = ResourceContainerBase<ResourceAudio>;
