#pragma once

enum class AudioType : char { BGM, 
							  SFX, 
							  END };

// The file reprensetation of audio, in other words, this is metadata. We need to use this to replace the AssetManager's sound struct in the future
struct Audio
{
	Audio(std::string n) : name(n) {}
	Audio(std::string n, FMOD::Sound* s) : name(n), sound(s) {}

	std::string name;
	FMOD::Sound* sound = nullptr;
	AudioType type = AudioType::END; // Undetermined
	float defaultVol = 1.f;
};

class AudioManager
{
public:
	// Allow ST to access private members.
	friend class ST<AudioManager>;

	AudioManager();
	~AudioManager(); // Public destructor - frees fmod
	void Initialise();
	inline void Update() { fmod_studio->update(); } // auto calls system->update() internally, no need for additional calls

	void CreateSound(const std::string& name);
	void DeleteSound(const std::string& name) {};//ResourceManager::DeleteSound(name); } //temp

	void PlaySound(const std::string& name, bool loop, AudioType category = AudioType::END); // Universal bypass call to FMOD
	//void PlaySoundAt(); // tbc: spatial audio impl

	std::vector<std::string> GetSoundNames() const { return soundNames; }

	void StopAllSounds();
	void SetBaseVolume(AudioType type, float vol);

private:

	FMOD_RESULT result;
	FMOD::System* system;
	FMOD::Studio::System* fmod_studio;

	FMOD::ChannelGroup* channelGroups[+AudioType::END] = { nullptr }; // BGM, SFX
	std::vector<std::string> soundNames;

	const int MAX_CHANNELS = 512;
};

// Written by Kendrick Sim
/*****************************************************************//*!
\class AudioReference
\brief
	References an audio asset, that can be passed to AudioManager for playing.
*//******************************************************************/
//class AudioReference : public ISerializeable
//{
//public:
//	/*****************************************************************//*!
//	\brief
//		Constructor.
//	\param defaultName
//		The name of the audio file.
//	*//******************************************************************/
//	AudioReference(const char* defaultName);
//
//	/*****************************************************************//*!
//	\brief
//		Cast operator to string. Used to decay this reference into std::string
//		to be compatible with AudioManager.
//		Note: In the future, this should probably not exist and the interface to
//		AudioManager should directly interface with AudioReference.
//	\return
//		The audio file name.
//	*//******************************************************************/
//	operator const std::string&() const;
//
//public:
//	/*****************************************************************//*!
//	\brief
//		Draws this reference to the inspector, enabling overriding of the audio file.
//	\param label
//		Label of this audio reference.
//	*//******************************************************************/
//	void EditorDraw(const char* label);
//
//private:
//	//! The name of the audio file.
//	std::string name;
//
//public:
//	property_vtable()
//};
//property_begin(AudioReference)
//{
//	property_var(name)
//}
//property_vend_h(AudioReference)
