/******************************************************************************/
/*!
\file   BehaviourTreeFactory.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
	This header file declares the object that creates the imgui window for the
	behavior tree editing.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Editor/Containers/GUIAsECS.h"   // <-- Required so WindowBase is known

namespace editor {

	/*****************************************************************//*!
	\class SettingsWindow
	\brief
		Draws the ImGui settings window.
	*//******************************************************************/
	class BehaviourTreeWindow : public WindowBase<BehaviourTreeWindow>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		BehaviourTreeWindow();

	private:
		/*****************************************************************//*!
		\brief
			Draws the contents of the settings window.
		*//******************************************************************/
		void DrawWindow() override;
		bool modificationsMade;
	};

}