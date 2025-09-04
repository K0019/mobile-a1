/******************************************************************************/
/*!
\file   ScriptGlue.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	This contains the Declaration of the class ScriptGlue

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

namespace CSharpScripts
{

	class ScriptGlue
	{
	public:
		/*****************************************************************//*!
		\brief
			Static function to link the internal call functions in c++ to 
			the c# side.
		\return
			None
		*//******************************************************************/
		static void RegisterFunctions();
	};

}

