/******************************************************************************/
/*!
\file   Console.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the definition of the Console

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Console.h"

#include "NameComponent.h"
#include "EntityUID.h"
#include "CustomViewport.h"
#include "Engine.h"

namespace editor {

	Console::Console()
		: WindowBase{ ICON_FA_TERMINAL"Console", gui::Vec2{ 500, 400 }, gui::FLAG_WINDOW::HAS_MENU_BAR }
		, inputTextBox{ "##" }
		, cmdMap{
			// NO COMMANDS ENABLED WHEN IMGUI IS DISABLED ANYWAY
		#ifdef IMGUI_ENABLED
				{ "help", [](Console& console, const std::vector<std::string>&) -> void {
					console.PrintAvailableCommands();
				}},
				{ "clear", [](Console&, const std::vector<std::string>&) -> void {
					ST<internal::LoggedMessagesBuffer>::Get()->ClearLog();
				}},
				{ "resize", [](Console& console, const std::vector<std::string>& tokens) -> void {
					if (tokens.size() < 3)
					{
						CONSOLE_LOG(LEVEL_INFO) << "Usage: resize <width> <height>";
						return;
					}
					try
					{
						unsigned int width = std::stoi(tokens[1]);
						unsigned int height = std::stoi(tokens[2]);
						ST<CustomViewport>::Get()->Resize(width, height);
					}
					catch (const std::invalid_argument&)
					{
						CONSOLE_LOG(LEVEL_INFO) << "Invalid arguments for resize command";
					}
				}},
				{ "maxfps", [](Console& console, const std::vector<std::string>& tokens) -> void {
					if (tokens.size() < 2)
					{
						CONSOLE_LOG(LEVEL_INFO) << "Usage: maxfps <fps>";
						return;
					}
					try
					{
						double maxFPS{ std::stod(tokens[1]) };
						ST<Engine>::Get()->setFPS(maxFPS);
						CONSOLE_LOG(LEVEL_INFO) << "Maximum FPS set to " << maxFPS;
					}
					catch (const std::invalid_argument&)
					{
						CONSOLE_LOG(LEVEL_INFO) << "Argument fps must be a number";
					}
				}},
		// For getting information about the currently selected entity or an entity with the provided UID
		{ "entityInfo", [](Console& console, const std::vector<std::string>& tokens) -> void {
			ecs::EntityHandle entity{};
			if (tokens.size() > 2)
			{
				if (tokens[1] == "uid")
					entity = EntityUIDLookup::GetEntity(std::stoull(tokens[2]));
				else if (tokens[1] == "addr")
					entity = reinterpret_cast<ecs::EntityHandle>(std::stoull(tokens[2]));
				else if (tokens[1] == "name")
					for (auto nameCompIter{ ecs::GetCompsBegin<NameComponent>() }, endIter{ ecs::GetCompsEnd<NameComponent>() }; nameCompIter != endIter; ++nameCompIter)
						if (nameCompIter->GetName() == tokens[2])
						{
							entity = nameCompIter.GetEntity();
							break;
						}
			}
			else
				entity = ST<Inspector>::Get()->GetSelectedEntity();
			if (!entity)
			{
				CONSOLE_LOG(LEVEL_INFO) << "No entity selected.";
				return;
			}

			Transform* parentTransform{ entity->GetTransform().GetParent() };

			CONSOLE_LOG(LEVEL_INFO) << "Entity: " << entity->GetComp<NameComponent>()->GetName();
			CONSOLE_LOG(LEVEL_INFO) << "-- UID: " << entity->GetComp<EntityUIDComponent>()->GetUID();
			CONSOLE_LOG(LEVEL_INFO) << "-- Address: " << reinterpret_cast<size_t>(entity);
			CONSOLE_LOG(LEVEL_INFO) << "-- Parent: " << (parentTransform ? parentTransform->GetEntity()->GetComp<EntityUIDComponent>()->GetUID() : 0);
		}},
#endif
		}
	{
	}

	namespace
	{
		std::vector<std::string> Tokenize(const std::string& str) {
			std::vector<std::string> tokens;
			std::istringstream stream(str);
			std::string token;
			while (stream >> token)
				tokens.push_back(token);
			return tokens;
		}
	}

#ifdef IMGUI_ENABLED
	void Console::HandleCommand(const char* command_line)
	{
		std::vector tokens{ Tokenize(command_line) };
		if (tokens.empty())
			return;

		const std::string& cmd{ tokens.front() };
		auto cmdFuncIter{ cmdMap.find(cmd) };
		if (cmdFuncIter == cmdMap.end())
		{
			CONSOLE_LOG(LEVEL_INFO) << "Unknown command: '" << cmd << "', type help for a list of commands";
			return;
		}

		cmdFuncIter->second(*this, tokens);
	}

	void Console::PrintAvailableCommands()
	{
		std::stringstream msg{};
		msg << "Available commands : ";

		// Sort commands alphabetically
		auto compFunc{ [](const std::string* a, const std::string* b) -> bool { return *a < *b; } };
		std::set<const std::string*, decltype(compFunc)> sortedCmds{};
		for (const auto& entry : cmdMap)
			sortedCmds.insert(&entry.first);

		// Print the commands
		bool isFirstCmd{ true };
		for (const auto& cmd : sortedCmds)
		{
			if (!isFirstCmd)
				msg << ", ";
			else
				isFirstCmd = false;
			msg << *cmd;
		}

		CONSOLE_LOG(LEVEL_INFO) << msg.str();
	}
#endif

	void Console::DrawWindow()
	{
#ifdef IMGUI_ENABLED
		// Close window if grave is pressed.
		if (gui::IsCurrWindowFocused() && gui::IsKeyPressed(gui::KEY::GRAVE))
		{
			SetIsOpen(false);
			return;
		}

		// Menu Bar
		if (gui::MenuBar menuBar{})
		{
			if (gui::Menu optionsMenu{ "Options" })
			{
				gui::MenuItem("Auto-scroll", &toggleScroll);
				gui::Separator();
				gui::MenuItem("Debug", &showDebug);
				gui::MenuItem("Info", &showInfo);
				gui::MenuItem("Warning", &showWarning);
				gui::MenuItem("Error", &showError);
				gui::MenuItem("Fatal", &showFatal);
			}
			if (gui::Menu actionsMenu{ "Actions" })
			{
				if (gui::MenuItem("Clear")) ST<internal::LoggedMessagesBuffer>::Get()->ClearLog();
				if (gui::MenuItem("Copy")) ImGui::LogToClipboard();
				if (gui::MenuItem("Dump to File")) ST<internal::LoggedMessagesBuffer>::Get()->FlushLogToFile("console_log.txt");
			}
		}

		// Filter
		static gui::TextBoxWithFilter filter;
		filter.Draw("Search", gui::GetWindowWidth() * 0.25f);
		gui::SameLine();
		gui::TextDisabled("(?)");
		if (gui::IsItemHovered())
		{
			gui::Tooltip tooltip{};
			gui::SetTextWrapPos wrapTextPos{ gui::GetFontSize() * 35.0f };
			gui::TextUnformatted("Filter usage:\n"
				"  \"\"         display all lines\n"
				"  \"xxx\"      display lines containing \"xxx\"\n"
				"  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
				"  \"-xxx\"     hide lines containing \"xxx\"");
		}

		gui::Separator();

		// Log window
		if (gui::Child scrollingRegion{ "ScrollingRegion", gui::Vec2(0, -gui::GetFrameHeightWithSpacing()), gui::FLAG_CHILD::NONE, gui::FLAG_WINDOW::HORIZONTAL_SCROLL_BAR })
		{
			gui::SetStyleVar styleVar{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{4, 1} }; // Tighten spacing
			gui::SetTextWrapPos textWrapPos{ gui::GetWindowContentRegionMax().x };

			for (const auto& entry : ST<internal::LoggedMessagesBuffer>::Get()->GetLogs())
			{
				if (!filter.PassFilter(entry.message.c_str()))
					continue;
				bool showEntry = false;
				switch (entry.level)
				{
				case LEVEL_DEBUG:   showEntry = showDebug; break;
				case LEVEL_INFO:    showEntry = showInfo; break;
				case LEVEL_WARNING: showEntry = showWarning; break;
				case LEVEL_ERROR:   showEntry = showError; break;
				case LEVEL_FATAL:   showEntry = showFatal; break;
				}
				if (!showEntry)
					continue;

				switch (entry.level)
				{
				case LEVEL_DEBUG:   gui::TextColored(gui::Vec4{ 0.5f, 0.5f, 0.5f, 1.0f }, "[DEBUG] %s", entry.message.c_str()); break;
				case LEVEL_INFO:    gui::TextColored(gui::Vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, "[INFO] %s", entry.message.c_str()); break;
				case LEVEL_WARNING: gui::TextColored(gui::Vec4{ 1.0f, 1.0f, 0.0f, 1.0f }, "[WARNING] %s", entry.message.c_str()); break;
				case LEVEL_ERROR:   gui::TextColored(gui::Vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "[ERROR] %s", entry.message.c_str()); break;
				case LEVEL_FATAL:   gui::TextColored(gui::Vec4{ 0.5f, 0.0f, 0.0f, 1.0f }, "[FATAL] %s", entry.message.c_str()); break;
				}
			}

			if (scrollToBottom && toggleScroll)
			{
				gui::SetScrollHereY(1.0f);
				scrollToBottom = false;
			}
		}

		gui::Separator();

		// Command-line
		bool reclaimFocus{ stealFocus };

		gui::FLAG_INPUT_TEXT inputTextFlags{
			gui::FLAG_INPUT_TEXT::ENTER_RETURNS_TRUE |
			gui::FLAG_INPUT_TEXT::CALLBACK_COMPLETION |
			gui::FLAG_INPUT_TEXT::CALLBACK_HISTORY |
			gui::FLAG_INPUT_TEXT::CALLBACK_CHAR_FILTER
		};
		if (inputTextBox.Draw(inputTextFlags, InputTextFilter)) {
			HandleCommand(inputTextBox.GetBufferPtr());
			inputTextBox.ClearBuffer();
			reclaimFocus = true;// Ensure input field is focused after command
		}

		gui::SetItemDefaultFocus();
		if (reclaimFocus)
		{
			gui::SetKeyboardFocusHere(-1);
			stealFocus = false;
		}
#endif
	}

	void Console::UserOnOpenStateChanged()
	{
		// When the console is opened we should grab the keyboard focus so the user doesn't need to click on the input field.
		if (GetIsOpen())
			stealFocus = true;
	}

#ifdef IMGUI_ENABLED
	int Console::InputTextFilter(gui::types::InputTextCallbackData* data) {
		// Implement command history and auto-completion here

		// Ignore certain characters
		switch (data->EventChar)
		{
		case '`': // This opens/closes the console. We should not need this char to be input.
			return 1;
		default:
			return 0;
		}
	}
#endif

}

#pragma region Scripting

SCRIPT_CALLABLE void CS_Log(const char* message, int level)
{
	CONSOLE_LOG(static_cast<LogLevel>(level)) << message;
}

#pragma endregion Scripting
