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
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"

#define SCRIPT_FUNCTIONS \
X(update)

struct LuaScriptWithMeta : public LuaScript
{
	LuaScriptWithMeta(LuaScript&& script);
	LuaScriptWithMeta();
	LuaScriptWithMeta(const LuaScriptWithMeta&) = default; // JUST TO SATISFY STD::VECTOR COMPILATION, SHOULD NEVER BE CALLED
	LuaScriptWithMeta(LuaScriptWithMeta&&) = default;
	LuaScriptWithMeta& operator=(const LuaScriptWithMeta&) = default; // JUST TO SATISFY STD::VECTOR COMPILATION, SHOULD NEVER BE CALLED
	LuaScriptWithMeta& operator=(LuaScriptWithMeta&&) = default;


#define X(funcName) bool has_##funcName;
	SCRIPT_FUNCTIONS
#undef X

private:
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
#define X(funcName) inline static const char* funcName_##funcName{ #funcName };
	SCRIPT_FUNCTIONS
#undef X

public:
	template <typename FuncType>
	void ForEachAttachedScript(FuncType function);

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
	System used as part of ecs to updates the scripts attached to 
	entities.
*//******************************************************************/
class ScriptUpdateSystem : public ecs::System<ScriptUpdateSystem, ScriptComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptUpdateSystem();

	/*****************************************************************//*!
	\brief
		Flushes any changes made to the entity within the method call.
	\return
		None
	*//******************************************************************/
	void PostRun() override;


private:
	/*****************************************************************//*!
	\brief
		Updates the entity's attached scripts by calling their OnUpdate
		Method
	\param[in] comp
		Scriptcomponent to call update methods of.
	\return
		None
	*//******************************************************************/
	void UpdateScriptComp(ScriptComponent& comp);
};

/*****************************************************************//*!
\brief
	System used as part of ecs to invoke the awake method of the scripts attached to
	entities.
*//******************************************************************/
class ScriptAwakeSystem : public ecs::System<ScriptAwakeSystem, ScriptComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptAwakeSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates the entity's attached scripts by calling their Awake
		Method
	\param[in] comp
		Scriptcomponent to call Awake method of.
	\return
		None
	*//******************************************************************/
	void AwakenScriptComp(ScriptComponent& comp);
};

/*****************************************************************//*!
\brief
	System used as part of ecs to invoke the OnStart method of the scripts attached to
	entities.
*//******************************************************************/
class ScriptStartSystem : public ecs::System<ScriptStartSystem, ScriptComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptStartSystem();

	/*****************************************************************//*!
	\brief
		Flushes any changes made to the entity within the method call.
	\return
		None
	*//******************************************************************/
	void PostRun() override;
private:

	/*****************************************************************//*!
	\brief
		Updates the entity's attached scripts by calling their OnStart
		Method
	\param[in] comp
		Scriptcomponent to call OnStart methods of.
	\return
		None
	*//******************************************************************/
	void StartScriptComp(ScriptComponent& comp);
};

/*****************************************************************//*!
\brief
	System used as part of ecs to invoke the LateUpdate method the scripts attached to
	entities.
*//******************************************************************/
class ScriptLateUpdateSystem : public ecs::System<ScriptLateUpdateSystem, ScriptComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptLateUpdateSystem();
	
	/*****************************************************************//*!
	\brief
		Flushes any changes made to the entity within the method call.
	\return
		None
	*//******************************************************************/
	void PostRun() override;
private:

	/*****************************************************************//*!
	\brief
		Updates the entity's attached scripts by calling their LateUpdate
		Method
	\param[in] comp
		Scriptcomponent to call LateUpdate method of.
	\return
		None
	*//******************************************************************/
	void LateUpdateScriptComp(ScriptComponent& comp);
};

/*****************************************************************//*!
\brief
	System used as part of ecs to invoke the SetHandle method of the scripts attached to
	entities.
*//******************************************************************/
class ScriptPreAwakeSystem : public ecs::System<ScriptPreAwakeSystem, ScriptComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptPreAwakeSystem();
private:

	/*****************************************************************//*!
	\brief
		Ensures identity of entity by calling SetHandle Method.
	\param[in] comp
		Scriptcomponent to call SetHandle method of.
	\return
		None
	*//******************************************************************/
	void PreAwakeScriptComp(ScriptComponent& comp);
};

template<typename FuncType>
void ScriptComponent::ForEachAttachedScript(FuncType function)
{
	for (auto& script : scripts)
		function(script);
}
