/******************************************************************************/
/*!
\file   AudioManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (99%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Kendrick Sim Hean Guan (1%)
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

#include "AudioManager.h"
#include <FMOD/fmod_errors.h>
#include "ResourceManager.h"

// Macro sprinkler for FMOD error, we don't need a function call stack for this
#define FMOD_ASSERT(FUNCTION) \
				result = FUNCTION;  \
				if (result != FMOD_OK) { \
						CONSOLE_LOG(LEVEL_ERROR) << "FMOD: (" << result << ") " << FMOD_ErrorString(result);}

// Creates and loads sounds on launch
AudioManager::AudioManager()
	: result			{}
	, system			{ nullptr }
	, channels			{}
	, channelGroups		{}
	, baseChannelGroups	{}
	, singleSounds		{}
	, groupedSounds		{}
	, singleSoundNames	{}
	, groupedSoundNames	{}
	, listening			{ true }
	, listenerPosition	{}
{
	FMOD_ASSERT(FMOD::System_Create(&system));
	FMOD_ASSERT(system->init(MAX_CHANNELS, FMOD_INIT_NORMAL, 0));

	// Create a default channel group
	CreateChannelGroup(std::string{ defaultGroup });
	
	// Load sounds
	for (std::filesystem::directory_entry const& dir : std::filesystem::directory_iterator(SingleSoundFolder()))
	{
		const std::string filepath = dir.path().filename().string();
		std::string name{ RemoveExtension(dir.path().filename().string()) };

		// yc: looks like traces of a file manager? idk but Im following this first.
		FMOD::Sound* soundResource{ ResourceManager::GetSound(name) };
		if (soundResource) { 
			ResourceManager::DeleteSound(name); 
			CONSOLE_LOG(LEVEL_WARNING) << "An existing sound with name " << name << " was found! Replacing the original..."; 
		}

		// TODO: When a centralized file manager is decided, naturally this load function would 
		// be put inside it and called as a single function instead and using the struct
		FMOD_ASSERT(system->createSound((SingleSoundFolder() + dir.path().filename().string()).c_str(), FMOD_2D, nullptr, &soundResource));
		ResourceManager::LoadSound(name, soundResource);

		sounds.emplace(name,Audio(name, soundResource));
		singleSoundNames.emplace(name);
	}

	// Subscribe to OnWindowFocus
	Messaging::Subscribe("OnWindowFocus", AudioManager::WindowFocusPauseResume);
}

AudioManager::~AudioManager()
{
	ReleaseChannelGroups();
	FMOD_ASSERT(system->close());
	FMOD_ASSERT(system->release());
}

void AudioManager::Initialise()
{
	// Nothing required as singleton instance will take care of constructor call...
}

#pragma region Public Interface
void AudioManager::StartSingleSound(std::string const& name, bool loop, std::optional<Vec3> const& position, float volume)
{
	if (!listening) { return; }

	auto entry = sounds.find(name);
	if (entry == sounds.end())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Tried to start invalid sound: " << name;
	}
	
	FMOD::Sound* sound = entry->second.sound;
	FMOD::ChannelGroup* channelGroup = channelGroups[entry->first];
	channels[name].push_back(StartSound(sound, channelGroup, loop, position, volume));
}

void AudioManager::SetChannelGroup(std::string const& soundName, std::string const& group)
{
	if (channelGroups.find(group) == channelGroups.end())
	{
		CreateChannelGroup(group);
	}

	if (singleSounds.find(soundName) != singleSounds.end())
	{
		singleSounds[soundName].second = channelGroups[group];
	}
	else if (groupedSounds.find(soundName) != groupedSounds.end())
	{
		groupedSounds[soundName].second = channelGroups[group];
	}
	else
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Tried to modify invalid sound: " << soundName;
	}
}

void AudioManager::CreateUpdateSound(std::string const& filename, bool isGrouped)
{
	// Create a name without file extension
	std::string name{ RemoveExtension(filename) };

	// If it is a grouped sound, we can also generate a base name
	std::string baseName{ RemoveTrailingNumbers(name) };

	// If ResourceManager already has the sound, destroy it first
	FMOD::Sound* soundResource{ ResourceManager::GetSound(name) };

	// ResourceManager does not discern single from grouped sounds, so this is ok
	if (soundResource) { ResourceManager::DeleteSound(name); }

	// If the sound was previously assigned to a channel group, we should keep track of it
	FMOD::ChannelGroup* channelGroup{ isGrouped ? groupedSounds[baseName].second : singleSounds[name].second };

	// If no group was assigned, we give it the default channel group
	if (!channelGroup) { channelGroup = channelGroups[std::string{ defaultGroup }]; }

	// Get either folder name depending on input boolean
	std::string folderName{ isGrouped ? GroupedSoundFolder() : SingleSoundFolder()};

	// Use FMOD system to create a new sound
	result = system->createSound((folderName + filename).c_str(), FMOD_2D, nullptr, &soundResource);
	if (result != FMOD_OK)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to load file: " << folderName + filename << " : " << FMOD_ErrorString(result);
		return;
	}

	if (isGrouped)
	{
		// Store in groupedSounds
		groupedSounds[baseName].first[name] = soundResource;
		groupedSounds[baseName].second = channelGroup;
		groupedSoundNames[baseName].erase(name); // Erase first to clear if it already exists
		groupedSoundNames[baseName].insert(name);
		CONSOLE_LOG(LEVEL_DEBUG) << "GroupedSound Create/Update: " << baseName << " | " << name;
	}
	else
	{
		// Store in singleSounds
		singleSounds[name].first = soundResource;
		singleSounds[name].second = channelGroup;
		singleSoundNames.erase(name); // Erase first to clear if it already exists
		singleSoundNames.insert(name);
		CONSOLE_LOG(LEVEL_DEBUG) << "SingleSound Create/Update: " << name;
	}
	
	// Store in ResourceManager
	ResourceManager::LoadSound(name, soundResource);
}

void AudioManager::DeleteSound(std::string const& filename, bool isGrouped)
{
	// Create a name without file extension
	std::string name{ RemoveExtension(filename) };

	if (isGrouped) // Grouped Sounds
	{
		// If it is a grouped sound, we can also generate a base name
		std::string baseName{ RemoveTrailingNumbers(name) };

		// Error checking
		if (groupedSounds.find(baseName) == groupedSounds.end())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Tried to delete sound that doesn't exist: " << baseName;
			return;
		}
		if (groupedSounds[baseName].first.find(name) == groupedSounds[baseName].first.end())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Tried to delete sound that doesn't exist: " << name;
			return;
		}
		
		// Now we can finally erase it
		groupedSounds[baseName].first.erase(name);
		groupedSoundNames[baseName].erase(name);

		// If it's empty, we erase the parent container
		if (groupedSounds[baseName].first.empty())
		{
			groupedSounds.erase(baseName);
			groupedSoundNames.erase(baseName);
		}
	}
	else // Single Sounds
	{
		// Error checking
		if (singleSounds.find(name) == singleSounds.end())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Tried to delete sound that doesn't exist: " << name;
			return;
		}

		// Erase
		singleSounds.erase(name);
		singleSoundNames.erase(name);
	}

	// If ResourceManager has the sound, destroy it
	FMOD::Sound* soundResource{ ResourceManager::GetSound(name) };

	// ResourceManager does not discern single from grouped sounds, so this is ok
	if (soundResource) { ResourceManager::DeleteSound(name); }

	// Delete from the directory too
	DeleteSoundFromDirectory(filename, isGrouped);
}

void AudioManager::StopSound(std::string const& name)
{
	std::vector<FMOD::Channel*>& v = channels[name];
	for (FMOD::Channel* channel : v)
	{
		channel->stop();
	}
}

void AudioManager::StopAllSounds()
{
	for (auto& pair : channels)
	{
		for (FMOD::Channel* channel : pair.second)
		{
			channel->stop();
		}
	}
}

void AudioManager::PauseAllSounds()
{
	for (auto& pair : channels)
	{
		for (FMOD::Channel* channel : pair.second)
		{
			channel->setPaused(true);
		}
	}
}

void AudioManager::ResumeAllSounds()
{
	for (auto& pair : channels)
	{
		for (FMOD::Channel* channel : pair.second)
		{
			channel->setPaused(false);
		}
	}
}

void AudioManager::SetSoundVolume(std::string const& name, float volume)
{
	std::vector<FMOD::Channel*>& v = channels[name];
	for (FMOD::Channel* channel : v)
	{
		channel->setVolumeRamp(true);
		channel->setVolume(volume);
	}
}

float AudioManager::GetGroupVolume(std::string const& group)
{
	if (channelGroups.find(group) == channelGroups.end())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Tried to read invalid group: " << group;
		return -1.0f;
	}
	FMOD::ChannelGroup* channelGroup = channelGroups[group];
	float ret{};
	channelGroup->getVolume(&ret);
	return ret;
}

float AudioManager::GetGroupPitch(std::string const& group)
{
	if (channelGroups.find(group) == channelGroups.end())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Tried to read invalid group: " << group;
		return -1.0f;
	}
	FMOD::ChannelGroup* channelGroup = channelGroups[group];
	float ret{};
	channelGroup->getPitch(&ret);
	return ret;
}

void AudioManager::SetGroupVolume(float volume, std::string const& group)
{
	// If unable to find group, create it first
	if (channelGroups.find(group) == channelGroups.end())
	{
		CreateChannelGroup(group);
	}
	FMOD::ChannelGroup* channelGroup = channelGroups[group];
	channelGroup->setVolume(volume);
}

void AudioManager::SetGroupPitch(float pitch, std::string const& group)
{
	if (channelGroups.find(group) == channelGroups.end())
	{
		CreateChannelGroup(group);
	}
	FMOD::ChannelGroup* channelGroup = channelGroups[group];
	channelGroup->setPitch(pitch);
}

void AudioManager::InterpolateGroupVolume(float targetVolume, float duration, std::string const& group)
{
	if (channelGroups.find(group) == channelGroups.end())
	{
		CreateChannelGroup(group);
	}
	FMOD::ChannelGroup* channelGroup = channelGroups[group];

	// Get sample rate
	int sampleRate = 0;
	system->getSoftwareFormat(&sampleRate, nullptr, nullptr);

	// Get current DSP clock
	unsigned long long clock = 0;
	channelGroup->getDSPClock(nullptr, &clock);

	// Calculate fade points
	unsigned long long start = clock;
	unsigned long long end = start + static_cast<unsigned long long>(duration * sampleRate);

	// Apply
	channelGroup->addFadePoint(start, GetGroupVolume(group));
	channelGroup->addFadePoint(end, targetVolume);
}
bool AudioManager::IsSoundPlaying(std::string const& name)
{
	if (channels.find(name) == channels.end())
	{
		return false;
	}

	// Iterate all channels under this name
	for (FMOD::Channel* channel : channels[name])
	{
		bool isPlaying{ false };
		channel->isPlaying(&isPlaying);
		if (isPlaying) { return true; } // Return true if it's still playing
	}
	return false;
}

void AudioManager::ReleaseChannelGroups()
{
	// Iterate through all soundResource groups 
	for (auto it = channelGroups.begin(); it != channelGroups.end(); ++it)
	{
		// Release resources
		it->second->release();
		it->second = nullptr;
	}
	channelGroups.clear();
}

void AudioManager::SetIsListening(bool isListening)
{
	listening = isListening;
}

void AudioManager::UpdateListenerAttributes(Vec3 const& position)
{
	listenerPosition = position;
}

void AudioManager::UpdateSpatialProperties(float minDistance, float maxDistance, float dopplerScale, float distanceFactor, float rolloffScale)
{
	for (auto& elem : singleSounds)
	{
		elem.second.first->set3DMinMaxDistance(minDistance, maxDistance);
	}

	for (auto& cont : groupedSounds)
	{
		for (auto& elem : cont.second.first)
		{
			elem.second->set3DMinMaxDistance(minDistance, maxDistance);
		}
	}
	system->set3DSettings(dopplerScale, distanceFactor, rolloffScale);
}

void AudioManager::UpdateSystem()
{
	system->update();
}

void AudioManager::SetBaseVolume(std::string channelGroupName, float volume)
{
	// If unable to find group, create it first
	if (channelGroups.find(channelGroupName) == channelGroups.end())
	{
		CreateChannelGroup(channelGroupName);
	}
	baseChannelGroups[channelGroupName]->setVolume(volume);
}

float AudioManager::GetBaseVolume(std::string channelGroupName)
{
	float volume;
	baseChannelGroups[channelGroupName]->getVolume(&volume);
	return volume;
}

FMOD::ChannelGroup* AudioManager::CreateChannelGroup(std::string const& name)
{
	// If it already exists, return
	if (channelGroups[name])
	{
		CONSOLE_LOG(LEVEL_DEBUG) << "No new Channel Group created as it already exists: " << name;
		return channelGroups[name];
	}

	// Else create a new channel group
	FMOD::ChannelGroup* group{ nullptr };
	FMOD_ASSERT(system->createChannelGroup(name.c_str(), &group));
	group->setVolume(1.0f);
	group->setPitch(1.0f);

	// Create a compressor (because why not)
	FMOD::DSP* compressor = nullptr;
	FMOD_ASSERT(system->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressor));

	// Set DSP params
	compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_ATTACK, 20.0f);
	compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_GAINMAKEUP, 0.0f);
	compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_RATIO, 4.0f);
	compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_RELEASE, 20.0f);
	compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, -20.0f);

	// Apply DSP
	FMOD_ASSERT(group->addDSP(0, compressor));
	compressor->setActive(true);

	// Whenever a new channel group is created, also create a base channel group and nest within it
	FMOD::ChannelGroup* baseGroup{ nullptr };
	FMOD_ASSERT(system->createChannelGroup(name.c_str(), &baseGroup));
	baseGroup->setVolume(1.0f);
	baseGroup->setPitch(1.0f);
	baseGroup->addGroup(group, false);
	baseChannelGroups[name] = baseGroup;

	return channelGroups[name] = group;
}
#pragma endregion

#pragma region Internal Functions
FMOD::Channel* AudioManager::StartSound(FMOD::Sound* sound, FMOD::ChannelGroup* channelGroup, bool loop, std::optional<Vec3> const& position, float volume)
{
	FMOD::Channel* channel{};

	// Potentially free up some FMOD resources
	// yc: what the fuck, there is no way a O(n^2) loop can be more efficient, is this for real? Can someone check?
	//CleanChannels();

	if (loop) { channel->setMode(FMOD_LOOP_NORMAL); }
	
	if (position)
	{
		channel->setMode(FMOD_3D);
		FMOD_VECTOR pos{ position->x - listenerPosition.x, position->y - listenerPosition.y, position->z - listenerPosition.z };
		FMOD_VECTOR vel{ 0.0f, 0.0f, 0.0f };
		FMOD_ASSERT(channel->set3DAttributes(&pos, &vel));
	}

	channel->setVolume(volume);
	FMOD_ASSERT(system->playSound(sound, channelGroup, false, &channel));
	
	return channel;
}

void AudioManager::CleanChannels()
{
	// Iterate through the unordered map of vectors
	for (auto i = channels.begin(); i != channels.end(); ++i)
	{
		// Reference the vector
		std::vector<FMOD::Channel*>& v = i->second;

		// Iterate all FMOD::Channel* in the vector
		for (auto j = v.begin(); j != v.end();)
		{
			FMOD::Channel* channel = *j;

			// If nullptr, just erase and continue
			if (!channel)
			{
				j = v.erase(j);
				continue;
			}
			bool isPlaying{ false };
			channel->isPlaying(&isPlaying);

			// If not playing, stop and erase
			if (!isPlaying)
			{
				channel->stop();
				j = v.erase(j);
			}
			else // We only have to increment the iterator if nothing was erased
			{
				++j;
			}
		}
	}
}

void AudioManager::WindowFocusPauseResume(bool isFocused)
{
	ST<AudioManager>::Get()->SetIsListening(isFocused);
	if (isFocused)
	{
		ST<AudioManager>::Get()->ResumeAllSounds();
	}
	else
	{
		ST<AudioManager>::Get()->PauseAllSounds();
	}
}

std::string AudioManager::RemoveTrailingNumbers(std::string const& name)
{
	size_t pos = name.size();

	while (pos > 0 && std::isdigit(name[pos - 1])) { --pos; }

	return name.substr(0, pos);
}

std::string AudioManager::RemoveExtension(std::string const& filename)
{
	size_t pos = filename.find_last_of('.');

	if (pos != std::string::npos && pos != 0)
	{
		return filename.substr(0, pos);
	}
	return filename;
}

void AudioManager::DebugPrint()
{
	int i{};
	system->getChannelsPlaying(&i);
	CONSOLE_LOG(LEVEL_DEBUG) << "Sound Channels: " << i;
}

void AudioManager::DeleteSoundFromDirectory(std::string const& filename, bool isGrouped)
{
	std::filesystem::path current = std::filesystem::current_path();
	std::filesystem::path folder = isGrouped ? GroupedSoundFolder() : SingleSoundFolder();
	std::filesystem::path name = filename + ".wav";
	std::filesystem::path dst = current / folder / name;

	try
	{
		if (std::filesystem::exists(dst))
		{
			if (std::filesystem::remove(dst))
			{
				CONSOLE_LOG(LEVEL_DEBUG) << "File deleted successfully: " << name;
			}
			else
			{
				CONSOLE_LOG(LEVEL_WARNING) << "File delete failed: " << name;
			}
		}
		else
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Tried to delete a file that does not exist: " << dst;
		}
	}
	catch (std::filesystem::filesystem_error const& error)
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Filesystem error: " << error.what();
	}
	catch (std::exception const& error)
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Exception caught: " << error.what();
	}
}
#pragma endregion

AudioReference::AudioReference(const char* defaultName)
	: name{ defaultName }
{
}

AudioReference::operator const std::string&() const
{
	return name;
}

void AudioReference::EditorDraw(const char* label)
{
	gui::TextBoxReadOnly(label, ICON_FA_VOLUME_HIGH + name);

	gui::PayloadTarget<std::string>("SOUND", [&name = name](const std::string& soundName) -> void {
		name = soundName;
	});
}

const std::string& AudioManager::SingleSoundFolder() { return ST<Filepaths>::Get()->soundSingleFolder;}
const std::string& AudioManager::GroupedSoundFolder() { return ST<Filepaths>::Get()->soundGroupedFolder; }