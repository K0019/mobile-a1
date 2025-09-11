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
	bool clip_is_3d = false;
	AudioType type = AudioType::END; // Undetermined is not an error state, just means the audio is not categorized
};

// Representation of a loaded sound asset - holds both the handle to fmod sound and the metadata
struct AudioAsset
{
	AudioAsset(){}; // suspicious, but has to be like this due to default constructor being required in the current resource manager implementation
	AudioAsset(FMOD::Sound* s, const std::string& n, float vol = 1.f, bool is3D = false, AudioType t = AudioType::END) : sound(s)
	{
		// Initializes the inner audio data
		data.name = n;
		data.clip_is_3d = is3D;
		data.type = t;
	}

	AudioData data;
	FMOD::Sound* sound = nullptr;
};

class AudioManager
{
public:
	class ChannelManager
	{
		struct ChannelInstance
		{
			FMOD::Channel* channel = nullptr;
			float minDistance;
			float maxDistance;
			float dopperScale;
			float distanceFactor;
			float rolloffScale;
		};

		static constexpr uint32_t INVALID_HANDLE = 0;
		uint32_t GetNextHandle();

	public:
		uint32_t RegisterChannel(FMOD::Channel* channel, float minDist = 20.f, float maxDist = 50.f, float doppler = 1.f, float distFactor = 0.01f, float rolloff = 0.1f);

		uint32_t currentChannelIndex = 1;
		std::unordered_map<uint32_t, ChannelInstance> channelMap;
	};

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
	uint32_t PlaySound(const std::string& name, bool loop, AudioType category = AudioType::END); // Universal bypass call to FMOD
	void StopSound(uint32_t channel);
	bool IsPlaying(uint32_t channel);
	const unsigned int GetChannelPosition(uint32_t channel);
	FMOD::Sound* GetSound(uint32_t channel) const;
	void SetChannelPosition(uint32_t channel, const Vec3& pos, const Vec3& vel = Vec3());
	void SetVolume(uint32_t channel, float vol);

	uint32_t PlaySound3D(const std::string& name, bool loop, Vec3 position, AudioType category = AudioType::END, std::pair<float, float> rolloff_minmax = { 2.f, 50.f });
	void SetChannel3D(uint32_t channel, bool is3D);
	bool IsChannel3D(uint32_t channel);
	void SetChannel3DAttributes(uint32_t channel, const Vec3& pos, const Vec3& vel = Vec3());

	// FMOD Studio Call
	// void CreateEvent(const std::string& name); // tbc: fmod studio
	// void PlaySoundAt(); // tbc: spatial audio impl

	// Master controls
	void UpdateListener(const Vec3& pos, const Vec3& vel = Vec3());
	void ConfigureListener(float dopplerScale, float distanceFactor, float rolloffScale);
	void StopAllSounds();
	void SetGroupVolume(AudioType type, float vol);

	const std::vector<std::string>& GetSoundNames() const;
	FMOD::Channel* CreateChannel(FMOD::Sound* sound, AudioType type = AudioType::END);

private:
	FMOD_RESULT result; // For error checking - bound to the class and used by FMOD_ASSERT macro
	FMOD::System* system;
	FMOD::Studio::System* fmod_studio;

	FMOD::ChannelGroup* masterChannelGroup = nullptr;
	FMOD::ChannelGroup* channelGroups[+AudioType::END] = { nullptr }; // BGM, SFX
	std::vector<std::string> soundNames;

	ChannelManager channelManager;

	const int MAX_CHANNELS = 512;
};