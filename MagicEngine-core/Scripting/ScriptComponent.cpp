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
#ifdef GLFW
#include "Scripting/ScriptComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Utilities/ScriptingUtil.h"

#include "Engine/Resources/ResourceManager.h"
#include "ECS/EntityUID.h"
#include "Components/NameComponent.h"
#include "Engine/PrefabManager.h"


LuaScriptWithMeta::LuaScriptWithMeta(LuaScript&& script)
	: LuaScript{ std::forward<LuaScript>(script) }
#define X(funcName) , has_##funcName{ DoesFunctionExist(#funcName) }
	SCRIPT_FUNCTIONS
#undef X
{
}

LuaScriptWithMeta::LuaScriptWithMeta()
	: LuaScript{}
#define X(funcName) , has_##funcName{}
	SCRIPT_FUNCTIONS
#undef X
{
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


ScriptUpdateSystem::ScriptUpdateSystem()
	: System_Internal{ &ScriptUpdateSystem::UpdateScriptComp }
{
}

void ScriptUpdateSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptUpdateSystem::UpdateScriptComp(ScriptComponent& comp)
{
	comp.ForEachAttachedScript([](LuaScript& script) -> void {
		script.code.PushGlobalFunction(ScriptComponent::funcName_update);
		ST<LuaScripting>::Get()->RunScript(script);
		script.code.Pop();
	});
}

ScriptAwakeSystem::ScriptAwakeSystem()
	: System_Internal{ &ScriptAwakeSystem::AwakenScriptComp }

{
}

void ScriptAwakeSystem::AwakenScriptComp(ScriptComponent& comp)
{
	//comp.InvokeAwake();
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
	//comp.InvokeOnStart();
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
	//comp.InvokeLateUpdate();
}

ScriptPreAwakeSystem::ScriptPreAwakeSystem()
	: System_Internal{ &ScriptPreAwakeSystem::PreAwakeScriptComp }
{
}

void ScriptPreAwakeSystem::PreAwakeScriptComp(ScriptComponent& comp)
{
	//comp.InvokeSetHandle();
}


//#pragma region Scripting
//
///*****************************************************************//*!
//\brief
//	Gets a GCHandle to a C# script object that's attached to an entity.
//	Freeing the handle doesn't delete the script object, but do still
//	remember to free the handle on C# side, otherwise there will be a
//	memory leak!
//\param entity
//	The entity to search for the script object on.
//\param typeName
//	The name of the class of the C# script object. If the class is defined
//	inside a namespace, it must be included in the name and delimited by '.'
//\return
//	A C# IntPtr object which can be converted into a GCHandle via GCHandle.FromIntPtr().
//	If the script object doesn't exist on the entity, returns 0. On C#,
//	this can be checked for by a comparison against IntPtr.Zero.
//*//******************************************************************/
//SCRIPT_CALLABLE intptr_t CS_GetScriptInstance(ecs::EntityHandle entity, const char* typeName)
//{
//	util::AssertEntityHandleValid(entity);
//	auto scriptComp{ entity->GetComp<ScriptComponent>() };
//	if (!scriptComp)
//		return 0;
//
//	if (MonoObject* scriptInstance{ scriptComp->FindScriptInstance(typeName) })
//		return mono_gchandle_new(scriptInstance, false);
//	else
//		return 0;
//
//}
//
///*****************************************************************//*!
//\brief
//	Searches the world for an entity of the specified name.
//	THIS IS VERY EXPENSIVE!
//\param name
//	The name of the entity.
//\return
//	The entity. If not found, 0.
//*//******************************************************************/
//SCRIPT_CALLABLE ecs::EntityHandle CS_FindEntityByName(const char* name)
//{
//	for (auto iter = ecs::GetCompsBegin<NameComponent>(); iter != ecs::GetCompsEnd<NameComponent>(); ++iter)
//		if (iter->GetName() == name)
//			return iter.GetEntity();
//	return nullptr;
//}
//
///*****************************************************************//*!
//\brief
//	Deletes an entity.
//\param entity
//	The entity.
//*//******************************************************************/
//SCRIPT_CALLABLE void CS_DeleteEntity(ecs::EntityHandle entity)
//{
//	util::AssertEntityHandleValid(entity);
//	ecs::DeleteEntity(entity);
//}
//
///*****************************************************************//*!
//\brief
//	Deferred clones an entity.
//\param entity
//	The entity.
//\param parent
//	The entity to parent the cloned entity to. If null, the new entity
//	does not have a parent.
//\return
//	The cloned entity.
//*//******************************************************************/
//SCRIPT_CALLABLE ecs::EntityHandle CS_CloneEntity(ecs::EntityHandle entity, ecs::EntityHandle parent)
//{
//	util::AssertEntityHandleValid(entity);
//	ecs::EntityHandle clonedEntity{ ecs::CloneEntity(entity) };
//	if (parent)
//	{
//		util::AssertEntityHandleValid(parent);
//		clonedEntity->GetTransform().SetParent(parent->GetTransform());
//	}
//	return clonedEntity;
//}
//
///*****************************************************************//*!
//\brief
//	Spawns a prefab as a new entity.
//\param prefabName
//	The name of the prefab.
//\param parent
//	The entity to parent the cloned entity to. If null, the new entity
//	does not have a parent.
//\return
//	The spawned prefab entity.
//*//******************************************************************/
//SCRIPT_CALLABLE ecs::EntityHandle CS_SpawnPrefab(const char* prefabName, ecs::EntityHandle parent)
//{
//	ecs::EntityHandle prefabEntity{ ST<PrefabManager>::Get()->LoadPrefab(prefabName) };
//	if (parent)
//	{
//		util::AssertEntityHandleValid(parent);
//		prefabEntity->GetTransform().SetParent(parent->GetTransform());
//	}
//	return prefabEntity;
//}
//
//#pragma endregion // Scripting
#endif
