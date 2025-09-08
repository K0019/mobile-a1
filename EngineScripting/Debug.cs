/******************************************************************************/
/*!
\file   Debug.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/15/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	Contains Functions mainly used for console logging

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
	public enum LogLevel : int
	{
		DEBUG = 0,
		INFO,
		WARNING,
		ERROR
	}

	public class Debug
	{
		[DllImport("__Internal", EntryPoint = "CS_Log")]
		public static extern void Log(string message, LogLevel level);

		public static void Log(string message)
		{
			Log(message, LogLevel.INFO);
		}
	}
}
