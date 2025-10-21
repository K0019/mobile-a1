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
#include "Game/AudioListener.h"
#include "Managers/AudioManager.h"

AudioListenerComponent::AudioListenerComponent()
{
}

AudioListenerSystem::AudioListenerSystem() :
	System_Internal{ &AudioListenerSystem::UpdateAudioListenerComp }
{
}

void AudioListenerSystem::UpdateAudioListenerComp(AudioListenerComponent& comp)
{
	ST<AudioManager>::Get()->UpdateListener(ecs::GetEntity(&comp)->GetTransform().GetWorldPosition());
}
