/******************************************************************************/
/*!
\file   ScriptComponent.h
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
  This file contains the declarations of the class Script Component used for 
  the engine's ECS system.
  When an entity has this component, they are able to run C# scripts.

  This also contains the declaration of the ScriptSystm class used to update
  the ScriptComponents that are attached to entities

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Scripting/LuaScripting.h"
#include "Scripting/LuaTypesECS.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Utilities/MaskTemplate.h"

// Lua function name, Enum name
#define SCRIPT_FUNCTIONS \
X(awake, AWAKE) \
X(start, START) \
X(update, UPDATE) \
X(lateUpdate, LATE_UPDATE)

enum class SCRIPT_FUNCTION {
#define X(funcName, enumName) enumName,
	SCRIPT_FUNCTIONS
#undef X
	TOTAL,
	ALL = TOTAL
};
GENERATE_ENUM_CLASS_ITERATION_OPERATORS(SCRIPT_FUNCTION)

struct LuaScriptWithMeta : public LuaScript
{
	LuaScriptWithMeta(LuaScript&& script);
	LuaScriptWithMeta(const std::string& scriptName); // Sets as invalid script
	LuaScriptWithMeta();
	LuaScriptWithMeta(const LuaScriptWithMeta&) = default; // JUST TO SATISFY STD::VECTOR COMPILATION, SHOULD NEVER BE CALLED
	LuaScriptWithMeta(LuaScriptWithMeta&&) = default;
	LuaScriptWithMeta& operator=(const LuaScriptWithMeta&) = default; // JUST TO SATISFY STD::VECTOR COMPILATION, SHOULD NEVER BE CALLED
	LuaScriptWithMeta& operator=(LuaScriptWithMeta&&) = default;

	bool valid; // Indicates whether the script successfully compiled

	MaskTemplate<SCRIPT_FUNCTION> availableFunctions;
	MaskTemplate<SCRIPT_FUNCTION> markedAsCalledFunctions; // For systems that only want to run a function once
	static const char* GetFunctionStr(SCRIPT_FUNCTION func);

private:
	int GetAvailableFunctionsMask();
	bool DoesFunctionExist(const char* funcName);
};

/*****************************************************************//*!
\brief
	Class to be used as part of the ECS system
*//******************************************************************/
class ScriptComponent
	: public IRegisteredComponent<ScriptComponent>
	, public IEditorComponent<ScriptComponent>
	, public ecs::IComponentCallbacks
{
public:
	template <typename FuncType>
	void ForEachAttachedScript(FuncType function);
	void RefreshScripts();

	// Attempts to call a function on each attached script
	template <typename ...Args>
	void CallScriptFunction(const std::string& funcName, const Args&... args);

private:
	std::vector<LuaScriptWithMeta> scripts;

public:
	void EditorDraw() override;

	void Deserialize(Deserializer& reader) override;

	property_vtable()
};
property_begin(ScriptComponent)
{
	property_var(scripts)
}
property_vend_h(ScriptComponent)

/*****************************************************************//*!
\brief
	Blueprint of an ecs system that runs a particular function in a
	lua script, passing the entity handle to it.
*//******************************************************************/
template <typename FinalSystemType, SCRIPT_FUNCTION luaFuncToCall, bool callOnlyOnce = false>
class ScriptSystemBase : public ecs::System<FinalSystemType, ScriptComponent>
{
public:
	ScriptSystemBase();

	void PostRun() override;

private:
	void UpdateScriptComp(ScriptComponent& comp);

};

template<typename FuncType>
void ScriptComponent::ForEachAttachedScript(FuncType function)
{
	for (auto& script : scripts)
		if (script.valid)
			function(script);
}

template<typename ...Args>
void ScriptComponent::CallScriptFunction(const std::string& funcName, const Args& ...args)
{
	ForEachAttachedScript([&funcName, ...args = &args](LuaScriptWithMeta& script) -> void {
		if (!script.code.PushGlobalFunction(funcName.c_str()))
			return;
		(script.code.PushArgument(args), ...);
		ST<LuaScripting>::Get()->RunScript(script);
		script.code.Pop(); // Pop global function from the stack
	});
}

template<typename FinalSystemType, SCRIPT_FUNCTION luaFuncToCall, bool callOnlyOnce>
ScriptSystemBase<FinalSystemType, luaFuncToCall, callOnlyOnce>::ScriptSystemBase()
	: ecs::internal::System_Internal<FinalSystemType, ScriptComponent>{ &ScriptSystemBase::UpdateScriptComp }
{
}

template<typename FinalSystemType, SCRIPT_FUNCTION luaFuncToCall, bool callOnlyOnce>
void ScriptSystemBase<FinalSystemType, luaFuncToCall, callOnlyOnce>::PostRun()
{
	// Flushes ecs changes made by script components
	ecs::FlushChanges();
}

template<typename FinalSystemType, SCRIPT_FUNCTION luaFuncToCall, bool callOnlyOnce>
void ScriptSystemBase<FinalSystemType, luaFuncToCall, callOnlyOnce>::UpdateScriptComp(ScriptComponent& comp)
{
	// Regarding keyword mutable: LuaWrapperEntity needs to be non-const for the lua metatable to work
	comp.ForEachAttachedScript([entity = ecs::GetEntity(&comp)](LuaScriptWithMeta& script) mutable -> void {
		if (!script.availableFunctions.TestMask(luaFuncToCall))
			return;
		if constexpr (callOnlyOnce)
		{
			if (script.markedAsCalledFunctions.TestMask(luaFuncToCall))
				return;
			script.markedAsCalledFunctions.SetMask(luaFuncToCall, true);
		}

		script.code.PushGlobalFunction(LuaScriptWithMeta::GetFunctionStr(luaFuncToCall));
		script.code.PushArgument(entity);
		ST<LuaScripting>::Get()->RunScript(script);
		script.code.Pop(); // Pop global function from the stack
	});
}

class ScriptAwakeSystem : public ScriptSystemBase<ScriptAwakeSystem, SCRIPT_FUNCTION::AWAKE, true> {};
class ScriptStartSystem : public ScriptSystemBase<ScriptStartSystem, SCRIPT_FUNCTION::START, true> {};
class ScriptUpdateSystem : public ScriptSystemBase<ScriptUpdateSystem, SCRIPT_FUNCTION::UPDATE> {};
class ScriptLateUpdateSystem : public ScriptSystemBase<ScriptLateUpdateSystem, SCRIPT_FUNCTION::LATE_UPDATE> {};

// Listens for when the lua script context is refreshed, to reload the lua states of all script components
class ScriptRefreshListenerSystem : public ecs::System<ScriptRefreshListenerSystem, ScriptComponent>
{
public:
	ScriptRefreshListenerSystem();
	bool PreRun() override;

private:
	void RefreshScriptsInComp(ScriptComponent& comp);
};
