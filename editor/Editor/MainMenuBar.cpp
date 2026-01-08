/******************************************************************************/
/*!
\file   MainMenuBar.cpp
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
#include "Editor/MainMenuBar.h"
#include "Editor/Containers/GUIAsECS.h"
#include "Engine/SceneManagement.h"

#include "Editor/SettingsWindow.h"
#include "Editor/Console.h"
#include "Editor/Performance.h"
#include "Editor/Inspector.h"
#include "Editor/AssetBrowser.h"
#include "Editor/Hierarchy.h"
#include "Editor/BehaviourTreeWindow.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"

namespace editor {

	bool MainMenuBar::PreRun()
	{
#ifdef IMGUI_ENABLED
		if (gui::MainMenuBar mainMenuBar{})
		{
			// Add a "File" menu
			if (gui::Menu fileMenu{ "File" })
			{
				if (gui::MenuItem("Save"))
				{
					ST<SceneManager>::Get()->SaveAllScenes();
					ST<SceneManager>::Get()->SaveWhichScenesOpened();
				}
				if (gui::MenuItem("Settings"))
					editor::CreateGuiWindow<editor::SettingsWindow>();
				if (gui::MenuItem("Exit"))
					ST<GraphicsMain>::Get()->SetPendingShutdown();
			}

			if (gui::Menu toolsMenu{ "Tools" })
			{
				if (gui::MenuItem("Console"))
				{
					editor::CreateGuiWindow<editor::Console>();
					//ImGui::SetWindowFocus(ICON_FA_TERMINAL"Console"); // Save the name of the windows somewhere else so i dont have to copy paste = ryan cheong
				}
				if (gui::MenuItem("Performance"))
				{
					editor::CreateGuiWindow<editor::PerformanceWindow>();
					//ImGui::SetWindowFocus(ICON_FA_GAUGE_HIGH" Performance");
				}
				if (gui::MenuItem("Inspector"))
				{
					editor::CreateGuiWindow<editor::Inspector>();
					//ImGui::SetWindowFocus(ICON_FA_MAGNIFYING_GLASS" Inspector");
				}
				if (gui::MenuItem("Browser"))
				{
					editor::CreateGuiWindow<editor::AssetBrowser>();
					//ImGui::SetWindowFocus(ICON_FA_FOLDER" Browser");
				}
				if (gui::MenuItem("Hierarchy"))
				{
					editor::CreateGuiWindow<editor::Hierarchy>();
					//ImGui::SetWindowFocus(ICON_FA_SITEMAP" Hierarchy");
				}
				if (gui::MenuItem("Behaviour Tree"))
					editor::CreateGuiWindow<editor::BehaviourTreeWindow>();
			}
		}
#endif

		return false; // No components to run
	}

}
