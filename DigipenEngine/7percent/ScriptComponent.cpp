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
#include "ScriptComponent.h"

#include "ResourceManager.h"
#include "EntityUID.h"
#include "NameComponent.h"
#include "PrefabManager.h"

using namespace CSharpScripts;

ScriptComponent::ScriptComponent(const ScriptComponent& other)
	: scriptMap(other.scriptMap)
{
	scriptsToAwaken = other.scriptsToAwaken;
	scriptsToStart = other.scriptsToStart;
	//for (auto const& pair : other.scriptMap)
	//{
	//	AddScript(pair.first);
	//}
}

ScriptComponent::ScriptComponent(ScriptComponent&& other) noexcept
	: scriptMap(std::move(other.scriptMap))
{
	scriptsToAwaken = std::move(other.scriptsToAwaken);
	scriptsToStart = std::move(other.scriptsToStart);
}

ScriptComponent::~ScriptComponent()
{
	scriptMap.clear();
	openStates.clear();
	scriptsToAwaken.clear();
	scriptsToStart.clear();
}

void ScriptComponent::EditorDraw()
{
	// Don't draw anything if the compilation is ongoing. We can't be sure if anything exists in that case.
	if (CSScripting::IsCurrentlyCompilingUserAssembly())
		return;

	gui::TextUnformatted("Attached Scripts:");

	for (auto& [scriptName, scriptInstance] : scriptMap)
	{
		{ // Draw scripts as buttons
			if (gui::Button scriptButton{ scriptName.c_str() })
				openStates[scriptName] = !openStates[scriptName];
			gui::SameLine();
			gui::SetID buttonID{ scriptName.c_str() };
			if (gui::Button removeButton{ "Remove" })
			{
				RemoveScript(scriptName);
				break;
			}
		}

		if (!openStates[scriptName])
			continue;

		// Define the height of the child rect based on the number of public variables
		auto& publicVars{ scriptInstance.GetPublicVars() };
		const float itemHeight = ImGui::GetTextLineHeightWithSpacing(); // Height of each item
		const float padding = 45.0f; // Extra padding for aesthetics
		float totalHeight = (publicVars.size() * itemHeight) + padding;

		// Draw each public variable
		if (gui::Child rectChild{ ("Child Rect" + scriptName).c_str(), gui::Vec2{ 0, totalHeight }, gui::FLAG_CHILD::NONE, gui::FLAG_WINDOW::NO_TITLE_BAR })
		{
			gui::TextFormatted("Details for %s", scriptName.c_str());
			for (auto& [varName, varField] : publicVars)
				varField.EditorDraw(varName, scriptInstance.GetInstance());
		}
	}

	// Draw add script dropdown
	gui::TextUnformatted("Add New Script:");
	if (gui::Combo addScriptCombo{ "", "Select Script" })
		for (const auto& [scriptName, _] : CSScripting::GetCoreClassMap())
			if (scriptMap.find(scriptName) == scriptMap.end())
				if (gui::Selectable(scriptName.c_str()))
					AddScript(scriptName);
}

bool ScriptComponent::AddScript(const std::string& sName)
{
	// We can't add a duplicate of a script
	if (scriptMap.find(sName) != scriptMap.end())
		return false;

	ScriptInstance& scriptInstance{ scriptMap.try_emplace(sName, CSScripting::GetClassFromData(sName)).first->second };
	openStates[sName] = true;

	InitializeScriptInstance(scriptInstance, sName);

	return true;
}

void ScriptComponent::RemoveScript(const std::string& sName)
{
	auto it{ scriptMap.find(sName) };
	if (it == scriptMap.end())
		return;

	auto ite{ std::find(scriptsToAwaken.begin(), scriptsToAwaken.end(), sName) };
	if (ite != scriptsToAwaken.end())
		scriptsToAwaken.erase(ite);

	ite = std::find(scriptsToStart.begin(), scriptsToStart.end(), sName);
	if (ite != scriptsToStart.end())
		scriptsToStart.erase(ite);

	openStates.erase(sName);
	scriptMap.erase(it);
}

void ScriptComponent::InvokeOnUpdate()
{
	float dt{ GameTime::FixedDt() };
	for (auto& pair : scriptMap)
		pair.second.InvokeMethod(METHOD::ON_UPDATE, &dt);
}

void ScriptComponent::InvokeAwake()
{
	if (scriptsToAwaken.empty())
		return;

	for (const std::string& name : scriptsToAwaken)
		scriptMap[name].InvokeMethod(METHOD::ON_AWAKE);

	// Move awaken scripts to the list of scripts to start
	scriptsToStart.insert(scriptsToStart.end(), std::make_move_iterator(scriptsToAwaken.begin()), std::make_move_iterator(scriptsToAwaken.end()));
	scriptsToAwaken.clear();
}

void ScriptComponent::InvokeOnStart()
{
	if (scriptsToStart.empty())
		return;

	for (const std::string& name : scriptsToStart)
		scriptMap[name].InvokeMethod(METHOD::ON_START);

	scriptsToStart.clear();
}

void ScriptComponent::InvokeLateUpdate()
{
	float dt{ GameTime::FixedDt() };
	for (auto& pair : scriptMap)
		pair.second.InvokeMethod(METHOD::ON_LATE_UPDATE, &dt);
}

void ScriptComponent::SaveVariables()
{
	// Clear all these first

	// Save everything first
	pvs.clear();
	names.clear();
	scriptsToAwaken.clear();
	scriptsToStart.clear();

	for (auto& pair : scriptMap)
	{
		names.push_back(pair.first);
		pvs.push_back(pair.second.GetPublicVars());
	}
}

void ScriptComponent::InvokeSetHandle()
{
	ecs::EntityHandle entity{ ecs::GetEntity(this) };
	for (auto& pair : scriptMap)
		pair.second.InvokeMethod(METHOD::SET_HANDLE, &entity);
}

MonoObject* ScriptComponent::FindScriptInstance(const std::string& name)
{
	auto it = scriptMap.find(name);
	if (it != scriptMap.end())
		return it->second.GetInstance();
	return nullptr;
}

void ScriptComponent::Serialize(Serializer& writer) const
{
	//Serialize scriptMap
	//Maybe serailize openStates as well? Could be convenient
	writer.Serialize("scriptMap", scriptMap);
}

void ScriptComponent::Deserialize(Deserializer& entry)
{
	entry.DeserializeVar("scriptMap",&scriptMap);

	for (auto& script : scriptMap)
		InitializeScriptInstance(script.second, script.first);
}

void ScriptComponent::RemoveAllScripts()
{
	for (std::string n : names)
	{
		RemoveScript(n);
	}
}

void ScriptComponent::ReattachAllScripts()
{
	// After all the scripts
	for (std::string n : names)
	{
		AddScript(n);
		scriptMap[n].RetrievePublicVariables();
	}
}

void ScriptComponent::LoadVariables()
{
	// Load variables saved previous before reload back into the scripts
	for (size_t i = 0; i < names.size(); ++i)
		for (auto& pair : pvs[i])
			scriptMap[names[i]].SetPublicVar(pair.first, pair.second.GetValue());

	pvs.clear();
	names.clear();
}

void ScriptComponent::OnAttached()
{
	//ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->Subscribe("OnCollision", this, &ScriptComponent::OnCollision);
}

void ScriptComponent::OnDetached()
{
	/*if (auto eventsComp{ ecs::GetEntity(this)->GetComp<EntityEventsComponent>() })
		eventsComp->Unsubscribe("OnCollision", this, &ScriptComponent::OnCollision);*/
}

void ScriptComponent::InitializeScriptInstance(CSharpScripts::ScriptInstance& instance, const std::string& scriptName)
{
	ecs::EntityHandle entity{ ecs::GetEntity(this) };
	instance.InvokeMethod(METHOD::SET_HANDLE, &entity);
	instance.InvokeMethod(METHOD::ON_AWAKE);

	instance.RetrievePublicVariables();
	scriptsToAwaken.push_back(scriptName);
}


ScriptSystem::ScriptSystem()
	: System_Internal{ &ScriptSystem::UpdateScriptComp }
{
}

void ScriptSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptSystem::UpdateScriptComp(ScriptComponent& comp)
{
	comp.InvokeOnUpdate();
}

ScriptAwakeSystem::ScriptAwakeSystem()
	: System_Internal{ &ScriptAwakeSystem::AwakenScriptComp }

{
}

void ScriptAwakeSystem::AwakenScriptComp(ScriptComponent& comp)
{
	comp.InvokeAwake();
}

ScriptStartSystem::ScriptStartSystem()
	: System_Internal{ &ScriptStartSystem::StartScriptComp }

{
}

void ScriptStartSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptStartSystem::StartScriptComp(ScriptComponent& comp)
{
	comp.InvokeOnStart();
}

ScriptLateUpdateSystem::ScriptLateUpdateSystem()
	: System_Internal{ &ScriptLateUpdateSystem::LateUpdateScriptComp }

{
}

void ScriptLateUpdateSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptLateUpdateSystem::LateUpdateScriptComp(ScriptComponent& comp)
{
	comp.InvokeLateUpdate();
}

ScriptPreAwakeSystem::ScriptPreAwakeSystem()
	: System_Internal{ &ScriptPreAwakeSystem::PreAwakeScriptComp }
{
}

void ScriptPreAwakeSystem::PreAwakeScriptComp(ScriptComponent& comp)
{
	comp.InvokeSetHandle();
}


#pragma region Scripting

SCRIPT_CALLABLE intptr_t CS_GetScriptInstance(ecs::EntityHandle entity, const char* typeName)
{
	auto scriptComp{ util::AssertCompExistsOnValidEntityAndGet<ScriptComponent>(entity) };
	return mono_gchandle_new(scriptComp->FindScriptInstance(typeName), false);
}

SCRIPT_CALLABLE ecs::EntityHandle CS_FindEntityByName(const char* name)
{
	for (auto iter = ecs::GetCompsBegin<NameComponent>(); iter != ecs::GetCompsEnd<NameComponent>(); ++iter)
		if (iter->GetName() == name)
			return iter.GetEntity();
	return nullptr;
}

SCRIPT_CALLABLE void CS_DeleteEntity(ecs::EntityHandle entity)
{
	util::AssertEntityHandleValid(entity);
	ecs::DeleteEntity(entity);
}

SCRIPT_CALLABLE ecs::EntityHandle CS_CloneEntity(ecs::EntityHandle entity, ecs::EntityHandle parent)
{
	util::AssertEntityHandleValid(entity);
	ecs::EntityHandle clonedEntity{ ecs::CloneEntity(entity) };
	if (parent)
	{
		util::AssertEntityHandleValid(parent);
		clonedEntity->GetTransform().SetParent(parent->GetTransform());
	}
	return clonedEntity;
}

SCRIPT_CALLABLE ecs::EntityHandle CS_SpawnPrefab(const char* prefabName, ecs::EntityHandle parent)
{
	ecs::EntityHandle prefabEntity{ ST<PrefabManager>::Get()->LoadPrefab(prefabName) };
	if (parent)
	{
		util::AssertEntityHandleValid(parent);
		prefabEntity->GetTransform().SetParent(parent->GetTransform());
	}
	return prefabEntity;
}

#pragma endregion // Scripting
