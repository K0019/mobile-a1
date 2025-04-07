/******************************************************************************/
/*!
\file   Time.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   12/21/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	This contains the Time class that provides functions related to delta time
	and frame time.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

namespace EngineScripting
{
    public class Time
    {
        public static float DeltaTime => InternalCalls.GetDeltaTime();
    }
}