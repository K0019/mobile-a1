#pragma once
// The file reprensetation of audio, in other words, this is metadata
enum AudioType { SFX, BGM, END };
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

	// FMOD core control systems
	~AudioManager(); // Public destructor - frees fmod
	void Initialise();
	void UpdateSystem();

	FMOD::ChannelGroup* CreateChannelGroup(std::string const& name);
	void StartSingleSound(std::string const& name, bool loop = false, std::optional<Vec3> const& position = std::nullopt, float volume = 1.0f);
	void SetChannelGroup(std::string const& soundName, std::string const& group);
	void CreateUpdateSound(std::string const& filename, bool isGrouped = false);
	void DeleteSound(std::string const& filename, bool isGrouped = false);
	void StopSound(std::string const& name);
	void StopAllSounds();
	void PauseAllSounds();
	void ResumeAllSounds();
	void SetSoundVolume(std::string const& name, float volume);
	float GetGroupVolume(std::string const& group = std::string{ defaultGroup });
	float GetGroupPitch(std::string const& group = std::string{ defaultGroup });
	void SetGroupVolume(float volume, std::string const& group = std::string{ defaultGroup });
	void SetGroupPitch(float pitch, std::string const& group = std::string{ defaultGroup });
	void InterpolateGroupVolume(float targetVolume, float duration, std::string const& group = std::string{ defaultGroup });
	bool IsSoundPlaying(std::string const& name);
	void ReleaseChannelGroups();
	void SetIsListening(bool isListening);
	void UpdateListenerAttributes(Vec3 const& position);
	void UpdateSpatialProperties(float minDistance, float maxDistance, float dopplerScale, float distanceFactor, float rolloffScale);
	void SetBaseVolume(std::string channelGroupName, float volume);
	float GetBaseVolume(std::string channelGroupName);

	/*****************************************************************//*!
	\brief
		Prints the number of sound sounds that are currently playing
		sound. May update to use in-editor console log in the future.
	*//******************************************************************/
	void DebugPrint();

	const std::string& SingleSoundFolder();
	const std::string& GroupedSoundFolder();

	// For asset browser drawing
	std::set<std::string> GetSingleSoundNames() { return singleSoundNames;}
	std::map<std::string, std::set<std::string>> GetGroupedSoundNames(){ return groupedSoundNames; }
	


	std::unordered_map<std::string, std::vector<FMOD::Channel*>> const& GetChannels() { return channels; }


private:

	FMOD_RESULT result;
	FMOD::System* system;

	std::unordered_map<std::string, Audio> sounds;

	std::unordered_map<std::string, FMOD::ChannelGroup*> channelGroups;
	std::unordered_map<std::string, FMOD::ChannelGroup*> baseChannelGroups; // A duplicate of channelGroups that contains the base channel groups (nested)
	std::unordered_map<std::string, std::vector<FMOD::Channel*>> channels;

	std::unordered_map<std::string, std::pair<FMOD::Sound*, FMOD::ChannelGroup*>> singleSounds;
	std::unordered_map<std::string, std::pair<std::unordered_map<std::string, FMOD::Sound*>, FMOD::ChannelGroup*>> groupedSounds;

	// For imgui rendering
	std::set<std::string> singleSoundNames;
	std::map<std::string, std::set<std::string>> groupedSoundNames;

	static constexpr int MAX_CHANNELS						{ 4095 };
	static constexpr std::string_view defaultGroup			{ "SFX" };
	bool listening;

	Vec3 listenerPosition;


	// INTERNAL HELPER FUNCTIONS - DO NOT ACCESS
	/*****************************************************************//*!
	\brief
		Callback function for message "OnWindowFocus" which will pause all
		sounds and resume them depending on whether the window was focused.
	\param isFocused
		Whether the window is being focused
	*//******************************************************************/
	static void WindowFocusPauseResume(bool isFocused);

	/*****************************************************************//*!
	\brief
		Removes trailing numbers from a string
	\param name
		Input string
	\return
		New string without the trailing numbers
	*//******************************************************************/
	std::string RemoveTrailingNumbers(std::string const& name);

	/*****************************************************************//*!
	\brief
		Removes file extensions from a string
	\param filename
		Input string
	\return
		New string without the extension
	*//******************************************************************/
	std::string RemoveExtension(std::string const& filename);
	FMOD::Channel* StartSound(FMOD::Sound* sound, FMOD::ChannelGroup* channelGroup, bool loop, std::optional<Vec3> const& position, float volume = 1.0f);

	AudioManager(); // Private constructor

	/*****************************************************************//*!
	\brief
		Apparently, FMOD is supposed to handle channel memory
		automatically. However I have found this to be inadequate and
		hence the manual cleaning of channels here.
	*//******************************************************************/
	void CleanChannels();
	void DeleteSoundFromDirectory(std::string const& filename, bool isGrouped = false);
};

// Written by Kendrick Sim
/*****************************************************************//*!
\class AudioReference
\brief
	References an audio asset, that can be passed to AudioManager for playing.
*//******************************************************************/
class AudioReference : public ISerializeable
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param defaultName
		The name of the audio file.
	*//******************************************************************/
	AudioReference(const char* defaultName);

	/*****************************************************************//*!
	\brief
		Cast operator to string. Used to decay this reference into std::string
		to be compatible with AudioManager.
		Note: In the future, this should probably not exist and the interface to
		AudioManager should directly interface with AudioReference.
	\return
		The audio file name.
	*//******************************************************************/
	operator const std::string&() const;

public:
	/*****************************************************************//*!
	\brief
		Draws this reference to the inspector, enabling overriding of the audio file.
	\param label
		Label of this audio reference.
	*//******************************************************************/
	void EditorDraw(const char* label);

private:
	//! The name of the audio file.
	std::string name;

public:
	property_vtable()
};
property_begin(AudioReference)
{
	property_var(name)
}
property_vend_h(AudioReference)
