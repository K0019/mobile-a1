/******************************************************************************/
/*!
\file   HotReloader.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/21/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e
\brief
  This file contains the definitions of the class Hot Reloader, which reloads
  the and updates the scripts inside the User Assembly .dll inside the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "HotReloader.h"
#include "GameSettings.h"
#include "CSScripting.h"
#include "game.h"

void HotReloader::FocusCallBackReload(GLFWwindow*, int focused)
{
	if (focused && ST<Game>::Get()->GetState() == GAMESTATE::EDITOR)
	{
#ifdef IMGUI_ENABLED
		CSharpScripts::CSScripting::ReloadAssembly();
#endif
	}
	else
	{
		// incase anything you want to add
	}
}
