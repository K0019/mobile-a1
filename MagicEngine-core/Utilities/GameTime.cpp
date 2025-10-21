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

#include "Utilities/GameTime.h"

float GameTime::targetRealFps{ 60 };
std::chrono::time_point<std::chrono::steady_clock> GameTime::lastFrameTimepoint{};
std::chrono::nanoseconds GameTime::targetFrametime{ std::chrono::nanoseconds{ static_cast<int64_t>(1e9 / targetRealFps) } };
std::chrono::time_point<std::chrono::steady_clock> GameTime::targetFrameTimepoint{ std::chrono::steady_clock::now() + GameTime::targetFrametime };
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

void GameTime::SetTargetFps(float newTargetFps)
{
	targetRealFps = newTargetFps;

	if (targetRealFps > 0.0)
	{
		const double frameTimeNs = 1e9 / targetRealFps;
		targetFrametime = std::chrono::nanoseconds{ static_cast<int64_t>(frameTimeNs) };
	}
	else
		targetFrametime = std::chrono::nanoseconds::zero();
	lastFrameTimepoint = std::chrono::steady_clock::now();
	targetFrameTimepoint = lastFrameTimepoint + targetFrametime;

	// Offset accumulated time so a new tick doesn't happen exactly on a new frame to fix system frame pacing issues
	accumulatedTime = realAccumulatedTime = std::chrono::duration_cast<std::chrono::duration<float>>(targetFrametime).count() * 0.5f;
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

void GameTime::WaitUntilNextFrame()
{
	// Sleep & Spin thread
	Wait();

	// Calculate dt
	auto chronoNow{ std::chrono::steady_clock::now() };
	auto chronoDeltaTime{ std::chrono::duration_cast<std::chrono::duration<float>>(chronoNow - lastFrameTimepoint) };
	realDeltaTime = chronoDeltaTime.count();
	fps = 1.0f / realDeltaTime;
	lastFrameTimepoint = chronoNow;

	deltaTime = realDeltaTime * timeScale;

	// If we're tying fixed delta time to normal delta time, we don't need to calculate accumulated time.
	if (!isUsingFixedDeltaTime)
	{
		fixedDeltaTime = realDeltaTime * timeScale;
		return;
	}

	// Ignore adding accumulated time if this is the first frame.
	if (numFixedFrames < 0)
	{
		realNumFixedFrames = numFixedFrames = 0;
		return;
	}

	// Calculate fixed delta time that is affected by timeScale.
	accumulatedTime += deltaTime;
	numFixedFrames = static_cast<int>(accumulatedTime / fixedDeltaTime);
	accumulatedTime -= numFixedFrames * fixedDeltaTime;

	// Calculate real delta time.
	realAccumulatedTime += realDeltaTime;
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

void GameTime::Wait()
{
	// Spin-wait threshold - sleep for longer waits, spin for shorter ones
	static constexpr std::chrono::nanoseconds SPIN_THRESHOLD{ std::chrono::microseconds{ 500 } };

	// Skip timing if no FPS limit is set (m_targetFrameTime will be zero)
	if (targetFrametime == std::chrono::nanoseconds::zero()) {
		return;
	}

	// Get current time - using steady_clock prevents issues with system time changes
	const auto now{ std::chrono::steady_clock::now() };

	// Calculate when this frame should end based on the perfect frame sequence
	// Instead of measuring from 'now', we measure from the last frame time
	// This prevents error accumulation that would cause FPS drift
	const auto targetTime{ lastFrameTimepoint + targetFrametime };

	// Only wait if we're ahead of schedule
	if (now < targetFrameTimepoint) {
		// Calculate how long we need to wait
		const auto remainingTime = targetFrameTimepoint - now;

		// For longer waits (>500µs by default), use sleep_for first
		// This saves CPU compared to pure spin-waiting
		// We stop sleeping SPIN_THRESHOLD before the target to account for
		// sleep_for's inaccuracy (OS might wake us up a few ms late)
		// Thread keeps running, actively checking time
		// Like watching the clock tick instead of using an alarm
		// This is more CPU-intensive but more accurate than sleep_for

		if (remainingTime > SPIN_THRESHOLD) {
			std::this_thread::sleep_for(remainingTime - SPIN_THRESHOLD);
		}

		// Fine-tune the remaining time with spin-waiting
		// yield() allows other threads to run during the spin-wait
		while (std::chrono::steady_clock::now() < targetFrameTimepoint) {
			std::this_thread::yield();
		}
	}

	// Update our frame time tracking
	// We use targetTime instead of now to maintain perfect frame pacing
	// If we're running behind (now > targetTime), this sets up the next frame
	// to be relative to where this frame SHOULD have been, not where it actually was
	// This helps maintain consistent frame pacing even if some frames take too long
	targetFrameTimepoint += targetFrametime;
}


#pragma region Scripting

/*****************************************************************//*!
\brief
	Gets delta time.
\return
	The delta time of the current frame.
*//******************************************************************/
SCRIPT_CALLABLE float CS_GetDt()
{
	return GameTime::Dt();
}

#pragma endregion // Scripting
