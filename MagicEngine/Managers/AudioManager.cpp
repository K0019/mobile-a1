#include "Managers/AudioManager.h"
#include <FMOD/fmod_errors.h>
#include "Assets/AssetManager.h"
#include "VFS/VFS.h"
#include "VFS/IFileStream.h"
#include "Engine/Audio.h"

// Macro sprinkler for FMOD error, we don't need a function call stack for this
// If condition tests against FMOD_OK (which is 0)
#ifdef _DEBUG
#define FMOD_ASSERT(FUNCTION) \
	if (FMOD_RESULT result{ FUNCTION }) \
			CONSOLE_LOG(LEVEL_ERROR) << "FMOD: (" << result << ") " << FMOD_ErrorString(result) << " at line " << __LINE__ << " in file " << __FILE__; 
#else
#define FMOD_ASSERT(FUNCTION) \
	FUNCTION
#endif

// Creates and loads sounds on launch
AudioManager::AudioManager()
	: system{}
	, fmod_studio{}
	, masterChannelGroup{}
	, channelGroups{}
	, channelManager{ static_cast<uint16_t>(MAX_CHANNELS) }
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

#ifdef __ANDROID__
	// Setup the VFS callbacks for FMOD on Android
	FMOD_ASSERT(system->setFileSystem(
		AudioManager::FileOpenCallback,
		AudioManager::FileCloseCallback,
		AudioManager::FileReadCallback,
		AudioManager::FileSeekCallback,
		nullptr,
		nullptr,
		-1
	));
#endif
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
	// Update listeners first, before calling update
	auto begin = ecs::GetCompsActiveBegin<AudioListenerComponent>();
	auto end = ecs::GetCompsEnd<AudioListenerComponent>();
	if (begin != end)
	{
		UpdateListener(begin.GetEntity()->GetTransform().GetWorldPosition(), Vec3{0.0f, 0.0f, 0.0f});
	}
	FMOD_ASSERT(fmod_studio->update());
}

void AudioManager::StopAllSounds()
{
	channelManager.ClearAllChannels();
	masterChannelGroup->stop();
}

void AudioManager::OnAppPause()
{
	FMOD_ASSERT(system->mixerSuspend());
}

void AudioManager::OnAppResume()
{
	FMOD_ASSERT(system->mixerResume());
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
		default:
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

FMOD::Sound* AudioManager::CreateSound(const std::string& filepath)
{
	FMOD::Sound* sound = nullptr;
	FMOD_ASSERT(system->createSound(filepath.c_str(), FMOD_DEFAULT, 0, &sound));
	return sound;
}

FMOD::Sound* AudioManager::CreateSoundFromData(const char* data, size_t length)
{
	FMOD::Sound* sound = nullptr;
	
	FMOD_CREATESOUNDEXINFO exinfo;
	memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
	exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfo.length = (unsigned int)length;

	FMOD_ASSERT(system->createSound(data, FMOD_OPENMEMORY, &exinfo, &sound));
	return sound;
}

void AudioManager::FreeSound(FMOD::Sound* sound)
{
	StopAllSounds(); // Ensure no sounds are playing before we free, as a channel might still be referencing it
	if (sound)
		FMOD_ASSERT(sound->release());
}

uint32_t AudioManager::PlaySound(const std::string& filename, bool loop, AudioType category, float volumeModifier)
{
	return PlaySound(util::GenHash(filename) | 1, loop, category, volumeModifier);
}

uint32_t AudioManager::PlaySound(size_t audioResourceHash, bool loop, AudioType category, float volumeModifier)
{
	const auto* resource{ ST<AssetManager>::Get()->GetContainer<ResourceAudio>().GetResource(audioResourceHash)};
	if (!resource)
		return 0;

	FMOD::Sound* sound = resource->sound;
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

	FMOD_ASSERT(channel->setVolume(volumeModifier));

	return channelManager.RegisterChannel(channel);
}
uint32_t AudioManager::PlaySound3D(const std::string& filename, bool loop, Vec3 position, AudioType category, std::pair<float, float> rolloff, float volumeModifier)
{
	return PlaySound3D(util::GenHash(filename) | 1, loop, position, category, rolloff, volumeModifier);
}

uint32_t AudioManager::PlaySound3D(size_t audioResourceHash, bool loop, Vec3 position, AudioType category, std::pair<float, float> rolloff, float volumeModifier)
{
	const auto* resource{ ST<AssetManager>::Get()->GetContainer<ResourceAudio>().GetResource(audioResourceHash) };
	if (!resource)
		return 0;

	FMOD::Sound* sound = resource->sound;
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

	FMOD_ASSERT(channel->setVolume(volumeModifier));

	FMOD_ASSERT(channel->setPaused(false));

	return channelManager.RegisterChannel(channel);
}

void AudioManager::StopSound(uint32_t handle)
{
	if (auto channel{ channelManager.GetChannel(handle) })
	{
		channel->stop();
		channelManager.DeleteChannel(handle);
	}
}

bool AudioManager::IsPlaying(uint32_t handle)
{
	auto channel{ channelManager.GetChannel(handle) };
	if (!channel)
		return false;

	bool isPlaying = true;

	// FMOD automatically clears channels, but it does not auto clear our own handle, so an invalid error will occur due to the auto cleanup, need to check for this. 
	FMOD_RESULT channel_status = channel->isPlaying(&isPlaying);
	if (channel_status == FMOD_ERR_INVALID_HANDLE)
	{
		channelManager.DeleteChannel(handle);
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

void AudioManager::SetChannelPosition(uint32_t handle, const Vec3& pos, const Vec3& vel)
{
	if (auto channel{ channelManager.GetChannel(handle) })
	{
		FMOD_VECTOR f_pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR f_vel = { vel.x, vel.y, vel.z };
		FMOD_ASSERT(channel->set3DAttributes(&f_pos, &f_vel));
	}
}

const unsigned int AudioManager::GetChannelPosition(uint32_t handle) const
{
	unsigned int currentPos = 0;
	if (auto channel{ channelManager.GetChannel(handle) })
		channel->getPosition(&currentPos, FMOD_TIMEUNIT_MS);
	return currentPos;
}

FMOD::Sound* AudioManager::GetSound(uint32_t handle) const
{
	FMOD::Sound* sound{};
	if (auto channel{ channelManager.GetChannel(handle) })
		FMOD_ASSERT(channel->getCurrentSound(&sound));

	return sound;
}

void AudioManager::SetChannel3D(uint32_t handle, bool is3D)
{
	if (auto channel{ channelManager.GetChannel(handle) })
		FMOD_ASSERT(channel->setMode(is3D ? FMOD_3D : FMOD_2D));
}

bool AudioManager::IsChannel3D(uint32_t handle) const
{
	if (auto channel{ channelManager.GetChannel(handle) })
	{
		FMOD_MODE mode;
		FMOD_ASSERT(channel->getMode(&mode));
		return (mode & FMOD_3D) != 0;
	}
	return false;
}

void AudioManager::SetChannel3DAttributes(uint32_t handle, const Vec3& pos, const Vec3& vel)
{
	if (!IsChannel3D(handle))
		return;

	if (auto channel{ channelManager.GetChannel(handle) })
	{
		FMOD_VECTOR f_pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR f_vel = { vel.x, vel.y, vel.z };
		FMOD_ASSERT(channel->set3DAttributes(&f_pos, &f_vel));
	}
}

void AudioManager::SetVolume(uint32_t handle, float vol)
{
	if (auto channel{ channelManager.GetChannel(handle) })
		FMOD_ASSERT(channel->setVolume(vol));
}

AudioManager::ChannelManager::ChannelManager(uint16_t maxChannels)
	: maxChannels{ maxChannels }
{
}

uint32_t AudioManager::ChannelManager::RegisterChannel(FMOD::Channel* channel)
{
	if (channelMap.size() >= maxChannels)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "AudioManager: Exceeded maximum number of concurrent channels!";
		return INVALID_HANDLE;
	}

	uint32_t handle = util::Rand_UID_32();
	while (channelMap.find(handle) != channelMap.end())
		handle = util::Rand_UID_32();

	channelMap[handle] = channel;
	return handle;
}

FMOD::Channel* AudioManager::ChannelManager::GetChannel(uint32_t handle) const
{
	auto channelIter{ channelMap.find(handle) };
	return (channelIter != channelMap.end() ? channelIter->second : nullptr);
}

void AudioManager::ChannelManager::DeleteChannel(uint32_t handle)
{
	channelMap.erase(handle);
}

void AudioManager::ChannelManager::ClearAllChannels()
{
	channelMap.clear();
}

void AudioManager::FadeoutAudio(uint32_t handle, [[maybe_unused]] float seconds)
{
	unsigned long long dspclock = 0;
	int rate;

	auto* channel = channelManager.GetChannel(handle);
	channel->getSystemObject(&system);
	system->getSoftwareFormat(&rate, nullptr, nullptr);
	channel->getDSPClock(nullptr, &dspclock);
	channel->addFadePoint(dspclock, 1.0f);  // Current volume
	channel->addFadePoint(dspclock + (rate * 5), 0.0f);  // 0 volume after 5 seconds
	channel->setDelay(0, dspclock + (rate * 5), true);  // Stop channel after fade
}


#ifdef __ANDROID__
// FMOD's FileOpen Callback
FMOD_RESULT F_CALLBACK AudioManager::FileOpenCallback(const char* name, unsigned int* filesize, void** handle, void* userdata)
{
	if (!name || !filesize || !handle)
		return FMOD_ERR_INVALID_PARAM;

	std::unique_ptr<IFileStream> fileStream = VFS::OpenFile(name, FileMode::Read);

	if (!fileStream)
	{
		*filesize = 0;
		*handle = nullptr;
		return FMOD_ERR_FILE_NOTFOUND;
	}

	int64_t size = fileStream->GetSize();

	// Check for overflow (files > 2GB)
	if (size > INT_MAX)
	{
		*filesize = 0;
		*handle = nullptr;
		return FMOD_ERR_FILE_BAD;
	}

	*filesize = static_cast<int>(size);
	*handle = fileStream.release();

	return FMOD_OK;
}

// FMOD's FileClose Callback
FMOD_RESULT F_CALLBACK AudioManager::FileCloseCallback(void* handle, void* userdata)
{
	// The 'handle' is the raw IFileStream* pointer
	IFileStream* stream = static_cast<IFileStream*>(handle);

	// Use a unique_ptr to manage the memory and call the destructor/cleanup
	// (which for AndroidFileStream calls AAsset_close)
	std::unique_ptr<IFileStream> fileStream(stream);

	// fileStream goes out of scope and calls delete on 'stream'
	return FMOD_OK;
}

// FMOD's FileRead Callback
FMOD_RESULT F_CALLBACK AudioManager::FileReadCallback(void* handle, void* buffer, unsigned int sizebytes, unsigned int* bytesread, void* userdata)
{
	if (!handle || !buffer || !bytesread)
		return FMOD_ERR_INVALID_PARAM;

	IFileStream* stream = static_cast<IFileStream*>(handle);

	size_t actualRead = stream->Read(buffer, sizebytes);
	*bytesread = static_cast<unsigned int>(actualRead);

	if (actualRead == 0 && stream->IsEOF())
		return FMOD_ERR_FILE_EOF;

	return FMOD_OK;
}

// FMOD's FileSeek Callback
FMOD_RESULT F_CALLBACK AudioManager::FileSeekCallback(void* handle, unsigned int pos, void* userdata)
{
	if (!handle)
		return FMOD_ERR_INVALID_HANDLE;

	IFileStream* stream = static_cast<IFileStream*>(handle);

	// Check if seeking beyond file size
	if (static_cast<int64_t>(pos) > stream->GetSize())
		return FMOD_ERR_FILE_EOF;

	stream->Seek(static_cast<int64_t>(pos), SeekOrigin::Begin);
	return FMOD_OK;
}
#endif // __ANDROID__
