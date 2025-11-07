/******************************************************************************/
/*!
\file   ScriptComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/24/2024

\author Marc Alviz Evangelista (80%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\author Matthew Chan Shao Jie (20%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This file contains the definitions of the class Script Component used for
  the engine's ECS system.
  When an entity has this component, they are able to run C# scripts.

  This also contains the definition of the ScriptSystm class used to update 
  the ScriptComponents that are attached to entities

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Scripting/ScriptComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Utilities/ScriptingUtil.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

#include "Engine/Resources/ResourceManager.h"
#include "ECS/EntityUID.h"
#include "Components/NameComponent.h"
#include "Engine/PrefabManager.h"

#define X(funcName, enumName) #funcName,
const char* const scriptFunctionNames[]{
	SCRIPT_FUNCTIONS
};
#undef X


LuaScriptWithMeta::LuaScriptWithMeta(LuaScript&& script)
	: LuaScript{ std::forward<LuaScript>(script) }
	, availableFunctions{ GetAvailableFunctionsMask() }
{
}

LuaScriptWithMeta::LuaScriptWithMeta()
	: LuaScript{}
	, availableFunctions{ GetAvailableFunctionsMask() }
{
}

const char* LuaScriptWithMeta::GetFunctionStr(SCRIPT_FUNCTION func)
{
	return scriptFunctionNames[+func];
}

int LuaScriptWithMeta::GetAvailableFunctionsMask()
{
	int mask{};
	for (SCRIPT_FUNCTION func{}; func < SCRIPT_FUNCTION::TOTAL; ++func)
		if (DoesFunctionExist(scriptFunctionNames[+func]))
			mask |= (1 << +func);
	return mask;
}

bool LuaScriptWithMeta::DoesFunctionExist(const char* funcName)
{
	try {
		code.PushGlobalFunction(funcName);
		code.Pop();
		return true;
	}
	catch (const std::runtime_error&) {
		return false;
	}
}

void ScriptComponent::RefreshScripts()
{
	for (auto scriptIter{ scripts.begin() }; scriptIter != scripts.end(); )
		if (auto newScriptInstance{ ST<LuaScripting>::Get()->GetScript(scriptIter->scriptName) })
		{
			*scriptIter = LuaScriptWithMeta{ std::move(newScriptInstance.value()) };
			++scriptIter;
		}
		else
			scriptIter = scripts.erase(scriptIter);
}

void ScriptComponent::EditorDraw()
{
	gui::TextCenteredUnformatted("Attached Scripts");
	for (auto scriptIter{ scripts.begin() }; scriptIter != scripts.end(); )
	{
		if (gui::CollapsingHeader header{ scriptIter->scriptName })
		{
			gui::SetID id{ scriptIter->scriptName.c_str() };
			gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.8f, 0.1f, 0.1f, 1.0f } };
			gui::SetStyleColor buttonHoveredColor{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 1.0f, 0.1f, 0.1f, 1.0f } };
			if (gui::Button deleteButton{ "Delete", { gui::GetAvailableContentRegion().x, 0.0f } })
			{
				scriptIter = scripts.erase(scriptIter);
				continue;
			}
		}

		++scriptIter;
	}

	gui::Spacing();
	gui::Separator();
	gui::Spacing();

	if (gui::Combo combo{ "Add Script", "Add Script" })
		for (const std::string& scriptName : ST<LuaScripting>::Get()->GetAllScriptNames())
			if (std::find_if(scripts.begin(), scripts.end(), [&scriptName](const LuaScript& existingScript) -> bool { return scriptName == existingScript.scriptName; }) == scripts.end())
				if (combo.Selectable(scriptName.c_str(), false))
					scripts.emplace_back(ST<LuaScripting>::Get()->GetScript(scriptName).value());
}

void ScriptComponent::Deserialize(Deserializer& reader)
{
	reader.PushAccess("scripts");
	for (size_t i{}; reader.PushArrayElementAccess(i); reader.PopAccess(), ++i)
	{
		std::string scriptName{};
		if (!reader.DeserializeVar("scriptName", &scriptName))
			continue;

		if (auto instantiatedScript{ ST<LuaScripting>::Get()->GetScript(scriptName) })
			scripts.push_back(std::move(instantiatedScript.value()));
	}
	reader.PopAccess();
}

ScriptRefreshListenerSystem::ScriptRefreshListenerSystem()
	: System_Internal{ &ScriptRefreshListenerSystem::RefreshScriptsInComp }
{
}

bool ScriptRefreshListenerSystem::PreRun()
{
	// If there exists a LuaScriptReloaded event, run this system to reload the scripts
	return EventsReader<Events::LuaScriptsReloaded>{}.ExtractEvent() != nullptr;
}

void ScriptRefreshListenerSystem::RefreshScriptsInComp(ScriptComponent& comp)
{
	comp.RefreshScripts();
}
