// AudioManager manages the channels, channelgroup and sound playing requests

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
{
	//FMOD_ASSERT(FMOD::System_Create(&system));
	FMOD_ASSERT(FMOD::Studio::System::create(&fmod_studio));
	FMOD_ASSERT(fmod_studio->getCoreSystem(&system));
	FMOD_ASSERT(fmod_studio->initialize(MAX_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0));

	// Create channel groups for BGM and SFX
	FMOD_ASSERT(system->createChannelGroup("BGM", &channelGroups[0]));
	FMOD_ASSERT(system->createChannelGroup("SFX", &channelGroups[1]));
}

AudioManager::~AudioManager()
{
	FMOD_ASSERT(fmod_studio->release());
}

void AudioManager::Initialise()
{
	// Nothing required as singleton instance will take care of constructor call...
}

void AudioManager::StopAllSounds()
{

}

void AudioManager::SetBaseVolume(AudioType type, float vol)
{

}

void AudioManager::CreateSound(const std::string& name)
{
	FMOD::Sound* sound = nullptr;
	FMOD_ASSERT(system->createSound((ST<Filepaths>::Get()->soundFolder + name).c_str(), FMOD_DEFAULT, 0, &sound));
	ResourceManager::LoadSound(name, sound);
	soundNames.push_back(name);
}

void AudioManager::PlaySound(const std::string& name, bool loop, AudioType category)
{
	FMOD::Channel* channel = nullptr;
	FMOD_ASSERT(system->playSound(ResourceManager::GetSound(name), channelGroups[0], false, &channel));
	if (loop && channel) FMOD_ASSERT(channel->setMode(FMOD_LOOP_NORMAL));
}