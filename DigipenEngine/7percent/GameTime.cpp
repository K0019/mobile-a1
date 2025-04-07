/******************************************************************************/
/*!
\file   GameTime.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements functions defined in GameTime.h interface file
  for getting time during each frame, for the purposes of game systems.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "GameTime.h"

float GameTime::fps{};
bool GameTime::isUsingFixedDeltaTime{ true };
float GameTime::deltaTime{}, GameTime::realDeltaTime{};
float GameTime::fixedDeltaTime{ 0.01666666667f };
float GameTime::accumulatedTime{}, GameTime::realAccumulatedTime{};
int GameTime::numFixedFrames{ -1 }, GameTime::realNumFixedFrames{ -1 };
float GameTime::timeScale{ 1.0f };

float GameTime::Dt()
{
	return deltaTime;
}

float GameTime::FixedDt()
{
	return fixedDeltaTime;
}

int GameTime::NumFixedFrames()
{
	return numFixedFrames;
}

float GameTime::RealDt()
{
	return realDeltaTime;
}

int GameTime::RealNumFixedFrames()
{
	return realNumFixedFrames;
}

bool GameTime::IsFixedDtMode()
{
	return isUsingFixedDeltaTime;
}

float GameTime::Fps()
{
	return fps;
}

void GameTime::SetFps(float newFps)
{
	fps = newFps;
}

void GameTime::SetTargetFixedDt(float newFixedDt)
{
	isUsingFixedDeltaTime = (newFixedDt > 0.0f);
	if (!isUsingFixedDeltaTime)
	{
		realNumFixedFrames = numFixedFrames = 1; // Enable systems that only run on fixed delta time to run each frame.
		return;
	}

	fixedDeltaTime = newFixedDt;
}

void GameTime::NewFrame(float dt)
{
	deltaTime = dt * timeScale;
	realDeltaTime = dt;

	// If we're tying fixed delta time to normal delta time, we don't need to calculate accumulated time.
	if (!isUsingFixedDeltaTime)
	{
		fixedDeltaTime = dt * timeScale;
		return;
	}

	// Ignore adding accumulated time if this is the first frame.
	if (numFixedFrames < 0)
	{
		realNumFixedFrames = numFixedFrames = 0;
		return;
	}

	// Calculate fixed delta time that is affected by timeScale.
	accumulatedTime += dt * timeScale;
	numFixedFrames = static_cast<int>(accumulatedTime / fixedDeltaTime);
	accumulatedTime -= numFixedFrames * fixedDeltaTime;

	// Calculate real delta time.
	realAccumulatedTime += dt;
	realNumFixedFrames = static_cast<int>(realAccumulatedTime / fixedDeltaTime);
	realAccumulatedTime -= realNumFixedFrames * fixedDeltaTime;
}

void GameTime::SetTimeScale(float newScale)
{
	timeScale = newScale;

	// 02/03/2025 Kendrick: This isn't really correct, but it's here to ensure that input polling syncs with fixed dt when time scale changes.
	realAccumulatedTime = accumulatedTime;
}

float GameTime::GetTimeScale()
{
	return timeScale;
}
