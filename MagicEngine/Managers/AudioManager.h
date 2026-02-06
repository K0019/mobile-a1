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

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#ifdef __ANDROID__
#include <FMOD/fmod_android.h>
#include <FMOD/fmod_errors.h>
#include <FMOD/fmod.hpp>
#include <FMOD/fmod_studio.hpp>
#else
#include <FMOD/fmod.hpp>
#include <FMOD/fmod_studio.hpp>
#endif
#include "Assets/Types/AssetTypesAudio.h"

class AudioManager
{
public:
	class ChannelManager
	{
	private:
	public:
		static constexpr uint32_t INVALID_HANDLE = 0;
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
	FMOD::Sound* CreateSound(const std::string& filepath);
	FMOD::Sound* CreateSoundFromData(const char* data, size_t length);
	void FreeSound(FMOD::Sound* sound);

	// Default FMOD System call
	// 2D call to FMOD
	uint32_t PlaySound(const std::string& filename, bool loop, AudioType category = AudioType::END, float volumeModifier = 1.0f);
	uint32_t PlaySound(size_t audioResourceHash, bool loop, AudioType category = AudioType::END, float volumeModifier = 1.0f);
	void StopSound(uint32_t handle);
	bool IsPlaying(uint32_t handle);
	// This refers to the current playback position in milliseconds, not the 3D position
	const unsigned int GetChannelPosition(uint32_t handle) const;
	FMOD::Sound* GetSound(uint32_t handle) const;
	void SetChannelPosition(uint32_t handle, const Vec3& pos, const Vec3& vel = Vec3());
	void SetVolume(uint32_t handle, float vol);

	// Default 3D Audio calls
	uint32_t PlaySound3D(const std::string& filename, bool loop, Vec3 position, AudioType category = AudioType::END, std::pair<float, float> rolloff_minmax = { 2.f, 50.f }, float volumeModifier = 1.0f);
	uint32_t PlaySound3D(size_t audioResourceHash, bool loop, Vec3 position, AudioType category = AudioType::END, std::pair<float, float> rolloff_minmax = { 2.f, 50.f }, float volumeModifier = 1.0f);
	void SetChannel3D(uint32_t handle, bool is3D);
	bool IsChannel3D(uint32_t handle) const;
	void SetChannel3DAttributes(uint32_t handle, const Vec3& pos, const Vec3& vel = Vec3());
	void FadeoutAudio(uint32_t handle, float seconds);

	// FMOD Studio Call
	// void CreateEvent(const std::string& name); // tbc: fmod studio
	// void PlaySoundAt(); // tbc: spatial audio impl

	// Master controls
	void UpdateListener(const Vec3& pos, const Vec3& vel = Vec3{});
	void ConfigureListener(float dopplerScale, float distanceFactor, float rolloffScale);
	void StopAllSounds();
	void OnAppPause();   // Suspend FMOD mixer (Android lifecycle)
	void OnAppResume();  // Resume FMOD mixer (Android lifecycle)
	void SetGroupVolume(AudioType type, float vol);

	static FMOD_RESULT F_CALLBACK ChannelCallback(FMOD_CHANNELCONTROL* channelControl, FMOD_CHANNELCONTROL_TYPE controlType,
		FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType, void* commandData1, void* commandData2);

#ifdef __ANDROID__
    static FMOD_RESULT F_CALLBACK FileOpenCallback(const char* name, unsigned int* filesize, void** handle, void* userdata);
	static FMOD_RESULT F_CALLBACK FileCloseCallback(void* handle, void* userdata);
	static FMOD_RESULT F_CALLBACK FileReadCallback(void* handle, void* buffer, unsigned int sizebytes, unsigned int* bytesread, void* userdata);
	static FMOD_RESULT F_CALLBACK FileSeekCallback(void* handle, unsigned int pos, void* userdata);
#endif

	// Internal access for video audio playback
	FMOD::System* INTERNAL_GetSystem() { return system; }

private:
	FMOD::System* system;
	FMOD::Studio::System* fmod_studio;

	FMOD::ChannelGroup* masterChannelGroup;
	FMOD::ChannelGroup* channelGroups[+AudioType::END]; // BGM, SFX

	static constexpr int MAX_CHANNELS = 512;
	ChannelManager channelManager;
};