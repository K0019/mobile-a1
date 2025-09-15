/******************************************************************************/
/*!
\file   CheatCodes.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/28/2024

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief

	Defininations for CheatCodes function


All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "CheatCodes.h"

Time CheatCodes::start = std::chrono::steady_clock::now();
Time CheatCodes::end = std::chrono::steady_clock::now();


void CheatCodes::ClearCheats()
{
	if (godMode)
	{
		godMode = false;
	}
	if (slowMotion)
	{
		slowMotion = false;
		GameTime::SetTimeScale(1.0f);
	}
	
}

bool CheatCodes::PreRun()
{
	/* 
	 * UP   : 0001
	 * Down : 0010
	 * Left : 0100
	 * Right: 1000
	*/

	end = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

	//Press C to clear cheats
	if (InputOld::GetKeyCurr(KEY::C))
	{
		ClearCheats();
	}

	//Clear all bits if more that 1 sec have passed since pressing
	if (elapsed.count() >= 1)
	{
		inputCmds.reset();
		bitPos = 0;
	}
		
	if (InputOld::GetKeyPressed(KEY::UP) || InputOld::GetKeyPressed(KEY::DOWN) ||
		InputOld::GetKeyPressed(KEY::LEFT) || InputOld::GetKeyPressed(KEY::RIGHT))
	{
		start = std::chrono::steady_clock::now();

		//An extra command is inputted and matches none of the cheats
		if (bitPos >= 16)
		{
			inputCmds.reset();
			bitPos = 0;
		}

		mask.reset();
		//Inputs here
		if (InputOld::GetKeyPressed(KEY::UP))
		{
			mask.set(0 + bitPos);
			bitPos += 4;
		}
		else if (InputOld::GetKeyPressed(KEY::DOWN))
		{
			mask.set(1 + bitPos);
			bitPos += 4;
		}
		else if (InputOld::GetKeyPressed(KEY::LEFT))
		{
			mask.set(2 + bitPos);
			bitPos += 4;
		}
		else if (InputOld::GetKeyPressed(KEY::RIGHT))
		{
			mask.set(3 + bitPos);
			bitPos += 4;
		}
		else
		{
			//Nothing
		}
		inputCmds |= mask;


		//Checks here
		if (inputCmds == Cheats::GodMode_PlayerAndObjective)
		{
			godMode = true;
			inputCmds.reset();
			bitPos = 0;
		}
		if (inputCmds == Cheats::SlowMotion)
		{
			slowMotion = !slowMotion;
			GameTime::SetTimeScale(slowMotion ? 0.1f : 1.0f);

			inputCmds.reset();
			bitPos = 0;
		}
		//else if (inputCmds == Cheats::Cheat2){}

	}


	return false;
}

