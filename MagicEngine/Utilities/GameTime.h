/******************************************************************************/
/*!
\file   GameTime.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the interface file for getting time during each frame, for the purposes of
  game systems.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\class GameTime
\brief
	Encapsulates functions that access game time.
*//******************************************************************/
class GameTime
{
public:
	/*****************************************************************//*!
	\brief
		The delta time of this frame. Affected by timescale.
	\return
		The delta time of this frame.
	*//******************************************************************/
	static float Dt();

	/*****************************************************************//*!
	\brief
		The fixed delta time of the application.
	\return
		The fixed delta time of the application.
	*//******************************************************************/
	static float FixedDt();

	/*****************************************************************//*!
	\brief
		The number of fixed frames passed since the last frame. Affected by timescale.
	\return
		The number of fixed frames passed since the last frame.
	*//******************************************************************/
	static int NumFixedFrames();

	/*****************************************************************//*!
	\brief
		The real delta time of this frame.
	\return
		The real delta time of this frame.
	*//******************************************************************/
	static float RealDt();

	/*****************************************************************//*!
	\brief
		The number of fixed frames passed since the last frame.
	\return
		The number of fixed frames passed since the last frame.
	*//******************************************************************/
	static int RealNumFixedFrames();

	/*****************************************************************//*!
	\brief
		Gets the number of seconds elapsed since the start of the first frame.
	\return
		The number of seconds elapsed since the start of the first frame.
	*//******************************************************************/
	static float TimeSinceStart();

	/*****************************************************************//*!
	\brief
		Checks whether fixed dt is actually fixed, or if we're in variable
		fixed dt mode.
	\return
		True if fixed dt mode. False if variable fixed dt mode.
	*//******************************************************************/
	static bool IsFixedDtMode();

	/*****************************************************************//*!
	\brief
		The instantaneous fps at this current frame.
	\return
		The instantaneous fps at this current frame.
	*//******************************************************************/
	static float Fps();

	/*****************************************************************//*!
	\brief
		Set the target FPS to run the program at.
	\param newTargetFps
		The new target FPS.
	*//******************************************************************/
	static void SetTargetFps(float newTargetFps);

	/*****************************************************************//*!
	\brief
		Set the fixed delta time of this application.
	\param newFixedDt
		The target fixed delta time of this application.
	*//******************************************************************/
	static void SetTargetFixedDt(float newFixedDt);

	/*****************************************************************//*!
	\brief
		Informs this class that a new frame is starting.
		This also calculates the number of fixed frames since the last update.
	\param dt
		The time since the last frame.
	*//******************************************************************/
	static void WaitUntilNextFrame();

	/*****************************************************************//*!
	\brief
		Set time scale.
	\param newScale
		The new value to use.
	*//******************************************************************/
	static void SetTimeScale(float newScale);

	/*****************************************************************//*!
	\brief
		Get time scale.
	\return
		Floating point value.
	*//******************************************************************/
	static float GetTimeScale();

private:
	static void Wait();

private:
	GameTime() = delete;

	//! Track time since first frame
	static std::chrono::time_point<std::chrono::steady_clock> firstFrameTimepoint;

	//! The target realtime fps to run at
	static float targetRealFps;
	static std::chrono::time_point<std::chrono::steady_clock> lastFrameTimepoint;
	static std::chrono::nanoseconds targetFrametime;
	static std::chrono::time_point<std::chrono::steady_clock> targetFrameTimepoint;

	//! The instantaneous fps at this current frame
	static float fps;

	//! The delta time since the previous frame. This can be scaled by time scale.
	static float deltaTime;
	//! The real delta time since the previous frame, unaffected by time scale.
	static float realDeltaTime;

	//! Whether fixed delta time is decoupled from delta time
	static bool isUsingFixedDeltaTime;
	//! The fixed delta time.
	static float fixedDeltaTime;

	//! The accumulated time since the last fixed frame
	static float accumulatedTime;
	//! The number of fixed frames since the last update
	static int numFixedFrames;

	//! The real accumulated time since the last fixed frame.
	static float realAccumulatedTime;
	//! The number of real fixed frames since the last update.
	static int realNumFixedFrames;

	//! Multiplier for time delta
	static float timeScale;
};