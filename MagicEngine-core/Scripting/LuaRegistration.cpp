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
#include "LuaRegistration.h"
#include <lua.hpp>
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
#include <luabridge3/LuaBridge/LuaBridge.h>
#pragma warning(pop)
#include "LuaTypesECS.h"
#include "Components/NameComponent.h"

// This section is unfortunately required. This registers what functions are available in a component.
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(NameComponent)
	SCRIPT_GENERATE_PROPERTY_FUNCS(std::string, GetName, SetName) // NameComponent::GetName() / SetName() exist, and they return/set std::string type.
SCRIPT_GENERATE_COMP_WRAPPER_END()

void Lua_Log(int level, std::string message)
{
	switch (level)
	{
	case +LEVEL_DEBUG: case +LEVEL_INFO: case +LEVEL_WARNING: case +LEVEL_ERROR:
		CONSOLE_LOG(static_cast<LogLevel>(level)) << message;
		break;
	default:
		CONSOLE_LOG(LEVEL_INFO) << message;
		break;
	}
}

void RegisterCppStuffToLua(luabridge::Namespace baseTable)
{
	// Reference for how to do stuff: https://kunitoki.github.io/LuaBridge3/Manual

	baseTable
		// ----- CLASSES -----
		.beginClass<Vec3>("Vec3")
			.addProperty("x", [](const Vec3* v) -> float { return v->x; }, [](Vec3* v, float x) { v->x = x; })
			.addProperty("y", [](const Vec3* v) -> float { return v->y; }, [](Vec3* v, float y) { v->y = y; })
			.addProperty("z", [](const Vec3* v) -> float { return v->z; }, [](Vec3* v, float z) { v->z = z; })
		.endClass()
		.beginClass<Transform>("Transform")
			.addProperty("localPosition", &Transform::GetLocalPosition, &Transform::SetLocalPosition)
			.addProperty("localRotation", &Transform::GetLocalRotation, &Transform::SetLocalRotation)
			.addProperty("localScale",	  &Transform::GetLocalScale   , &Transform::SetLocalScale   )
			.addProperty("worldPosition", &Transform::GetWorldPosition, &Transform::SetWorldPosition)
			.addProperty("worldRotation", &Transform::GetWorldRotation, &Transform::SetWorldRotation)
			.addProperty("worldScale",	  &Transform::GetWorldScale   , &Transform::SetWorldScale   )
		.endClass()
		.beginClass<ecs::Entity>("Entity")
			.addProperty("transform", [](ecs::EntityHandle entity) -> Transform* { return &entity->GetTransform(); })
			// This section allows lua to get components
			SCRIPT_REGISTER_COMP_GETTER(NameComponent) // syntax e.g. - local nameComp = entity:GetNameComponent(); if nameComp:Exists() then ...;
		.endClass()
		// This section registers the component type to lua and allows lua to call functions on that component. Ensure the getter/setter names align with your GENERATE macro calls at the top of this file.
		SCRIPT_REGISTER_COMP_BEGIN(NameComponent)
			SCRIPT_REGISTER_COMP_PROPERTY(NameComponent, "name", GetName, SetName) // Calls NameComponent::GetName() / SetName(). syntax e.g. - print(nameComp.name); nameComp.name = "LuaObject";
		SCRIPT_REGISTER_COMP_END()

		// ----- GLOBAL FUNCTIONS -----
		.addFunction("Log", Lua_Log)

		// ----- GLOBAL VARIABLES -----
		.beginNamespace("LogLevel")
			.addVariable("debug", +LEVEL_DEBUG)
			.addVariable("info", +LEVEL_INFO)
			.addVariable("warning", +LEVEL_WARNING)
			.addVariable("error", +LEVEL_ERROR)
		.endNamespace();
}
