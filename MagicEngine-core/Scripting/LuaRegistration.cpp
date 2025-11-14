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
#include "Engine/Audio.h"
#include "Graphics/CameraComponent.h"
#include "Graphics/LightComponent.h"


//=========================================== START REGISTERING COMPONENTS ================================================================================
// This section is unfortunately required. This registers what functions are available in a component.

//Name Component
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(NameComponent)
	SCRIPT_GENERATE_PROPERTY_FUNCS(std::string, GetName, SetName) // NameComponent::GetName() / SetName() exist, and they return/set std::string type.
SCRIPT_GENERATE_COMP_WRAPPER_END()

// AudioSourceComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(AudioSourceComponent)
	SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMinDistance, SetMinDistance)
	SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMaxDistance, SetMaxDistance)
	SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetDopplerScale, SetDopplerScale)
	SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetDistanceFactor, SetDistanceFactor)
	SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetRolloffScale, SetRolloffScale)
	SCRIPT_GENERATE_PROPERTY_FUNCS(size_t, GetAudioFile, SetAudioFile)
	SCRIPT_GENERATE_PROPERTY_FUNCS(bool, IsPlaying, SetIsPlaying)
	void Play(int a) { GetHandle()->Play(static_cast<AudioType>(a)); }
SCRIPT_GENERATE_COMP_WRAPPER_END()

// CameraComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(CameraComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetActive, SetActiveLua)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetZoom, SetZoom)
SCRIPT_GENERATE_COMP_WRAPPER_END()

// AnchorToCameraComponent – no properties, but we still generate a wrapper type
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(AnchorToCameraComponent)
SCRIPT_GENERATE_COMP_WRAPPER_END()

// ShakeComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(ShakeComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetTraumaLua, SetTraumaLua)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetTraumaExponent, SetTraumaExponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetRecoverySpeed, SetRecoverySpeed)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetFrequency, SetFrequency)
SCRIPT_GENERATE_COMP_WRAPPER_END()

// LightBlinkComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(LightBlinkComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMinAlpha, SetMinAlpha)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMaxAlpha, SetMaxAlpha)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMinRadius, SetMinRadius)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMaxRadius, SetMaxRadius)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetSpeed, SetSpeed)
Vec2 AddTimeElapsed(float dt) {
	return GetHandle()->AddTimeElapsed(static_cast<float>(dt));
}
SCRIPT_GENERATE_COMP_WRAPPER_END()

// LightComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(LightComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(int, GetTypeLua, SetTypeLua)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetIntensity, SetIntensity)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetInnerConeAngle, SetInnerConeAngle)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetOuterConeAngle, SetOuterConeAngle)
SCRIPT_GENERATE_PROPERTY_FUNCS(std::string, GetName, SetName)
SCRIPT_GENERATE_COMP_WRAPPER_END()

//=========================================== END REGISTERING COMPONENTS ================================================================================

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
		.beginClass<Vec2>("Vec2")
		.addProperty("x", [](const Vec2* v) -> float { return v->x; }, [](Vec2* v, float x) { v->x = x; })
		.addProperty("y", [](const Vec2* v) -> float { return v->y; }, [](Vec2* v, float y) { v->y = y; })
		.endClass()

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

		//=========================================== START REGISTER GETTER ================================================================================

		// This section allows lua to get components
		SCRIPT_REGISTER_COMP_GETTER(NameComponent)  // syntax e.g. - local nameComp = entity:GetNameComponent(); if nameComp:Exists() then ...;
		SCRIPT_REGISTER_COMP_GETTER(AudioSourceComponent)
		SCRIPT_REGISTER_COMP_GETTER(CameraComponent)
		SCRIPT_REGISTER_COMP_GETTER(AnchorToCameraComponent)
		SCRIPT_REGISTER_COMP_GETTER(ShakeComponent)
		SCRIPT_REGISTER_COMP_GETTER(LightComponent)
		SCRIPT_REGISTER_COMP_GETTER(LightBlinkComponent)

		//=========================================== END REGISTER GETTER ================================================================================

		.endClass()
		// This section registers the component type to lua and allows lua to call functions on that component. Ensure the getter/setter names align with your GENERATE macro calls at the top of this file.

		//NameComponent
		SCRIPT_REGISTER_COMP_BEGIN(NameComponent)
			SCRIPT_REGISTER_COMP_PROPERTY(NameComponent, "name", GetName, SetName) // Calls NameComponent::GetName() / SetName(). syntax e.g. - print(nameComp.name); nameComp.name = "LuaObject";
		SCRIPT_REGISTER_COMP_END()
		
		// AudioSourceComponent
		SCRIPT_REGISTER_COMP_BEGIN(AudioSourceComponent)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "minDistance", GetMinDistance, SetMinDistance)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "maxDistance", GetMaxDistance, SetMaxDistance)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "dopplerScale", GetDopplerScale, SetDopplerScale)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "distanceFactor", GetDistanceFactor, SetDistanceFactor)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "rolloffScale", GetRolloffScale, SetRolloffScale)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "audioFile", GetAudioFile, SetAudioFile)
			SCRIPT_REGISTER_COMP_PROPERTY(AudioSourceComponent, "isPlaying", IsPlaying, SetIsPlaying)
			SCRIPT_REGISTER_COMP_FUNCTION(AudioSourceComponent, "Play", Play)
		SCRIPT_REGISTER_COMP_END()

		// CameraComponent
		SCRIPT_REGISTER_COMP_BEGIN(CameraComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(CameraComponent, "active", GetActive, SetActiveLua)
		SCRIPT_REGISTER_COMP_PROPERTY(CameraComponent, "zoom", GetZoom, SetZoom)
		SCRIPT_REGISTER_COMP_END()

		// AnchorToCameraComponent (no properties / functions for now)
		SCRIPT_REGISTER_COMP_BEGIN(AnchorToCameraComponent)
		SCRIPT_REGISTER_COMP_END()

		// ShakeComponent
		SCRIPT_REGISTER_COMP_BEGIN(ShakeComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(ShakeComponent, "trauma", GetTraumaLua, SetTraumaLua)
		SCRIPT_REGISTER_COMP_PROPERTY(ShakeComponent, "traumaExponent", GetTraumaExponent, SetTraumaExponent)
		SCRIPT_REGISTER_COMP_PROPERTY(ShakeComponent, "recoverySpeed", GetRecoverySpeed, SetRecoverySpeed)
		SCRIPT_REGISTER_COMP_PROPERTY(ShakeComponent, "frequency", GetFrequency, SetFrequency)

		//SCRIPT_REGISTER_COMP_FUNCTION(ShakeComponent,"InduceStress", InduceStress)
		//SCRIPT_REGISTER_COMP_FUNCTION(ShakeComponent,"UpdateTime",		UpdateTime)
		SCRIPT_REGISTER_COMP_END()

		// LightBlinkComponent
		SCRIPT_REGISTER_COMP_BEGIN(LightBlinkComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(LightBlinkComponent, "minAlpha", GetMinAlpha, SetMinAlpha)
		SCRIPT_REGISTER_COMP_PROPERTY(LightBlinkComponent, "maxAlpha", GetMaxAlpha, SetMaxAlpha)
		SCRIPT_REGISTER_COMP_PROPERTY(LightBlinkComponent, "minRadius", GetMinRadius, SetMinRadius)
		SCRIPT_REGISTER_COMP_PROPERTY(LightBlinkComponent, "maxRadius", GetMaxRadius, SetMaxRadius)
		SCRIPT_REGISTER_COMP_PROPERTY(LightBlinkComponent, "speed", GetSpeed, SetSpeed)
		SCRIPT_REGISTER_COMP_FUNCTION(LightBlinkComponent, "AddTimeElapsed", AddTimeElapsed)
		SCRIPT_REGISTER_COMP_END()

		// LightComponent
		SCRIPT_REGISTER_COMP_BEGIN(LightComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(LightComponent, "type", GetTypeLua, SetTypeLua)
		SCRIPT_REGISTER_COMP_PROPERTY(LightComponent, "intensity", GetIntensity, SetIntensity)
		SCRIPT_REGISTER_COMP_PROPERTY(LightComponent, "innerConeAngle", GetInnerConeAngle, SetInnerConeAngle)
		SCRIPT_REGISTER_COMP_PROPERTY(LightComponent, "outerConeAngle", GetOuterConeAngle, SetOuterConeAngle)
		SCRIPT_REGISTER_COMP_PROPERTY(LightComponent, "name", GetName, SetName)
		SCRIPT_REGISTER_COMP_END()

		// ----- GLOBAL FUNCTIONS -----
		.addFunction("Log", Lua_Log)

		// ----- GLOBAL VARIABLES -----
		.beginNamespace("LogLevel")
			.addVariable("debug", +LEVEL_DEBUG)
			.addVariable("info", +LEVEL_INFO)
			.addVariable("warning", +LEVEL_WARNING)
			.addVariable("error", +LEVEL_ERROR)
		.endNamespace()
		.beginNamespace("AudioType")
			.addVariable("BGM", static_cast<int>(AudioType::BGM))
			.addVariable("SFX", static_cast<int>(AudioType::SFX))
		.endNamespace()
		.beginNamespace("LightType")
			.addVariable("Directional", static_cast<int>(LightType::Directional))
			.addVariable("Point", static_cast<int>(LightType::Point))
			.addVariable("Spot", static_cast<int>(LightType::Spot))
			.addVariable("Area", static_cast<int>(LightType::Area))
		.endNamespace();
}
