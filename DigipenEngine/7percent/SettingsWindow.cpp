/******************************************************************************/
/*!
\file   SettingsWindow.cpp
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
  This is a source file that implements a window that shows various editor
  and program settings.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "SettingsWindow.h"
#include "GameSettings.h"
#include "LayersMatrix.h"
#include "Editor.h"
#include "AssetBrowser.h"

const std::pair<int, int> SettingsWindow::availableResolutions[]{
	{ 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 }
};
const int SettingsWindow::availableFPSValues[]{
	0, 30, 60, 120, 144, 240, 300
};
const char* const SettingsWindow::FPSValuesText[]{
	"Unlimited", "30 FPS", "60 FPS", "120 FPS", "144 FPS", "240 FPS", "300 FPS"
};
const char* const SettingsWindow::logLevelsText[]{
	"Debug", "Info", "Warning", "Error", "FATAL"
};

SettingsWindow::SettingsWindow()
	: gui::Window{ ICON_FA_GEAR" Settings", gui::Vec2{ 540.0f, 768.0f } }
	, selectedFullscreen{ ST<GameSettings>::Get()->m_fullscreenMode != 0 }
	, selectedResolutionIndex{ static_cast<int>(std::find_if(std::begin(availableResolutions), std::end(availableResolutions),
		[settings = ST<GameSettings>::Get()](const auto& resolution) -> bool {
			return resolution.first == settings->m_resolutionX;
		}) - std::begin(availableResolutions)) }
	, selectedFPSValueIndex{ static_cast<int>(std::find(std::begin(availableFPSValues), std::end(availableFPSValues), ST<GameSettings>::Get()->m_maxFPS) - std::begin(availableFPSValues)) }
	, modificationsMade{ false }
{
}

void SettingsWindow::DrawContents()
{
	gui::TextUnformatted("Graphics");
	gui::Separator();

	auto* settings = ST<GameSettings>::Get();

	// Fullscreen Checkbox
	gui::TextUnformatted("Display Mode");
	gui::Checkbox("Fullscreen", &selectedFullscreen);
	if (!selectedFullscreen)
	{
		gui::TextUnformatted("Resolution");
		gui::SameLine();
		gui::TextDisabled("(?)");
		if (gui::IsItemHovered())
		{
			gui::Tooltip tooltip{};
			gui::TextFormatted("Current: %dx%d", settings->m_resolutionX, settings->m_resolutionY);
		}

		std::vector<std::string> resolutionStrings{};
		std::transform(std::begin(availableResolutions), std::end(availableResolutions), std::back_inserter(resolutionStrings),
			[](const auto& resolution) -> std::string { return std::format("{}x{}", resolution.first, resolution.second); }
		);

		std::vector<const char*> resolutionItems{};
		std::transform(resolutionStrings.begin(), resolutionStrings.end(), std::back_inserter(resolutionItems),
			[](const auto& str) -> const char* { return str.c_str(); }
		);

		gui::Combo resolutionCombo{ "##Resolution", resolutionItems, &selectedResolutionIndex };
	}

	gui::TextUnformatted("FPS Limit");
	gui::SameLine();
	gui::TextDisabled("(?)");
	if (gui::IsItemHovered())
	{
		gui::Tooltip tooltip{};
		gui::TextFormatted("Current: %s", settings->m_maxFPS > 0 ? std::to_string(settings->m_maxFPS).c_str() : "Unlimited");
	}

	gui::Combo fpsLimitCombo{ "FPSLimit", FPSValuesText, std::size(FPSValuesText), &selectedFPSValueIndex};

	// Apply Button
	modificationsMade = (static_cast<int>(selectedFullscreen) != settings->m_fullscreenMode) ||
						(!selectedFullscreen && availableResolutions[selectedResolutionIndex].first != settings->m_resolutionX) ||
						(availableFPSValues[selectedFPSValueIndex] != settings->m_maxFPS);

	gui::Separator();
	if (modificationsMade)
	{
		gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.2f, 0.7f, 0.2f, 1.0f } };
		gui::SetStyleColor buttonHoveredColor{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 0.3f, 0.8f, 0.3f, 1.0f } };
		gui::SetStyleColor buttonActiveColor{ gui::FLAG_STYLE_COLOR::BUTTON_ACTIVE, gui::Vec4{ 0.2f, 0.6f, 0.2f, 1.0f } };

		if (gui::Button{ "Apply Graphics Settings" })
		{
			settings->m_fullscreenMode = static_cast<int>(selectedFullscreen);
			settings->m_maxFPS = availableFPSValues[selectedFPSValueIndex];

			if (!selectedFullscreen)
			{
				settings->m_resolutionX = availableResolutions[selectedResolutionIndex].first;
				settings->m_resolutionY = availableResolutions[selectedResolutionIndex].second;
			}

			settings->Save();
			settings->Apply();
		}

		modificationsMade = false;
	}
	///Debugging
	gui::TextUnformatted("Debugging");
	gui::Separator();
	gui::Combo consoleLogLevelCombo{ "Console Log Level", logLevelsText, std::size(logLevelsText), &settings->m_logLevel };
	gui::Separator();

	// Layers
	if (gui::Button{ "Layers", gui::Vec2{ -0.1f, 30.0f } })
		ST<LayersMatrix>::Get()->SetIsOpen(true);

#ifdef IMGUI_ENABLED
	ST<Editor>::Get()->DrawSceneView();
	ST<AssetBrowser>::Get()->DrawConfig();
#endif
}
