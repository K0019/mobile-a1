/******************************************************************************/
/*!
\file   AudioListener.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  AudioListener is an ECS component-system pair that identifies an entity within
  a scene as the primary audio listener for spatial audio effects.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "AudioListener.h"
#include "AudioManager.h"

AudioListenerComponent::AudioListenerComponent()
	: minDistance{ 10.0f }
	, maxDistance{ 1000.0f }
	, dopperScale{ 1.0f }
	, distanceFactor{ 0.01f }
	, rolloffScale{ 0.1f }
{
}

void AudioListenerComponent::OnStart()
{
	ST<AudioManager>::Get()->UpdateSpatialProperties(minDistance, maxDistance, dopperScale, distanceFactor, rolloffScale);
}

void AudioListenerComponent::EditorDraw()
{
	gui::VarDrag("Minimum Distance", &minDistance);
	gui::VarDrag("Maximum Distance", &maxDistance);
	gui::VarDrag("Doppler Scale", &dopperScale);
	gui::VarDrag("Distance Factor", &distanceFactor);
	gui::VarDrag("Rolloff Scale", &rolloffScale);
}

AudioListenerSystem::AudioListenerSystem() :
	System_Internal{ &AudioListenerSystem::UpdateAudioListenerComp }
{
}

void AudioListenerSystem::UpdateAudioListenerComp(AudioListenerComponent& comp)
{
	ST<AudioManager>::Get()->UpdateListenerAttributes(ecs::GetEntity(&comp)->GetTransform().GetWorldPosition());
}
