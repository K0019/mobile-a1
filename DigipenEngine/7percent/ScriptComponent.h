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
#include "Physics.h"
#include "CSScripting.h"



/*****************************************************************//*!
\brief
	Class to be used as part of the ECS system
*//******************************************************************/
class ScriptComponent : public IRegisteredComponent<ScriptComponent>, ecs::IComponentCallbacks
#ifdef IMGUI_ENABLED
	, IEditorComponent<ScriptComponent>
#endif
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptComponent();

	/*****************************************************************//*!
	\brief
		Copy Constructor of the class
	\param[in]
		Reference of another ScriptComponent to copy from
	\return
		None
	*//******************************************************************/
	ScriptComponent(const ScriptComponent& other);

	/*****************************************************************//*!
	\brief
		Move Constructor of the class
	\param[in]
		ScriptComponent to take over ownership of.
	\return
		None
	*//******************************************************************/
	ScriptComponent(ScriptComponent&& other) noexcept;

	/*****************************************************************//*!
	\brief
		Destructor of the class
	\return
		None
	*//******************************************************************/
	~ScriptComponent();

	/*****************************************************************//*!
	\brief
		Draws in editor, the Script component. Will draw the scripts 
		attached inside the component and the scripts variables that can
		be adjusted/manipulated during runtime.
	\param[in,out] comp
		Script Component to draw in the editor
	\return
		None
	*//******************************************************************/
#ifdef IMGUI_ENABLED
	static void EditorDraw(ScriptComponent& comp);
#endif

	/*****************************************************************//*!
	\brief
		Adds to the unordered map of scripts attached to the entity containing
		this component.
	\param[in] sName
		Name of the Script to add
	\return
		None
	*//******************************************************************/
	void AddScript(const std::string sName);
	/*****************************************************************//*!
	\brief
		Removes from the unordered map of scripts attached to the entity containing
		this component.
	\param[in] sName
		Name of the Script to remove
	\return
		None
	*//******************************************************************/
	void RemoveScript(const std::string sName);

	/*****************************************************************//*!
	\brief
		Calls the InvokeOnUpdate method of each script inside the 
		unordered map.
	\return
		None
	*//******************************************************************/
	void InvokeOnUpdate();

	/*****************************************************************//*!
	\brief
		Calls the InvokeAwake method of each script inside the
		unordered map.
	\return
		None
	*//******************************************************************/
	void InvokeAwake();

	/*****************************************************************//*!
	\brief
		Calls the InvokeOnStart method of each script inside the
		unordered map.
	\return
		None
	*//******************************************************************/
	void InvokeOnStart();

	/*****************************************************************//*!
	\brief
		Calls the InvokeLateUpdate method of each script inside the
		unordered map.
	\return
		None
	*//******************************************************************/
	void InvokeLateUpdate();

	/*****************************************************************//*!
	\brief
		Saves varaibles of all scripts attached
	\return
		None
	*//******************************************************************/
	void SaveVariables();

	/*****************************************************************//*!
	\brief
		Removes all scripts attached to the component
	\return
		None
	*//******************************************************************/
	void RemoveAllScripts();

	/*****************************************************************//*!
	\brief
		Reattaches all scripts removed to the component
	\return
		None
	*//******************************************************************/
	void ReattachAllScripts();

	/*****************************************************************//*!
	\brief
		Loads varaibles of all scripts attached
	\return
		None
	*//******************************************************************/
	void LoadVariables();

	/*****************************************************************//*!
	\brief
		Calls the InvokeSetHandle method of each script inside the
		unordered map.
	\return
		None
	*//******************************************************************/
	void InvokeSetHandle();

	/*****************************************************************//*!
	\brief
		Tries to find the specified script instance attached to the entity.
	\param[in] name
		Name of the script class to find.
	\return
		MonoObject* of the class
	*//******************************************************************/
	MonoObject* FindScriptInstance(std::string name);

	/*****************************************************************//*!
	\brief
		Function to call when this object is serialized.
	\param[in] serializer
		The Serializer instance.
	\return
		None
	*//******************************************************************/
	void Serialize(Serializer& serializer) const final;
	/*****************************************************************//*!
	\brief
		Function to call when this object is deserialized.
	\param[in] deserializer
		The Deserializer instance.
	\return
		None
	*//******************************************************************/
	void Deserialize(Deserializer& deserializer) final;

	/*****************************************************************//*!
	\brief
		Registers to the collision callback on this entity.
	*//******************************************************************/
	void OnAttached() override;

	/*****************************************************************//*!
	\brief
		Unregisters from the collision callback on this entity.
	*//******************************************************************/
	void OnDetached() override;

private:
	/*****************************************************************//*!
	\brief
		Sets the entity for deletion.
	\param collisionData
		The data of the collision.
	*//******************************************************************/
	void OnCollision(const Physics::CollisionEventData& collisionData);

	std::unordered_map<std::string, CSharpScripts::ScriptInstance> scriptMap;
	std::vector<std::string> scriptsToAwaken;
	std::vector<std::string> scriptsToStart;
	// For EditorDraw
	std::unordered_map<std::string, bool> openStates;

	// Map containing:
	//	Key: Name of Script, Value: Map Containing:
	//							Key: name of variable, Value: value of variable
	std::vector<std::string> names;
	std::vector<std::unordered_map<std::string, CSharpScripts::internal::Field>> pvs;
	property_vtable()
};
property_begin(ScriptComponent)
{
}
property_vend_h(ScriptComponent)

/*****************************************************************//*!
\brief
	System used as part of ecs to updates the scripts attached to 
	entities.
*//******************************************************************/
class ScriptSystem : public ecs::System<ScriptSystem, ScriptComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor of the class
	\return
		None
	*//******************************************************************/
	ScriptSystem();

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