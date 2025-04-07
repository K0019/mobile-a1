/******************************************************************************/
/*!
\file   GameManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	GameManager is a singleton controlling game logic and keeping track of
	parameters such as whether the game is paused, current game state, etc.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Runtime.CompilerServices;

namespace EngineScripting
{
    public static class GameManager
    {
		/*****************************************************************//*!
		\brief
			Get whether the game is paused.
		\return
			True for paused, false for unpaused.
		*//******************************************************************/
		public static bool GetPaused()
        {
            return InternalCalls.GetStatusPause();
        }

		/*****************************************************************//*!
	    \brief
		    Get whether the player's jump is enhanced.
	    \return
		    True for enhanced, false for not.
	    *//******************************************************************/
		public static bool GetPlayerJumpEnhanced()
        {
            return InternalCalls.GetPlayerJumpEnhanced();
        }
    }

}
