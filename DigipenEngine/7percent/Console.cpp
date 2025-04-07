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

#include "AnimatorComponent.h"
#include "NameComponent.h"
#include "EntityUID.h"
#include "RenderComponent.h"
#include "CustomViewport.h"
#include "Engine.h"
#include "ryan-c/Renderer.h"
#include "AudioManager.h"

Console::Console()
	: gui::Window{ ICON_FA_TERMINAL"Console", gui::Vec2{ 500, 400 }, gui::FLAG_WINDOW::HAS_MENU_BAR }
	, inputTextBox{ "##" }
	, cmdMap{
	// NO COMMANDS ENABLED WHEN IMGUI IS DISABLED ANYWAY
#ifdef IMGUI_ENABLED
		{ "help", [](Console& console, const std::vector<std::string>&) -> void {
			console.PrintAvailableCommands();
		}},
		{ "clear", [](Console& console, const std::vector<std::string>&) -> void {
			console.ClearLog();
		}},
		{ "resize", [](Console& console, const std::vector<std::string>& tokens) -> void {
			if (tokens.size() < 3)
			{
				console.AddLog("Usage: resize <width> <height>");
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
				console.AddLog("Invalid arguments for resize command");
			}
		}},
		{ "maxfps", [](Console& console, const std::vector<std::string>& tokens) -> void {
			if (tokens.size() < 2)
			{
				console.AddLog("Usage: maxfps <fps>");
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
				console.AddLog("Argument fps must be a number");
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
				entity = ST<Editor>::Get()->GetSelectedEntity();
			if (!entity)
			{
				console.AddLog("No entity selected.");
				return;
			}

			Transform* parentTransform{ entity->GetTransform().GetParent() };

			CONSOLE_LOG(LEVEL_INFO) << "Entity: " << entity->GetComp<NameComponent>()->GetName();
			CONSOLE_LOG(LEVEL_INFO) << "-- UID: " << entity->GetComp<EntityUIDComponent>()->GetUID();
			CONSOLE_LOG(LEVEL_INFO) << "-- Address: " << reinterpret_cast<size_t>(entity);
			CONSOLE_LOG(LEVEL_INFO) << "-- Parent: " << (parentTransform ? parentTransform->GetEntity()->GetComp<EntityUIDComponent>()->GetUID() : 0);
		}},
		// For getting information about the currently playing soudnds
		{ "currentlyPlayingSound", [](Console&, const std::vector<std::string>&) -> void {
			
			auto channels = ST<AudioManager>::Get()->GetChannels();

			for (auto i = channels.begin(); i != channels.end(); ++i)
			{
				CONSOLE_LOG(LEVEL_INFO) << i->first;
			}
		}},
#endif
	}
{
}

Console::Logger::~Logger() {
	Flush();
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

void Console::Logger::Flush() {
	if (console && !messageBuffer.str().empty())
	{
		console->Log(messageBuffer.str(), logLevel);
		messageBuffer.clear();
	}
}

void Console::ClearLog()
{
	log.clear();
	log_count = 0;
}

void Console::AddLog(std::string_view message, LogLevel level)
{
	scrollToBottom = true;
	log.push_back({ std::string(message), level });
#ifdef _DEBUG
	std::cout << CreateFormattedMessage(level, message) << '\n';
#endif
	if (log_count < MAX_LOG_ENTRIES) {
		log_count++;
	}
	else
	{
		FlushLogToFile("console_log.txt");
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
		AddLog("Unknown command: '" + cmd + "', type help for a list of commands");
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

	AddLog(msg.str());
}
#endif

void Console::SetLogLevel(LogLevel level)
{
	logLevel = level;
}

void Console::Log(std::string_view message, LogLevel level)
{
	if (level >= logLevel)
	{
		AddLog(message, level);  // AddLog will handle converting string_view to string
	}
}

void Console::FlushLogToFile(const std::string& filename)
{
	std::ofstream file(filename, std::ios::app);
	log_count = 0;
	if (!file.is_open())
	{
		Log(LEVEL_ERROR) << "Failed to open log file: " << filename;
		return;
	}
	for (const auto& entry : log)
	{
		file << CreateFormattedMessage(entry.level, entry.message) << std::endl;
	}
	file.close();
	ClearLog();
	Log(LEVEL_WARNING) << "Log at Maximum Capacity, flushing log to " << filename;
}

void Console::DrawContents()
{
#ifdef IMGUI_ENABLED

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
			if (gui::MenuItem("Clear")) ClearLog();
			if (gui::MenuItem("Copy")) ImGui::LogToClipboard();
			if (gui::MenuItem("Dump to File")) DumpToFile("console_log.txt");
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

		for (const auto& entry : log)
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

void Console::OnOpenStateChanged()
{
	// When the console is opened we should grab the keyboard focus so the user doesn't need to click on the input field.
	if (isOpen)
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

std::string Console::CreateFormattedMessage(LogLevel level, std::string_view message)
{
	auto now = std::chrono::system_clock::now();
	auto now_time_t = std::chrono::system_clock::to_time_t(now);
	auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm tm_time;
	localtime_s(&tm_time, &now_time_t);

	std::ostringstream oss;
	oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
	oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

	switch (level)
	{
	case LEVEL_DEBUG:   oss << " [DEBUG] "; break;
	case LEVEL_INFO:    oss << " [INFO] "; break;
	case LEVEL_WARNING: oss << " [WARNING] "; break;
	case LEVEL_ERROR:   oss << " [ERROR] "; break;
	case LEVEL_FATAL:   oss << " [FATAL] "; break;
	}

	oss << message;
	return oss.str();
}

void Console::DumpToFile(const std::string& filename)
{
	log_count = 0;
	std::ofstream file(filename);
	if (!file.is_open())
	{
		Log(LEVEL_ERROR) << "Failed to open log file: " << filename;
		return;
	}

	for (const auto& entry : log)
	{
		file << CreateFormattedMessage(entry.level, entry.message) << std::endl;
	}

	file.close();
	Log(LEVEL_INFO) << "Log dumped to file: " << filename;
}

void Console::SetupCrashHandler()
{
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
	log.reserve(MAX_LOG_ENTRIES);
}

LONG WINAPI Console::UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
	ST<Console>::Get()->Log(LEVEL_FATAL) << "Application Crash Detected, Dumping Log to file";
	ST<Console>::Get()->DumpToFile("console_log.txt");

	std::ofstream file("crash_log.txt");
	if (file.is_open())
	{
		file << "==============================\n";
		file << "       APPLICATION CRASH      \n";
		file << "==============================\n\n";

		file << "Crash detected!\n";
		file << "Exception Information:\n";
		file << "-------------------------\n";
		file << "Exception code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::dec << "\n";
		file << "Exception address: 0x" << std::hex << (DWORD64)(exceptionInfo->ExceptionRecord->ExceptionAddress) << std::dec << "\n";
		file << "Number of parameters: " << exceptionInfo->ExceptionRecord->NumberParameters << "\n";

		file << "\nStack Trace:\n";
		file << "-------------------------\n";
		ST<Console>::Get()->DumpStackTrace(file);

		file << "\n==============================\n";
		file << "       END OF CRASH LOG       \n";
		file << "==============================\n";

		file.close();
	}
	return EXCEPTION_EXECUTE_HANDLER;
}


/*Why is C-style memory management used here?
The SYMBOL_INFO structure from the Windows API contains a flexible array member at the end:
typedef struct _SYMBOL_INFO {
	ULONG   SizeOfStruct;
	ULONG   TypeIndex;  // Type Index of symbol
	ULONG64 Reserved[2];
	ULONG   Index;
	ULONG   Size;
	ULONG64 ModBase;    // Base of module containing this symbol
	ULONG   Flags;
	ULONG64 Value;
	ULONG64 Address;    // Address of symbol including first byte
	ULONG   Register;   // Register holding value or pointer to value
	ULONG   Scope;      // Scope of the symbol
	ULONG   Tag;        // pdb classification
	ULONG   NameLen;    // Actual length of name
	ULONG   MaxNameLen;
	CHAR    Name[1];    // The name starts here, but it's flexible
}  SYMBOL_INFO;
The Name[1] field is a flexible array member. This is a technique used in C/C++ where an array with a single element is placed at the end of a structure,
allowing you to dynamically allocate more space for the array at runtime.
In this case, the Name field is supposed to hold the name of the symbol being resolved,
and you need more space than a single character.
C++'s new operator or std::make_unique does not handle this kind of flexible memory allocation directly.
new would allocate just enough memory for the SYMBOL_INFO structure without accounting for the additional space required for the Name field,
leading to memory corruption when the program tries to write to the unallocated memory.*/
void Console::DumpStackTrace(std::ofstream& file)
{
	void* stack[62];
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	WORD numberOfFrames = CaptureStackBackTrace(0, 62, stack, NULL);

	constexpr int maxNameLength = 255;
	constexpr int symbolInfoSize = sizeof(SYMBOL_INFO) + (maxNameLength + 1) * sizeof(char);

	auto symbol = reinterpret_cast<SYMBOL_INFO*>(malloc(symbolInfoSize));

	if (symbol) // Check if malloc succeeded
	{
		memset(symbol, 0, symbolInfoSize);  // Zero out the memory for safety

		symbol->MaxNameLen = maxNameLength;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		for (int i = 0; i < numberOfFrames; i++)
		{
			if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
			{
				file << "[" << std::setw(2) << i << "] "
					<< std::setw(40) << symbol->Name
					<< " - Address: 0x" << std::hex << symbol->Address << std::dec << "\n";
			}
			else
			{
				file << "[" << std::setw(2) << i << "] Frame: [Symbol information unavailable]\n";
			}
		}

		free(symbol);  // Free the allocated memory
	}
	else
	{
		file << "Error: Failed to allocate memory for stack trace symbol info." << std::endl;
	}
}

