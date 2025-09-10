/******************************************************************************/
/*!
\file   AudioManager.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Section B
\date   10/09/2025

\author Kuan Yew Chong (100%)
\par    email: yewchong.k\@digipen.edu
\par    DigiPen login: yewchong.k

\brief
	The audio manager is a persistent singleton that manages the FMOD system and studio system. 
	It does not directly handle the channel nor hold any audio playing state, leaving it to the components. 
	Components request audio playback as long as they have a valid channel pointer and a name of the audio asset.

	However, it does control channels via groups, allowing for group mixing and volume control.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

enum class AudioType : char { BGM, 
							  SFX, 
							  END };

// Audio metadata
struct AudioData
{
	std::string name = "";
	float defaultVol = 1.f;
	bool clip_is_3d = false;
	AudioType type = AudioType::END; // Undetermined
};

// Representation of a loaded sound asset - holds both the handle to fmod sound and the metadata
struct AudioAsset
{
	AudioAsset(){}; // I would never allow this but this is due to default constructor being required in the current resource manager implementation
	AudioAsset(FMOD::Sound* s, const std::string& n, float vol = 1.f, bool is3D = false, AudioType t = AudioType::END) : sound(s)
	{
		// Initializes the inner audio data
		data.name = n;
		data.defaultVol = vol;
		data.clip_is_3d = is3D;
		data.type = t;
	}

	AudioData data;
	FMOD::Sound* sound = nullptr;
};

class AudioManager
{
public:
	// Allow ST to access private members.
	friend class ST<AudioManager>;
	AudioManager();
	~AudioManager(); // Public destructor - frees fmod
	void Initialise();
	void Update(); // auto calls system->update() internally, no need for additional calls

	// File management calls
	void CreateSound(const std::string& name);
	void FreeSound(FMOD::Sound* sound);

	// Default FMOD System call
	void PlaySound(FMOD::Channel*& channel, const std::string& name, bool loop, AudioType category = AudioType::END); // Universal bypass call to FMOD
	void StopSound(FMOD::Channel*& channel);
	bool IsPlaying(FMOD::Channel*& channel);
	void PlaySound3D(FMOD::Channel*& channel, const std::string& name, bool loop, Vec3 position, AudioType category = AudioType::END);

	// FMOD Studio Call
	// void CreateEvent(const std::string& name); // tbc: fmod studio
	// void PlaySoundAt(); // tbc: spatial audio impl

	// Master controls
	void UpdateListener(const FMOD_VECTOR& pos, const FMOD_VECTOR& vel);
	void StopAllSounds();
	void SetBaseVolume(AudioType type, float vol);

	const std::vector<std::string>& GetSoundNames() const;

private:
	FMOD_RESULT result; // For error checking - bound to the class and used by FMOD_ASSERT macro
	FMOD::System* system;
	FMOD::Studio::System* fmod_studio;

	FMOD::ChannelGroup* masterChannelGroup = nullptr;
	FMOD::ChannelGroup* channelGroups[+AudioType::END] = { nullptr }; // BGM, SFX
	std::vector<std::string> soundNames;

	const int MAX_CHANNELS = 512;
};