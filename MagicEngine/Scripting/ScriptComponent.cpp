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

#include "Assets/AssetManager.h"
#include "ECS/EntityUID.h"
#include "Components/NameComponent.h"
#include "Engine/PrefabManager.h"
#include "Engine/EntityEvents.h"

#include "core/platform/platform.h"

#define X(funcName, enumName) #funcName,
const char* const scriptFunctionNames[]{
	SCRIPT_FUNCTIONS
};
#undef X


LuaScriptWithMeta::LuaScriptWithMeta(LuaScript&& script)
	: LuaScript{ std::forward<LuaScript>(script) }
	, valid{ true }
	, availableFunctions{ GetAvailableFunctionsMask() }
	, markedAsCalledFunctions{ 0 }
{
}

LuaScriptWithMeta::LuaScriptWithMeta(const std::string& scriptName)
	: LuaScript{ scriptName, LuaCpp::StateProxy{ {} } }
	, valid{}
	, availableFunctions{}
	, markedAsCalledFunctions{ 0 }
{
}

LuaScriptWithMeta::LuaScriptWithMeta(const LuaScriptWithMeta& other)
	: LuaScript{ other.scriptName, LuaCpp::StateProxy{ {} } }
	, valid{ other.valid }
	, availableFunctions{ other.availableFunctions }
	, markedAsCalledFunctions{ 0 }
{
	if (auto scriptInstance{ ST<LuaScripting>::Get()->GetScript(other.scriptName) })
		code = std::move(scriptInstance.value().code);
	else
		valid = false;
}

LuaScriptWithMeta::LuaScriptWithMeta()
	: LuaScript{}
	, valid{}
	, availableFunctions{}
	, markedAsCalledFunctions{ 0 }
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
	bool functionExists{ code.PushGlobalFunction(funcName) };
	code.Pop();
	return functionExists;
}

void ScriptComponent::RefreshScripts()
{
	for (auto scriptIter{ scripts.begin() }; scriptIter != scripts.end(); ++scriptIter)
		if (auto newScriptInstance{ ST<LuaScripting>::Get()->GetScript(scriptIter->scriptName) })
			*scriptIter = LuaScriptWithMeta{ std::move(newScriptInstance.value()) };
		else
			scriptIter->valid = false;
}

void ScriptComponent::CallScriptFunctionPlain(const std::string& funcName)
{
	CallScriptFunction(funcName, ecs::GetEntity(this));
}

void ScriptComponent::OnAttached()
{
	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->Subscribe("CallScriptFunc", this, &ScriptComponent::CallScriptFunctionPlain);
}

void ScriptComponent::OnDetached()
{
	// Hack: Don't call scripts when program is closing - for fixing a memory leak within lua.
	if (Core::Platform::Get().GetLifecycle().ShouldQuit())
		return;

	CallScriptFunction("onDestroy", ecs::GetEntity(this));

	if (auto eventsComp{ ecs::GetEntity(this)->GetComp<EntityEventsComponent>() })
		eventsComp->Unsubscribe("CallScriptFunc", this, &ScriptComponent::CallScriptFunctionPlain);
}

void ScriptComponent::EditorDraw()
{
	gui::TextCenteredUnformatted("Attached Scripts");
	for (auto scriptIter{ scripts.begin() }; scriptIter != scripts.end(); )
	{
		gui::SetStyleColor headerColor{ gui::FLAG_STYLE_COLOR::HEADER, gui::Vec4{ 0.0f, 0.0f, 0.0f, 0.156f }, !scriptIter->valid };
		gui::SetStyleColor textColor{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 1.0f, 1.0f, 1.0f, 0.3f }, !scriptIter->valid };
		if (gui::CollapsingHeader header{ scriptIter->scriptName })
		{
			gui::SetID id{ scriptIter->scriptName.c_str() };
			gui::UnsetStyleColor unsetTextColor{ (scriptIter->valid ? 0u : 1u) };
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

	gui::PayloadTargetRect<std::string>("SCRIPT_HASH", "Drag script here to add", gui::Vec2{ gui::GetAvailableContentRegion().x, 40.0f }, [this](const std::string& scriptName) -> void {
		if (std::find_if(scripts.begin(), scripts.end(), [&scriptName](const LuaScript& existingScript) -> bool { return scriptName == existingScript.scriptName; }) == scripts.end())
			scripts.emplace_back(ST<LuaScripting>::Get()->GetScript(scriptName).value());
	});
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
		else
			scripts.emplace_back(scriptName);
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
