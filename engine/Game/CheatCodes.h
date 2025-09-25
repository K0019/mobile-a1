/* TO BE REFACTORED - This implementation sucks to be frank. */


/******************************************************************************/
/*!
\file   CheatCodes.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/28/2024

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief

	Header file for functions to set up and execute cheats 


All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "GraphicsECSSprite.h"
#include "TextSystem.h"

using Time = std::chrono::time_point<std::chrono::steady_clock>;

enum Cheats {

	/*
	 * UP   : 0001
	 * Down : 0010
	 * Left : 0100
	 * Right: 1000
	 * 
	 * HOW TO ADD CHEATS :)
	 * 
	 * Cheats can have only 4 inputs,
	 * 
	 * If I want the input to be UP, RIGHT, DOWN, LEFT
	 * 
	 * I have to write the input with the 1st command being on the far right
	 * 
	 * E.G
	 *  
	 *  UP,
	 *	............0001
	 * 
	 *  UP, RIGHT,
	 *  ........1000-0001
	 *  
	 *  UP, RIGHT, DOWN,
	 *  ....0010-1000-0001
	 * 
	 *	UP, RIGHT, DOWN, LEFT
	 *  0100-0010-1000-0001
	*/

	//Input in Binary starts from right
	GodMode_PlayerAndObjective = 0b0001000100010001, //Up, Up, Up, Up
	SlowMotion = 0b0010001000100010, //Down, Down, Down, Down
	LaserRifle = 0b1000100010001000 //Down, Down, Down, Down
	

};

class CheatCodes : public ecs::System<CheatCodes>
{
public:

	/*****************************************************************//*!
	\brief This funtion runs first before the actual system runs

	*//******************************************************************/
	bool PreRun() override;


private:
	std::bitset<16> inputCmds{0b0000000000000000}; // 0b for binary literal
	static Time start;
	static Time end;
	int bitPos{};
	std::bitset<16> mask;
	bool godMode = false;
	bool slowMotion = false;

	/*****************************************************************//*!
	\brief
		Clears ALL cheats currently active
	\return
		void
	*//******************************************************************/
	void ClearCheats();

};
