/******************************************************************************/
/*!
\file   SettingsWindow.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   01/16/2025

\author Ryan Cheong (80%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (20%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file that implements a window that shows various editor
  and program settings.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "GUICollection.h"

/*****************************************************************//*!
\class SettingsWindow
\brief
	Draws the ImGui settings window.
*//******************************************************************/
class SettingsWindow : public gui::Window
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	SettingsWindow();

private:
	/*****************************************************************//*!
	\brief
		Draws the contents of the settings window.
	*//******************************************************************/
	void DrawContents() override;

	//! The available window resolutions.
	static const std::pair<int, int> availableResolutions[];
	//! The available FPS targets.
	static const int availableFPSValues[];
	//! The text of the equivalent FPS targets.
	static const char* const FPSValuesText[];
	//! The available log levels.
	static const char* const logLevelsText[];

	//! Whether the user has selected fullscreen.
	bool selectedFullscreen;
	//! Which resolution has the user selected.
	int selectedResolutionIndex;
	//! Which FPS has the user selected.
	int selectedFPSValueIndex;
	//! Whether any settings have been modified since the last save.
	bool modificationsMade;

};

