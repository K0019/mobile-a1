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

void TestFunction(LuaWrapperEntity entity)
{
	std::cout << "HELLO FROM LUA! by " << entity->GetComp<NameComponent>()->GetName();
}

void RegisterCppStuffToLua(luabridge::Namespace baseTable)
{
	// Reference for how to do stuff: https://kunitoki.github.io/LuaBridge3/Manual

	baseTable
		// ----- CLASSES -----
		.beginClass<LuaWrapperEntity>("Entity")
			SCRIPT_REGISTER_COMP_GETTER(NameComponent)
		.endClass()
		SCRIPT_REGISTER_COMP_BEGIN(NameComponent)
			SCRIPT_REGISTER_COMP_PROPERTY(NameComponent, "name", GetName, SetName)
		SCRIPT_REGISTER_COMP_END()
		/*.beginClass<LuaWrapperComp_NameComponent>("NameComp")
			.addFunction("exists", &LuaWrapperComp_NameComponent::Valid)
			.addProperty("name", &LuaWrapperComp_NameComponent::GetName, LuaWrapperComp_NameComponent::SetName)
		.endClass()*/

		// ----- GLOBAL FUNCTIONS -----
		.addFunction("TestFunction", TestFunction);
}
