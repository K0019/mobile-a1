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
#include "Scripting/HotReloader.h"
#include "GameSettings.h"
#include "Game/GameSystems.h"

void HotReloader::Init()
{
	Messaging::Subscribe("OnWindowFocus", OnFocusChanged);
}

HotReloader::~HotReloader()
{
	Messaging::Unsubscribe("OnWindowFocus", OnFocusChanged);
}

void HotReloader::OnFocusChanged([[maybe_unused]] bool isFocused)
{
#ifdef IMGUI_ENABLED
	if (isFocused && ST<GameSystemsManager>::Get()->GetState() == GAMESTATE::EDITOR)
	{
		//CSharpScripts::CSScripting::ReloadAssembly();
	}
#endif
}
