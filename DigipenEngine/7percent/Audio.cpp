#include "Audio.h"
#include "AudioManager.h"

AudioSourceComponent::AudioSourceComponent() 
{
}

void AudioSourceComponent::OnStart()
{
	//ST<AudioManager>::Get()->UpdateSpatialProperties(minDistance, maxDistance, dopperScale, distanceFactor, rolloffScale);
}

void AudioSourceComponent::Play(AudioType a, std::string name)
{
	ST<AudioManager>::Get()->PlaySound(channel, name, false, a);
}

void AudioSourceComponent::EditorDraw()
{
	//gui::VarDrag("Volume", &volume, 0.f, 1.f);
	//gui::VarDrag("Minimum Distance", &minDistance);
	//gui::VarDrag("Maximum Distance", &maxDistance);
	//gui::VarDrag("Doppler Scale", &dopperScale);
	//gui::VarDrag("Distance Factor", &distanceFactor);
	//gui::VarDrag("Rolloff Scale", &rolloffScale);
}

bool AudioSystem::PreRun()
{
	ST<AudioManager>::Get()->Update();
	return true;
}

void AudioSystem::UpdateComp(AudioSourceComponent& comp)
{
	//ST<AudioManager>::Get()->UpdateListenerAttributes(ecs::GetEntity(&comp)->GetTransform().GetWorldPosition());
}
