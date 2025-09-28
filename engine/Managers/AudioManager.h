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
#include <FMOD/fmod.hpp>
#include <FMOD/fmod_studio.hpp>

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

// Representation of a loaded sound asset - holds both the handle to fmod sound and the metadata
struct AudioAsset
{
	AudioAsset() = default;
	AudioAsset(FMOD::Sound* sound, const std::string& name, bool is3D = false, AudioType type = AudioType::END);

	AudioData data;
	FMOD::Sound* sound = nullptr;
};

class AudioManager
{
public:
	class ChannelManager
	{
	private:
		static constexpr uint32_t INVALID_HANDLE = 0;

	public:
		ChannelManager(uint16_t maxChannels);

		uint32_t RegisterChannel(FMOD::Channel* channel);
		FMOD::Channel* GetChannel(uint32_t handle) const;
		void DeleteChannel(uint32_t handle);

		void ClearAllChannels();

	private:
		std::unordered_map<uint32_t, FMOD::Channel*> channelMap;
		uint16_t maxChannels;
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
	// 2D call to FMOD
	uint32_t PlaySound(const std::string& name, bool loop, AudioType category = AudioType::END);  
	void StopSound(uint32_t handle);
	bool IsPlaying(uint32_t handle);
	// This refers to the current playback position in milliseconds, not the 3D position
	const unsigned int GetChannelPosition(uint32_t handle) const;
	FMOD::Sound* GetSound(uint32_t handle) const;
	void SetChannelPosition(uint32_t handle, const Vec3& pos, const Vec3& vel = Vec3());
	void SetVolume(uint32_t handle, float vol);

	// Default 3D Audio calls
	uint32_t PlaySound3D(const std::string& name, bool loop, Vec3 position, AudioType category = AudioType::END, std::pair<float, float> rolloff_minmax = { 2.f, 50.f });
	void SetChannel3D(uint32_t handle, bool is3D);
	bool IsChannel3D(uint32_t handle) const;
	void SetChannel3DAttributes(uint32_t handle, const Vec3& pos, const Vec3& vel = Vec3());

	// FMOD Studio Call
	// void CreateEvent(const std::string& name); // tbc: fmod studio
	// void PlaySoundAt(); // tbc: spatial audio impl

	// Master controls
	void UpdateListener(const Vec3& pos, const Vec3& vel = Vec3{});
	void ConfigureListener(float dopplerScale, float distanceFactor, float rolloffScale);
	void StopAllSounds();
	void SetGroupVolume(AudioType type, float vol);

	const std::vector<std::string>& GetSoundNames() const;

private:
	FMOD::System* system;
	FMOD::Studio::System* fmod_studio;

	FMOD::ChannelGroup* masterChannelGroup;
	FMOD::ChannelGroup* channelGroups[+AudioType::END]; // BGM, SFX
	std::vector<std::string> soundNames;

	static constexpr int MAX_CHANNELS = 512;
	ChannelManager channelManager;
};