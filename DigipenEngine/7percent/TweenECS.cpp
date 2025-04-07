/******************************************************************************/
/*!
\file   TweenECS.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  Non-template function defines for TweenECS.h.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TweenECS.h"

TweenComponent::TweenComponent() : tweens{}
{
}

TweenComponent::TweenComponent(TweenComponent const& other) noexcept
	: tweens(other.tweens)
{
}

void TweenComponent::Update(float dt)
{
	// Don't need to care about looping if there are no tweens
	if (tweens.empty())
	{
		return;
	}

	for (auto it = tweens.begin(); it != tweens.end();)
	{
		(*it)->Update(dt);

		// Remove any inactive tweens
		if (!(*it)->IsActive())
		{
			delete (*it);
			it = tweens.erase(it);
			continue;
		}

		++it;
	}
}

void TweenComponent::Clean()
{
	// CONSOLE_LOG_EXPLICIT("Tween Component Cleaned!", LogLevel::LEVEL_INFO);
	for (auto it = tweens.begin(); it != tweens.end(); ++it)
	{
		delete (*it);
	}
	tweens.clear();
}

size_t TweenComponent::GetNumberOfTweens()
{
	return tweens.size();
}

TweenComponent::TweenComponent(TweenComponent&& other) noexcept
	: tweens(std::move(other.tweens))
{
	// CONSOLE_LOG_EXPLICIT("Tween Component Moved!", LogLevel::LEVEL_INFO);
}

TweenComponent::~TweenComponent()
{
	Clean();
}

TweenSystem::TweenSystem() : System_Internal{ &TweenSystem::Update }
{
}

void TweenSystem::Update(TweenComponent& tweenComp)
{
	tweenComp.Update(GameTime::FixedDt());
}
