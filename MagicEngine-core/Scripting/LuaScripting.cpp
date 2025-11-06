/******************************************************************************/
/*!
\file   LuaScripting.cpp
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
#include "LuaScripting.h"
#include <VFS/VFS.h>

LuaScript::LuaScript(const std::string& scriptName, LuaCpp::StateProxy&& code)
	: scriptName{ scriptName }
	, code{ std::forward<LuaCpp::StateProxy>(code) }
{
}

LuaScript::LuaScript()
	: code{ {} }
{
}

LuaScript::LuaScript(const LuaScript&)
	: code{ {} }
{
	CONSOLE_LOG(LEVEL_FATAL) << "LuaScript copy constructor called. SHOULD NEVER BE CALLED";
}

LuaScript& LuaScript::operator=(const LuaScript&)
{
	CONSOLE_LOG(LEVEL_FATAL) << "LuaScript copy assignment called. SHOULD NEVER BE CALLED";
	return *this;
}

void LuaScripting::LoadScriptsInFolder(const std::string& folder)
{
	for (const auto& file : VFS::ListDirectory(folder))
	{
		// Ignore if not lua file
		if (VFS::GetExtension(file) != ".lua")
			continue;

		// Load file content
		std::string fileContents{};
		if (!VFS::ReadFile(VFS::JoinPath(folder, file), fileContents))
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Failed to load script file " << file;
			continue;
		}

		// Register script
		try {
			std::string scriptName{ VFS::GetFilename(file) };
			context.CompileString(scriptName, fileContents);
			loadedScripts.push_back(std::move(scriptName));
			CONSOLE_LOG(LEVEL_DEBUG) << "Compiled script successfully: " << file;
		}
		catch (const std::logic_error& e) {
			CONSOLE_LOG(LEVEL_ERROR) << "Script compile error: " << file << " - " << e.what();
		}
		catch (const std::runtime_error& e) {
			CONSOLE_LOG(LEVEL_ERROR) << "Unexpected error while compiling script: " << file << " - " << e.what();
		}
	}

	// Sort script names alphabetically
	std::sort(loadedScripts.begin(), loadedScripts.end());
}

std::optional<LuaScript> LuaScripting::GetScript(const std::string& scriptName)
{
	try {
		return LuaScript{ scriptName, context.CreateStateFor(scriptName) };
	}
	catch (const std::runtime_error&) {
		// The script doesn't exist
		return std::nullopt;
	}
}

const std::vector<std::string>& LuaScripting::GetAllScriptNames() const
{
	return loadedScripts;
}

void LuaScripting::RunScript(LuaScript& script)
{
	context.Run(script.code);
}

LuaLibrary LuaScripting::GetLibrary(const std::string& libName)
{
	auto libIter{ libraries.find(libName) };
	if (libIter != libraries.end())
		return LuaLibrary{ libIter->second };

	const auto& newLib{ libraries.insert({ libName, std::make_shared<LuaCpp::Registry::LuaLibrary>(libName) }).first->second };
	ST<LuaScripting>::Get()->context.AddLibrary(newLib);
	return LuaLibrary{ newLib };
}
