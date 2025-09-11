#include "AudioManager.h"
#include <FMOD/fmod_errors.h>
#include "ResourceManager.h"

// Macro sprinkler for FMOD error, we don't need a function call stack for this
#define FMOD_ASSERT(FUNCTION) \
				result = FUNCTION;  \
				if (result != FMOD_OK) { \
						CONSOLE_LOG(LEVEL_ERROR) << "FMOD: (" << result << ") " << FMOD_ErrorString(result) << " at line " << __LINE__ << " in file " << __FILE__;}

// Creates and loads sounds on launch
AudioManager::AudioManager()
	: result			{}
	, system			{ nullptr }
{
	FMOD_ASSERT(FMOD::Studio::System::create(&fmod_studio));
	FMOD_ASSERT(fmod_studio->getCoreSystem(&system));
	FMOD_ASSERT(fmod_studio->initialize(MAX_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0));

	FMOD_ASSERT(system->getMasterChannelGroup(&masterChannelGroup));
	FMOD_ASSERT(system->createChannelGroup("BGM", &channelGroups[0]));
	FMOD_ASSERT(system->createChannelGroup("SFX", &channelGroups[1]));
	FMOD_ASSERT(masterChannelGroup->addGroup(channelGroups[0]));
	FMOD_ASSERT(masterChannelGroup->addGroup(channelGroups[1]));

	FMOD_ASSERT(system->set3DSettings(1.0, 1.0f, 1.0f));
}

AudioManager::~AudioManager()
{
	FMOD_ASSERT(fmod_studio->release());
}

void AudioManager::Initialise()
{
	// Empty for now
}

void AudioManager::Update()
{
	FMOD_ASSERT(fmod_studio->update());
}

void AudioManager::StopAllSounds()
{
	channelManager.channelMap.clear(); 
	masterChannelGroup->stop();
}

void AudioManager::SetGroupVolume(AudioType type, float vol)
{
	switch (type)
	{
		case AudioType::BGM:
			channelGroups[0]->setVolume(vol);
			break;
		case AudioType::SFX:
			channelGroups[1]->setVolume(vol);
			break;
	}

	// Ensure master volume is never lower than any category volume, because that makes no sense
	float masterVol;
	masterChannelGroup->getVolume(&masterVol);
	if (masterVol < vol)
	{
		masterChannelGroup->setVolume(vol);
	}
}

void AudioManager::CreateSound(const std::string& name)
{
	// yc: this should be delegated to ResourceManager, when we have a proper asset management system
	FMOD::Sound* sound = nullptr;
	FMOD_ASSERT(system->createSound((ST<Filepaths>::Get()->soundFolder + name).c_str(), FMOD_DEFAULT, 0, &sound));

	AudioAsset soundAsset(sound, name);
	sound->setUserData((void*)&soundAsset.data); // Setup using fmod's own internal user data system for easy retrieval
	
	ResourceManager::LoadSound(name, soundAsset);
	soundNames.push_back(name);
}

void AudioManager::FreeSound(FMOD::Sound* sound)
{
	StopAllSounds(); // Ensure no sounds are playing before we free, as a channel might still be referencing it
	if (sound)
		FMOD_ASSERT(sound->release());
}

uint32_t AudioManager::PlaySound(const std::string& name, bool loop, AudioType category)
{
	FMOD::Sound* sound = ResourceManager::GetSound(name).sound;
	FMOD::Channel* channel = nullptr;

	switch(category)
	{
		case AudioType::BGM:
			FMOD_ASSERT(system->playSound(sound, channelGroups[0], false, &channel));
			break;
		case AudioType::SFX:
			FMOD_ASSERT(system->playSound(sound, channelGroups[1], false, &channel));
			break;
		case AudioType::END:
			FMOD_ASSERT(system->playSound(sound, masterChannelGroup, false, &channel));
			break; // Bypass to universal play
	}

	if (loop) 
		FMOD_ASSERT(channel->setMode(FMOD_LOOP_NORMAL));

	return channelManager.RegisterChannel(channel);
}

uint32_t AudioManager::PlaySound3D(const std::string& name, bool loop, Vec3 position, AudioType category, std::pair<float, float> rolloff)
{
	FMOD::Sound* sound = ResourceManager::GetSound(name).sound;
	FMOD::Channel* channel = nullptr;

	switch (category)
	{
	case AudioType::BGM:
		FMOD_ASSERT(system->playSound(sound, channelGroups[0], true, &channel));
		break;
	case AudioType::SFX:
		FMOD_ASSERT(system->playSound(sound, channelGroups[1], true, &channel));
		break;
	case AudioType::END:
		FMOD_ASSERT(system->playSound(sound, masterChannelGroup, true, &channel));
		break; // Bypass to universal play
	}

	// Linear rolloff assumes complete silence at max distance - can potentially look at custom curves later/allowing user to define their own curves
	FMOD_ASSERT(channel->setMode(FMOD_3D | FMOD_3D_LINEARROLLOFF));

	if (loop) 
		FMOD_ASSERT(channel->setMode(FMOD_LOOP_NORMAL));

	FMOD_ASSERT(channel->set3DMinMaxDistance(rolloff.first, rolloff.second));

	FMOD_VECTOR initialVel = { 0.0f, 0.0f, 0.0f };
	FMOD_VECTOR fmodVecPos = { position.x, position.y, position.z };
    FMOD_ASSERT(channel->set3DAttributes(&fmodVecPos, &initialVel));

	FMOD_ASSERT(channel->setPaused(false));

	return channelManager.RegisterChannel(channel);
}

void AudioManager::StopSound(uint32_t id)
{
	if (channelManager.channelMap.find(id) != channelManager.channelMap.end())
	{
		if (channelManager.channelMap[id].channel)
		{
			FMOD_ASSERT(channelManager.channelMap[id].channel->stop());
			channelManager.channelMap.erase(id);
		}
	}
}

bool AudioManager::IsPlaying(uint32_t id)
{
	if (channelManager.channelMap.find(id) == channelManager.channelMap.end())
	{
		return false;
	}

	bool isPlaying = true;

	// FMOD automatically clears channels, but it does not auto clear our own handle, so an invalid error will occur due to the auto cleanup, need to check for this. 
	FMOD_RESULT channel_status = channelManager.channelMap[id].channel->isPlaying(&isPlaying);
	if (channel_status == FMOD_ERR_INVALID_HANDLE)
	{
		channelManager.channelMap.erase(id);
		return false;
	}
	else if (channel_status != FMOD_OK)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "FMOD: (" << channel_status << ") " << FMOD_ErrorString(channel_status) << " at line " << __LINE__ << " in file " << __FILE__;
		return false;
	}

	return isPlaying;
}

void AudioManager::UpdateListener(const Vec3& pos, const Vec3& vel)
{
	FMOD_VECTOR fmodPos = { pos.x, pos.y, pos.z };
	FMOD_VECTOR fmodVel = { vel.x, vel.y, vel.z };
    FMOD_VECTOR forward = { 0.0f, 0.0f, -1.0f };		// Seems we are using Z backwards. This ensures that positive x value = sound comes from right ear
    FMOD_VECTOR up      = { 0.0f, 1.0f, 0.0f };

    FMOD_ASSERT(system->set3DListenerAttributes(0, &fmodPos, &fmodVel, &forward, &up));
}

void AudioManager::ConfigureListener(float dopplerScale, float distanceFactor, float rolloffScale)
{
	FMOD_ASSERT(system->set3DSettings(dopplerScale, distanceFactor, rolloffScale));
}

const std::vector<std::string>& AudioManager::GetSoundNames() const
{
	return soundNames;
}

FMOD::Channel* AudioManager::CreateChannel(FMOD::Sound* sound, AudioType type)
{
	FMOD::Channel* channel = nullptr;
	switch(type)
	{
		case AudioType::BGM:
			FMOD_ASSERT(system->playSound(sound, channelGroups[0], true, &channel));
			break;
		case AudioType::SFX:
			FMOD_ASSERT(system->playSound(sound, channelGroups[1], true, &channel));
			break;
		case AudioType::END:
			FMOD_ASSERT(system->playSound(sound, masterChannelGroup, true, &channel));
			break; // Bypass to universal play
	}

	return channel;
}

void AudioManager::SetChannelPosition(uint32_t channel, const Vec3& pos, const Vec3& vel)
{
	if (channelManager.channelMap.find(channel) != channelManager.channelMap.end())
	{
		FMOD_VECTOR f_pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR f_vel = { vel.x, vel.y, vel.z };
		FMOD_ASSERT(channelManager.channelMap[channel].channel->set3DAttributes(&f_pos, &f_vel));
	}
}

const unsigned int AudioManager::GetChannelPosition(uint32_t channel)
{
	unsigned int currentPos;
	if (channelManager.channelMap.find(channel) != channelManager.channelMap.end())
	{
		channelManager.channelMap[channel].channel->getPosition(&currentPos, FMOD_TIMEUNIT_MS);
	}
	return currentPos;
}

FMOD::Sound* AudioManager::GetSound(uint32_t channel) const
{
	FMOD::Sound* sound = nullptr;
	if (channelManager.channelMap.find(channel) != channelManager.channelMap.end())
	{
		channelManager.channelMap.at(channel).channel->getCurrentSound(&sound);
	}
	return sound;
}

void AudioManager::SetChannel3D(uint32_t channel, bool is3D)
{
	if (channelManager.channelMap.find(channel) != channelManager.channelMap.end())
	{
		if (is3D)
		{
			FMOD_ASSERT(channelManager.channelMap[channel].channel->setMode(FMOD_3D));
		}
		else
		{
			FMOD_ASSERT(channelManager.channelMap[channel].channel->setMode(FMOD_2D));
		}
	}
}

void AudioManager::SetChannel3DAttributes(uint32_t channel, const Vec3& pos, const Vec3& vel)
{
	if (!IsChannel3D(channel))
		return;

	if (channelManager.channelMap.find(channel) != channelManager.channelMap.end())
	{
		FMOD_VECTOR f_pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR f_vel = { vel.x, vel.y, vel.z };
		FMOD_ASSERT(channelManager.channelMap[channel].channel->set3DAttributes(&f_pos, &f_vel));
	}
}

void AudioManager::SetVolume(uint32_t channel, float vol)
{
	if (channelManager.channelMap.find(channel) != channelManager.channelMap.end())
	{
		FMOD_ASSERT(channelManager.channelMap[channel].channel->setVolume(vol));
	}
}

uint32_t AudioManager::ChannelManager::GetNextHandle()
{
	uint32_t handle = currentChannelIndex;
	currentChannelIndex++;
	while (currentChannelIndex == INVALID_HANDLE || channelMap.find(currentChannelIndex) != channelMap.end()) 
	{
		currentChannelIndex++;
	}
	return handle;
}

uint32_t AudioManager::ChannelManager::RegisterChannel(FMOD::Channel* channel, float minDist, float maxDist, float doppler, float distFactor, float rolloff)
{
	uint32_t handle = GetNextHandle();
	channelMap[handle] = { channel, minDist, maxDist, doppler, distFactor, rolloff };

	return handle;
}

bool AudioManager::IsChannel3D(uint32_t handle)
{
	if (channelManager.channelMap.find(handle) != channelManager.channelMap.end())
	{
		FMOD_MODE mode;
		FMOD_ASSERT(channelManager.channelMap[handle].channel->getMode(&mode));
		return (mode & FMOD_3D) != 0;
	}
	return false;
}