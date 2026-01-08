/******************************************************************************/
/*!
\file   CrashHandler.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/23/2025

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
	Saves fatal exceptions to a crash log.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

class CrashHandler
{
private:
	CrashHandler() = default;
public:
	static void SetupCrashHandler();
};
