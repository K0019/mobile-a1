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

bool AudioSystem::PreRun()
{
	ST<AudioManager>::Get()->UpdateSystem();
	return true;
}
