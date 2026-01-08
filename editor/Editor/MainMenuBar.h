/******************************************************************************/
/*!
\file   MainMenuBar.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  ImGui popup that can display a variety of text to screen.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/ECS.h"

namespace editor {

	/*****************************************************************//*!
	\class MainMenuBar
	\brief
		Draws the main menu bar at the top of the editor.
		Making this an ecs system so it can process itself.
	*//******************************************************************/
	class MainMenuBar : public ecs::System<MainMenuBar>
	{
	public:
		/*****************************************************************//*!
		\brief
			Draws the main menu bar.
		*//******************************************************************/
		bool PreRun() override;

	};

}
