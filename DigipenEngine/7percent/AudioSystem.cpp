/******************************************************************************/
/*!
\file   AudioSystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	AudioSystem is an ECS system which exists solely to do continuous update on
	FMOD::System within AudioManager. Whether or not this helps audio playback
	quality is yet to be rigorously tested, although it is said to be good practice.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "AudioSystem.h"
#include "AudioManager.h"

AudioSourceComponent::AudioSourceComponent() 
	: volume{ 1.f }
{
}

void AudioSourceComponent::OnStart()
{
	//ST<AudioManager>::Get()->UpdateSpatialProperties(minDistance, maxDistance, dopperScale, distanceFactor, rolloffScale);
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
