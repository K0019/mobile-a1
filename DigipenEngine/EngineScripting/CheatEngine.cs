/******************************************************************************/
/*!
\file   CheatEngine.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/20/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	Contains functions to call cheats for demonstrations of different game mechanics.
    Easy to access and add to in c#.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
    public class CheatEngine
    {
        /*****************************************************************//*!
        \brief
            Spawns the next wave.
        \return
        *//******************************************************************/
        public static void SpawnEnemyWave()
        {
            InternalCalls.SpawnEnemyWave();
        }
    }
}
