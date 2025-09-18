/******************************************************************************/
/*!
\file   TweenManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  TweenManager is a singleton class initialised when it is first accessed and
  dies at the end of the program. It is the primary interface for starting
  tweens (interpolating values), whether it be from general objects or ECS
  entities.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TweenManager.h"

TweenManager::TweenManager() : tweens{}
{
}

void TweenManager::Update(float dt)
{
	// Don't need to care about looping if there are no tweens
	if (tweens.empty())
	{
		return;
	}

	for (auto it = tweens.begin(); it != tweens.end();)
	{
		// Remove any inactive tweens
		if (!(*it)->IsActive())
		{
			it = tweens.erase(it);
			continue;
		}

		(*it)->Update(dt);
		++it;
	}
}

void TweenManager::Clean()
{
	CONSOLE_LOG_EXPLICIT("Tween Manager Cleaned!", LogLevel::LEVEL_INFO);
	for (auto it = tweens.begin(); it != tweens.end();)
	{
		it = tweens.erase(it);
	}
}

TweenManager::~TweenManager()
{
	Clean();
}
