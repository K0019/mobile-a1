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
	void Update(); // auto calls system->update() internally, no need for additional calls

	void CreateSound(const std::string& name);
	void FreeSound(FMOD::Sound* sound);

	// FMOD System call
	FMOD::Channel* PlaySound(const std::string& name, bool loop, AudioType category = AudioType::END); // Universal bypass call to FMOD
	FMOD::Channel* PlaySound3D(const std::string& name, bool loop, Vec3 position, AudioType category = AudioType::END);
	void UpdateListener(const FMOD_VECTOR& pos, const FMOD_VECTOR& vel);

	// FMOD Studio Call
	// void CreateEvent(const std::string& name); // tbc: fmod studio
	// void PlaySoundAt(); // tbc: spatial audio impl
	void StopAllSounds();
	void SetBaseVolume(AudioType type, float vol);

	const std::vector<std::string>& GetSoundNames() const;

private:
	FMOD_RESULT result;
	FMOD::System* system;
	FMOD::Studio::System* fmod_studio;

	FMOD::ChannelGroup* masterChannelGroup = nullptr;
	FMOD::ChannelGroup* channelGroups[+AudioType::END] = { nullptr }; // BGM, SFX
	std::vector<std::string> soundNames;

	const int MAX_CHANNELS = 512;
};