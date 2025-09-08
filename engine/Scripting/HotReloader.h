/******************************************************************************/
/*!
\file   HotReloader.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/21/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e
\brief
  This file contains the declaration of the class Hot Reloader, which reloads
  the and updates the scripts inside the User Assembly .dll inside the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

struct GLFWwindow;

class HotReloader
{
public:
	/*****************************************************************//*!
	\brief
		Callback function for GLFW to use when the engine regains focus.
		Reloads user scripts upon regaining focus.
	\param[in] window
		Pointer of the window to check the focus of.
	\param[in] focused
		To check if the window is currently in focus.

	*//******************************************************************/
	static void FocusCallBackReload(GLFWwindow* window, int focused);
private:

};

