/******************************************************************************/
/*!
\file   Input.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/15/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	This contains the Input class and its functions such as GetKeyPressed that
    uses the internal call of the same name 

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
	public enum KeyCode
	{
		A = 65,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,

		NUM_0 = 48,
		NUM_1,
		NUM_2,
		NUM_3,
		NUM_4,
		NUM_5,
		NUM_6,
		NUM_7,
		NUM_8,
		NUM_9,

		SPACE = 32,
		ESC = 256,
		ENTER,
		GRAVE = 96,

		RIGHT = 262,
		LEFT,
		DOWN,
		UP,

		F1 = 290,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,

		M_LEFT = 0,
		M_RIGHT,
		M_MIDDLE

	}
	public class Input
	{
		/*****************************************************************//*!
		\brief
			Checks if a specific key has been pressed.
		\param[in] key
			Enum of the key press being checked
        \return
            If the specified key was pressed.
		*//******************************************************************/
		public static bool GetKeyPressed(KeyCode key)
		{
			return InternalCalls.GetKeyPressed((int)key);
		}

		/*****************************************************************//*!
		\brief
			Checks if a specific key has been released.
		\param[in] key
			Enum of the key release being checked
        \return
            If the specified key was released.
		*//******************************************************************/
		public static bool GetKeyReleased(KeyCode key)
		{
			return InternalCalls.GetKeyReleased((int)key);
		}

		/*****************************************************************//*!
		\brief
			Checks if a specific key is being pressed.
		\param[in] key
			Enum of the key press being checked
        \return
            If the specified key is being pressed.
		*//******************************************************************/
		public static bool GetCurrKey(KeyCode key)
		{
			return InternalCalls.GetCurrKey((int)key);
		}

		/*****************************************************************//*!
		\brief
			Gets the world position of the mouse.
		\return
			Vector 2 of the Mouse world position.
		*//******************************************************************/
		public static Vector2 GetMousePos()
		{
			InternalCalls.GetMouseWorldPos(out Vector2 Result);
			return Result;
		}
    }
}
