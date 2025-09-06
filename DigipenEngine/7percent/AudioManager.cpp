// 
// AudioManager manages the channelgroup and sound playing requests
// Management of the actual channel is done by the component and is the responsibility of the component to stop/start sounds
// AudioManager controls through channelgroups
//

#include "AudioManager.h"
#include <FMOD/fmod_errors.h>
#include "ResourceManager.h"

// Macro sprinkler for FMOD error, we don't need a function call stack for this
#define FMOD_ASSERT(FUNCTION) \
				result = FUNCTION;  \
				if (result != FMOD_OK) { \
						CONSOLE_LOG(LEVEL_ERROR) << "FMOD: (" << result << ") " << FMOD_ErrorString(result) << "at line " << __LINE__;}

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
	masterChannelGroup->stop();
}

void AudioManager::SetBaseVolume(AudioType type, float vol)
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
	ResourceManager::LoadSound(name, sound);
	soundNames.push_back(name);
}

void AudioManager::FreeSound(FMOD::Sound* sound)
{
	StopAllSounds(); // Ensure no sounds are playing before we free, as a channel might still be referencing it
	if (sound)
		FMOD_ASSERT(sound->release());
}

FMOD::Channel* AudioManager::PlaySound(const std::string& name, bool loop, AudioType category)
{
	FMOD::Channel* channel = nullptr;
	switch(category)
	{
		case AudioType::BGM:
			FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), channelGroups[0], false, &channel));
			break;
		case AudioType::SFX:
			FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), channelGroups[1], false, &channel));
			break;
		case AudioType::END:
			FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), masterChannelGroup, false, &channel));
			break; // Bypass to universal play
	}

	if (loop && channel) 
		FMOD_ASSERT(channel->setMode(FMOD_LOOP_NORMAL));

	return channel;
}

FMOD::Channel* AudioManager::PlaySound3D(const std::string& name, bool loop, Vec3 position, AudioType category)
{
	FMOD::Channel* channel = nullptr;
	switch (category)
	{
	case AudioType::BGM:
		FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), channelGroups[0], true, &channel));
		break;
	case AudioType::SFX:
		FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), channelGroups[1], true, &channel));
		break;
	case AudioType::END:
		FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), masterChannelGroup, true, &channel));
		break; // Bypass to universal play
	}

	FMOD_ASSERT(channel->setMode(FMOD_3D));
	if (loop) 
		FMOD_ASSERT(channel->setMode(FMOD_LOOP_NORMAL));

	FMOD_ASSERT(channel->set3DMinMaxDistance(2.0f, 200.0f));

	FMOD_VECTOR initialVel = { 0.0f, 0.0f, 0.0f };
	FMOD_VECTOR fmodVecPos = { position.x, position.y, position.z };
    FMOD_ASSERT(channel->set3DAttributes(&fmodVecPos, &initialVel));

	FMOD_ASSERT(channel->setPaused(false));

	return channel;
}

void AudioManager::UpdateListener(const FMOD_VECTOR& pos, const FMOD_VECTOR& vel)
{
    // 'forward' and 'up' vectors define the listener's orientation.
    FMOD_VECTOR forward = { 0.0f, 0.0f, 1.0f };
    FMOD_VECTOR up      = { 0.0f, 1.0f, 0.0f };

    // Set the listener's attributes.
    FMOD_ASSERT(system->set3DListenerAttributes(0, &pos, &vel, &forward, &up));
}

const std::vector<std::string>& AudioManager::GetSoundNames() const
{
	return soundNames;
}