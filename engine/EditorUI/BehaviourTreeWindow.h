#pragma once
#include "pch.h"
#include "GUIAsECS.h"   // <-- Required so WindowBase is known
#include "BehaviourTree.h"
#include "BehaviourTreeImguiHelper.h"
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