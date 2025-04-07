/******************************************************************************/
/*!
\file   AudioManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (90%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Kendrick Sim Hean Guan (10%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  AudioManager is a singleton class initialised when it is first accessed and dies at 
  the end of the program. It can be used to play sounds, modify pitch and modify
  volume.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

/*****************************************************************//*!
\class AudioManager
\brief
	The primary class for audio management.
*//******************************************************************/
class AudioManager
{
public:
	// Allow ST to access private members.
	friend class ST<AudioManager>;

	/*****************************************************************//*!
	\brief
		Destructor for AudioManager. Releases the soundgroup and closes
		the FMOD System.
	*//******************************************************************/
	~AudioManager(); // Public destructor

	/*****************************************************************//*!
	\brief
		Manual initialisation of the instance is required because base
		volume needs to be set by GameSettings::Load() before the first
		sound plays.
	*//******************************************************************/
	void Initialise();

	/*****************************************************************//*!
	\brief
		Creates a new channel group.
	\param name
		Name identifier for this channel group.
	*//******************************************************************/
	FMOD::ChannelGroup* CreateChannelGroup(std::string const& name);

	/*****************************************************************//*!
	\brief
		Universal StartSound function that will decipher what type of sound
		is being passed in as parameter. Avoid using this if performance is
		a concern.
	\param name
		Name of the sound, excluding extensions and trailing numbers.
	\param loop
		Whether to loop this sound.
	\param position
		World position. If provided, spatial audio will be applied.
	\param volume
		Volume. 0.0f is muted, 1.0f is full.
	*//******************************************************************/
	void StartSound(std::string const& name, bool loop = false, std::optional<Vector2> const& position = std::nullopt, float volume = 1.0f);

	/*****************************************************************//*!
	\brief
		Starts playing a single sound.
	\param name
		Name of the sound, excluding extensions and trailing numbers.
	\param loop
		Whether to loop this sound.
	\param position
		World position. If provided, spatial audio will be applied.
	\param volume
		Volume. 0.0f is muted, 1.0f is full.
	*//******************************************************************/
	void StartSingleSound(std::string const& name, bool loop = false, std::optional<Vector2> const& position = std::nullopt, float volume = 1.0f);

	/*****************************************************************//*!
	\brief
		Starts playing a random sound from a sound group.
	\param baseName
		Name of the sound group, excluding extensions and trailing numbers.
	\param loop
		Whether to loop this sound.
	\param position
		World position. If provided, spatial audio will be applied.
	\param volume
		Volume. 0.0f is muted, 1.0f is full.
	*//******************************************************************/
	void StartGroupedSound(std::string const& baseName, bool loop = false, std::optional<Vector2> const& position = std::nullopt, float volume = 1.0f);

	/*****************************************************************//*!
	\brief
		Starts playing a specific sound from a sound group.
		Primarily for usage with Asset Browser.
	\param name
		Name of the sound, excluding extensions and trailing numbers.
	\param loop
		Whether to loop this sound.
	\param position
		World position. If provided, spatial audio will be applied.
	\param volume
		Volume. 0.0f is muted, 1.0f is full. 
	*//******************************************************************/
	void StartSpecificGroupedSound(std::string const& name, bool loop = false, std::optional<Vector2> const& position = std::nullopt, float volume = 1.0f);

	/*****************************************************************//*!
	\brief
		Sets the channel group of a certain sound.
	\param soundName
		Name of the sound, excluding extensions and trailing numbers.
	\param group
		Name of the channel group.
		"SFX" is available by default.
	*//******************************************************************/
	void SetChannelGroup(std::string const& soundName, std::string const& group);

	/*****************************************************************//*!
	\brief
		Creates or updates an existing sound.
	\param filename
		Name of the sound file.
	\param isGrouped
		Whether the sound is grouped or not.
	*//******************************************************************/
	void CreateUpdateSound(std::string const& filename, bool isGrouped = false);

	/*****************************************************************//*!
	\brief
		Tells you if a filename is a single sound or grouped sound.
	\param filename
		Name of the sound file.
	\return
		True if the sound is grouped, false otherwise.
	*//******************************************************************/
	bool CheckIsGrouped(std::string const& filename);

	/*****************************************************************//*!
	\brief
		Deletes a sound from the AudioManager and ResourceManager.
		Also calls DeleteSoundFromDirectory.
	\param filename
		Name of the sound file.
	\param isGrouped
		Whether the sound is grouped or not.
	*//******************************************************************/
	void DeleteSound(std::string const& filename, bool isGrouped = false);

	/*****************************************************************//*!
	\brief
		Stops a sound that is currently playing.
	\param name
		Name of the sound you want to stop.
	*//******************************************************************/
	void StopSound(std::string const& name);

	/*****************************************************************//*!
	\brief
		Stops all currently playing sounds.
	*//******************************************************************/
	void StopAllSounds();

	/*****************************************************************//*!
	\brief
		Pauses all currently playing sounds.
	*//******************************************************************/
	void PauseAllSounds();

	/*****************************************************************//*!
	\brief
		Resumes all currently paused sounds.
	*//******************************************************************/
	void ResumeAllSounds();

	/*****************************************************************//*!
	\brief
		Sets the volume of a currently playing sound.
	\param name
		Name of the sound you want to modify.
	\param volume
		Value of volume to set.
	*//******************************************************************/
	void SetSoundVolume(std::string const& name, float volume);

	/*****************************************************************//*!
	\brief
		Gets the volume of a particular sound group.
	\param group
		Name of the sound group.
	\return
		float value representing volume.
	*//******************************************************************/
	float GetGroupVolume(std::string const& group = std::string{ defaultGroup });

	/*****************************************************************//*!
	\brief
		Gets the pitch of a particular sound group.
	\param group
		Name of the sound group.
	\return
		float value representing pitch.
	*//******************************************************************/
	float GetGroupPitch(std::string const& group = std::string{ defaultGroup });

	/*****************************************************************//*!
	\brief
		Sets the volume of a soundgroup.
	\param volume
		float value representing new volume.
	\param group
		Name of the sound group.
	*//******************************************************************/
	void SetGroupVolume(float volume, std::string const& group = std::string{ defaultGroup });

	/*****************************************************************//*!
	\brief
		Sets the pitch of a soundgroup.
	\param pitch
		float value representing new pitch.
	\param group
		Name of the sound group.
	*//******************************************************************/
	void SetGroupPitch(float pitch, std::string const& group = std::string{ defaultGroup });

	/*****************************************************************//*!
	\brief
		Uses FMOD fade point to interpolate the volume of chosen
		channel group.
	\param targetVolume
		float value representing target volume.
	\param duration
		How long the interpolation should take.
	\pram group
		Name of the channel group.
	*//******************************************************************/
	void InterpolateGroupVolume(float targetVolume, float duration, std::string const& group = std::string{ defaultGroup });

	/*****************************************************************//*!
	\brief
		Prints the number of sound sounds that are currently playing
		sound. May update to use in-editor console log in the future.
	*//******************************************************************/
	void DebugPrint();

	/*****************************************************************//*!
	\brief
		Getter for singleSoundFolder.
	\return
		string.
	*//******************************************************************/
	const std::string& SingleSoundFolder();

	/*****************************************************************//*!
	\brief
		Getter for groupedSoundFolder.
	\return
		string.
	*//******************************************************************/
	const std::string& GroupedSoundFolder();

	/*****************************************************************//*!
	\brief
		Returns a sorted list of names of single sounds that are currently
		loaded.
	\return
		Set of strings
	*//******************************************************************/
	std::set<std::string> GetSingleSoundNames();

	/*****************************************************************//*!
	\brief
		Returns a sorted list of names of grouped sounds that are currently
		loaded.
	\return
		Map of Set of strings. Each key in the map represents the base name
		of each sound group.
	*//******************************************************************/
	std::map<std::string, std::set<std::string>> GetGroupedSoundNames();
	
	/*****************************************************************//*!
	\brief
		Checks FMOD Channels to see if a certain sound is playing.
		Primarily for usage with Asset Browser.
	\param name
		Name of the sound to check.
	\return
		True if an instance of the sound is currently playing.
		False otherwise.
	*//******************************************************************/
	bool IsSoundPlaying(std::string const& name);

	/*****************************************************************//*!
	\brief
		Releases channel groups and clears channelGroups.
	*//******************************************************************/
	void ReleaseChannelGroups();

	/*****************************************************************//*!
	\brief
		Set whether the AudioManager should be able to start new audio.
	\param isListening
		Value to set.
	*//******************************************************************/
	void SetIsListening(bool isListening);

	/*****************************************************************//*!
	\brief
		Set the attributes of the audio listener.
	\param position
		Value to set.
	\param velocity
		Value to set.
	*//******************************************************************/
	void UpdateListenerAttributes(Vector2 const& velocity);

	/*****************************************************************//*!
	\brief
		Updates the FMOD::System spatial audio properties.
	\param minDistance
		Distance at which sound is heard at full volume.
	\param maxDistance
		Distance at which sound fades completely.
	\param dopplerScale
		Controls the intensity of pitch shifting for moving sound sources.
		Currently, sounds have no relative velocity so doppler scale does nothing.
	\param distanceFactor
		Scales how much distance affects a sound's volume.
		A lower value will result in a less steep sound gradient.
	\param rolloffScale
		Determines how quickly sounds will fade as the listener moves away from the sound source.
	*//******************************************************************/
	void UpdateSpatialProperties(float minDistance, float maxDistance, float dopplerScale, float distanceFactor, float rolloffScale);

	/*****************************************************************//*!
	\brief
		Updates the FMOD system.
	*//******************************************************************/
	void UpdateSystem();

	/*****************************************************************//*!
	\brief
		Sets the BASE volume of a soundgroup.
	\param volume
		float value representing new volume.
	\param channelGroupName
		Name of the sound group.
	*//******************************************************************/
	void SetBaseVolume(std::string channelGroupName, float volume);

	/*****************************************************************//*!
	\brief
		Returns the BASE volume of a soundgroup.
	\param channelGroupName
		Name of the sound group.
	\return
		float value representing volume.
	*//******************************************************************/
	float GetBaseVolume(std::string channelGroupName);

	std::unordered_map<std::string, std::vector<FMOD::Channel*>> const& GetChannels();

private:

	FMOD_RESULT result;
	FMOD::System* system;
	std::unordered_map<std::string, FMOD::ChannelGroup*> channelGroups;
	std::unordered_map<std::string, FMOD::ChannelGroup*> baseChannelGroups; // A duplicate of channelGroups that contains the base channel groups (nested)
	std::unordered_map<std::string, std::vector<FMOD::Channel*>> channels;
	std::unordered_map<std::string, std::pair<FMOD::Sound*, FMOD::ChannelGroup*>> singleSounds;
	std::unordered_map<std::string, std::pair<std::unordered_map<std::string, FMOD::Sound*>, FMOD::ChannelGroup*>> groupedSounds;

	std::set<std::string> singleSoundNames;
	std::map<std::string, std::set<std::string>> groupedSoundNames;

	static constexpr int MAX_CHANNELS						{ 4095 };
	static constexpr std::string_view defaultGroup			{ "SFX" };
	bool listening;

	Vector2 listenerPosition;


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

	/*****************************************************************//*!
	\brief
		Internal play sound helper function
	\param sound
		FMOD::Sound* containing sound data
	\param channelGroup
		FMOD::ChannelGroup* to play the sound from
	\param loop
		Whether to loop this sound.
	\param position
		World position. If provided, spatial audio will be applied.
	\param volume
		Volume. 0.0f is muted, 1.0f is full.
	\return
		Returns the FMOD::Channel* used to play the sound.
	*//******************************************************************/
	FMOD::Channel* StartSound(FMOD::Sound* sound, FMOD::ChannelGroup* channelGroup, bool loop, std::optional<Vector2> const& position, float volume = 1.0f);

	/*****************************************************************//*!
	\brief
		Initialises FMOD systems. An FMOD sound object is created for each
		sound file found and then passed to ResourceManager for storage.
	*//******************************************************************/
	AudioManager(); // Private constructor

	/*****************************************************************//*!
	\brief
		Apparently, FMOD is supposed to handle channel memory
		automatically. However I have found this to be inadequate and
		hence the manual cleaning of channels here.
	*//******************************************************************/
	void CleanChannels();

	/*****************************************************************//*!
	\brief
		Just a helper function to check if anything went wrong while
		initialising, playing sound and closing the FMOD systems.
	\param inResult
		FMOD_RESULT container.
	*//******************************************************************/
	void ErrorCheck(FMOD_RESULT inResult);

	/*****************************************************************//*!
	\brief
		Reads a directory and returns a vector of all filenames found
		within directory.
	\param folderName
		The path of the folder to look within.
	\return
		Vector of strings.
	*//******************************************************************/
	std::vector<std::string> GetFilenames(const std::string& folderName);

	/*****************************************************************//*!
	\brief
		Uses std::filesystem to delete a sound file from the directory.
	\param filename
		Name of the sound file.
	\param isGrouped
		Whether the sound is grouped or not.
	*//******************************************************************/
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
