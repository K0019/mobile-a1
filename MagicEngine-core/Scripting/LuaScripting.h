/******************************************************************************/
/*!
\file   LuaScripting.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   11/01/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides an interface for lua scripting.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "LuaLibrary/LuaCpp.hpp"
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
#include <luabridge3/LuaBridge/LuaBridge.h>
#pragma warning(pop)
#include "Utilities/Serializer.h"
#include "Engine/Events/EventsQueue.h"

class LuaScripting;

#pragma region User Types

struct LuaScript : public ISerializeableWithoutJsonObj
{
	LuaScript(const std::string& scriptName, LuaCpp::StateProxy&& code);
	LuaScript();
	LuaScript(const LuaScript&); // JUST TO SATISFY STD::VECTOR COMPILATION, SHOULD NEVER BE CALLED
	LuaScript(LuaScript&&) = default;
	LuaScript& operator=(const LuaScript&); // JUST TO SATISFY STD::VECTOR COMPILATION, SHOULD NEVER BE CALLED
	LuaScript& operator=(LuaScript&&) = default;

	std::string scriptName;
	LuaCpp::StateProxy code;

	property_vtable()
};
property_begin(LuaScript)
{
	property_var(scriptName)
}
property_vend_h(LuaScript)

struct LuaLibrary
{
	const SPtr<LuaCpp::Registry::LuaLibrary>& libPtr;
};

#pragma endregion // User Types

class LuaScripting
{
public:
	void Init();
	void LoadScriptsInFolder(const std::string& folder);
	void DisposeAllLoadedScripts();

	// fullName should be how referring to the function looks like in lua.
	// e.g. Magic.FuncX
	template <typename FuncType>
	static void RegisterFunction(const std::string& fullName, FuncType function);

	std::optional<LuaScript> GetScript(const std::string& scriptName);
	const std::vector<std::string>& GetAllScriptNames() const;

	void RunScript(LuaScript& script);

private:
	// Retrieves the lua library (equivalent to c++ namespaces).
	// If it doesn't exist yet, makes the library before returning.
	LuaLibrary GetLibrary(const std::string& libName);

private:
	//! The lua driver
	LuaCpp::LuaContext context;

	//! Libraries (so functions can be organized into namespaces)
	std::unordered_map<std::string, SPtr<LuaCpp::Registry::LuaLibrary>> libraries;

	//! Record of the available scripts
	std::vector<std::string> loadedScripts;

	AutoEventHandler reloadEventHandler;
};

template<typename FuncType>
void LuaScripting::RegisterFunction(const std::string& fullName, FuncType function)
{
	// Split to library and function name
	size_t periodPos{ fullName.rfind('.') };
	std::string libName{ periodPos != std::numeric_limits<size_t>::max() ? fullName.substr(0, periodPos) : "Magic" };
	std::string funcName{ periodPos != std::numeric_limits<size_t>::max() ? fullName.substr(periodPos + 1) : fullName };

	// Register the function
	ST<LuaScripting>::Get()->GetLibrary(libName).libPtr->AddCFunction(funcName, function);
}
