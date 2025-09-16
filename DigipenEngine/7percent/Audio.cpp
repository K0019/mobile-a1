#include "Audio.h"
#include "AudioManager.h"

AudioSourceComponent::AudioSourceComponent() 
	: minDistance{ 20.0f }
	, maxDistance{ 50.0f }
	, dopperScale{ 1.0f }
	, distanceFactor{ 0.01f }
	, rolloffScale{ 0.1f }
	, channelHandle{}
{
}

void AudioSourceComponent::Play(AudioType a, const std::string& name)
{
	channelHandle = ST<AudioManager>::Get()->PlaySound(name, false, a);
}

void AudioSourceComponent::EditorDraw()
{
	gui::VarDrag("Minimum Distance", &minDistance);
	gui::VarDrag("Maximum Distance", &maxDistance);
	gui::VarDrag("Doppler Scale", &dopperScale);
	gui::VarDrag("Distance Factor", &distanceFactor);
	gui::VarDrag("Rolloff Scale", &rolloffScale);
}

bool AudioSystem::PreRun()
{
	ST<AudioManager>::Get()->Update();
	return true;
}
