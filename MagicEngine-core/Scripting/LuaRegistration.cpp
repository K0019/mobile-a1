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
#include <luabridge3/LuaBridge/LuaBridge.h>
#include "LuaTypesECS.h"
#include "Components/NameComponent.h"

SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(NameComponent)
	SCRIPT_GENERATE_PROPERTY_FUNCS(std::string, GetName, SetName)
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
			.addProperty("localPosition", [](const Transform* transform) -> Vec3 { return transform->GetLocalPosition(); }, [](Transform* transform, Vec3 localPos)   -> void { transform->SetLocalPosition(localPos); })
			.addProperty("localRotation", [](const Transform* transform) -> Vec3 { return transform->GetLocalRotation(); }, [](Transform* transform, Vec3 localRot)   -> void { transform->SetLocalRotation(localRot); })
			.addProperty("localScale",	  [](const Transform* transform) -> Vec3 { return transform->GetLocalScale();	 }, [](Transform* transform, Vec3 localScale) -> void { transform->SetLocalScale(localScale);  })
			.addProperty("worldPosition", [](const Transform* transform) -> Vec3 { return transform->GetWorldPosition(); }, [](Transform* transform, Vec3 localPos)   -> void { transform->SetWorldPosition(localPos); })
			.addProperty("worldRotation", [](const Transform* transform) -> Vec3 { return transform->GetWorldRotation(); }, [](Transform* transform, Vec3 localRot)   -> void { transform->SetWorldRotation(localRot); })
			.addProperty("worldScale",	  [](const Transform* transform) -> Vec3 { return transform->GetWorldScale();	 }, [](Transform* transform, Vec3 localScale) -> void { transform->SetWorldScale(localScale);  })
		.endClass()
		.beginClass<LuaWrapperEntity>("Entity")
			.addProperty("transform", &LuaWrapperEntity::GetTransform)
			SCRIPT_REGISTER_COMP_GETTER(NameComponent)
		.endClass()
		SCRIPT_REGISTER_COMP_BEGIN(NameComponent)
			SCRIPT_REGISTER_COMP_PROPERTY(NameComponent, "name", GetName, SetName)
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
