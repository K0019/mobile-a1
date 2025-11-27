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

All content � 2024 DigiPen Institute of Technology Singapore.
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
#include "Components/EntityReferenceHolder.h"
#include "Engine/Audio.h"
#include "Graphics/CameraComponent.h"
#include "Graphics/LightComponent.h"
#include "Physics/Collision.h"  
#include "Physics/Physics.h"
#include "Game/GameCameraController.h"
#include "Game/Character.h"
#include "Game/PlayerCharacter.h"
#include "Game/GrabbableItem.h"
#include "Game/Health.h"
#include "Engine/EntityLayers.h"
#include "Managers/AudioManager.h"
#include "Engine/Input.h"
#include "UI/SpriteComponent.h"
#include "Engine/SceneManagement.h"
#include "Utilities/Scheduler.h"
#include "Scripting/ScriptComponent.h"
#include "Graphics/RenderComponent.h"
#include "Engine/PrefabManager.h"

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

// AnchorToCameraComponent � no properties, but we still generate a wrapper type
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

using BoxColliderComp = physics::BoxColliderComp;

// BoxColliderComp
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(BoxColliderComp)
SCRIPT_GENERATE_PROPERTY_FUNCS(Vec3, GetCenter, SetCenter)
SCRIPT_GENERATE_PROPERTY_FUNCS(Vec3, GetSize, SetSize)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetEnabled, SetEnabled)
SCRIPT_GENERATE_COMP_WRAPPER_END()

using PhysicsComp = physics::PhysicsComp;
// PhysicsComp
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(PhysicsComp)
SCRIPT_GENERATE_PROPERTY_FUNCS(Vec3, GetLinearVelocity, SetLinearVelocity)
SCRIPT_GENERATE_PROPERTY_FUNCS(Vec3, GetAngularVelocity, SetAngularVelocity)

SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetIsKinematic, SetIsKinematic)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetUseGravity, SetUseGravity)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetRotLockedX, SetRotLockedX)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetRotLockedY, SetRotLockedY)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetRotLockedZ, SetRotLockedZ)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetEnabled, SetEnabled)
void AddLinearVelocity(const Vec3& vel)
{
	GetHandle()->AddLinearVelocity(vel);
}
SCRIPT_GENERATE_COMP_WRAPPER_END()


// GameCameraControllerComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(GameCameraControllerComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCameraAutoZoomSpeed, SetCameraAutoZoomSpeed)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetTargetCameraDistance, SetTargetCameraDistance)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCurrentCameraDistance, SetCurrentCameraDistance)
SCRIPT_GENERATE_PROPERTY_FUNCS(Vec3, GetOffsetPosition, SetOffsetPosition)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCameraSensitivity, SetCameraSensitivity)
SCRIPT_GENERATE_COMP_WRAPPER_END()

// CharacterMovementComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(CharacterMovementComponent)

// Properties
SCRIPT_GENERATE_PROPERTY_FUNCS(Vec2, GetMovementVector, SetMovementVector)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMoveSpeed, SetMoveSpeed)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetRotateSpeed, SetRotateSpeed)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetStunTimePerHit, SetStunTimePerHit)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetGroundFriction, SetGroundFriction)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetDodgeCooldown, SetDodgeCooldown)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetDodgeDuration, SetDodgeDuration)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetDodgeSpeed, SetDodgeSpeed)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCurrentStunTime, SetCurrentStunTime)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCurrentDodgeTime, SetCurrentDodgeTime)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCurrentDodgeCooldown, SetCurrentDodgeCooldown)

//functions
void Dodge(const Vec2 v) { GetHandle()->Dodge(v); }
void RotateTowards(const Vec2 v) { GetHandle()->RotateTowards(v); }
void SetMovementVectorLua(const Vec2 v) { GetHandle()->SetMovementVector(v); }
void DropItem() { GetHandle()->DropItem(); }
void Throw(const Vec3 dir) { GetHandle()->Throw(dir); }
void Attack() { GetHandle()->Attack(); }
SCRIPT_GENERATE_COMP_WRAPPER_END()

// PlayerMovementComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(PlayerMovementComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetGrabDistance, SetGrabDistance)
SCRIPT_GENERATE_COMP_WRAPPER_END()

// GrabbableItemComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(GrabbableItemComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetDamage, SetDamage)
SCRIPT_GENERATE_PROPERTY_FUNCS(bool, GetIsHeld, SetIsHeld)
void AttackLua(const Vec3& origin, const Vec3& direction)
{ GetHandle()->Attack(origin, direction);}
SCRIPT_GENERATE_COMP_WRAPPER_END()

// HealthComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(HealthComponent)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetCurrHealth, SetHealth)
SCRIPT_GENERATE_PROPERTY_FUNCS(float, GetMaxHealth, SetMaxHealth)
void AddHealthLua(float amount)
{
	GetHandle()->AddHealth(amount);
}

void TakeDamageLua(float amount, const Vec3& direction)
{
	GetHandle()->TakeDamage(amount, direction);
}

bool IsDead() const
{
	return GetHandle()->IsDead();
}

float GetHealthFractionLua() const
{
	return GetHandle()->GetHealthFraction();
}
SCRIPT_GENERATE_COMP_WRAPPER_END()

// EntityLayerComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(EntityLayerComponent)
int GetLayerLua() const
{
	return static_cast<int>(GetHandle()->GetLayer());
}

void SetLayerLua(int v)
{
	GetHandle()->SetLayer(static_cast<ENTITY_LAYER>(v));
}
SCRIPT_GENERATE_COMP_WRAPPER_END()

// EntityReferenceHolderComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(EntityReferenceHolderComponent)
ecs::EntityHandle GetEntityReference(int index)
{
	return GetHandle()->GetEntity(index);
}
void SetEntityReference(int index, ecs::EntityHandle entity)
{
	GetHandle()->SetEntity(index, entity);
}
int GetSize()
{
	return GetHandle()->GetSize();
}
SCRIPT_GENERATE_COMP_WRAPPER_END()

// SpriteComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(SpriteComponent)
	SCRIPT_GENERATE_PROPERTY_FUNCS(Vec4, GetColor, SetColor)
SCRIPT_GENERATE_COMP_WRAPPER_END()

// ScriptComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(ScriptComponent)
void CallScriptFunction(std::string funcName)
{
	GetHandle()->CallScriptFunction(funcName);
}
SCRIPT_GENERATE_COMP_WRAPPER_END()

// RenderComponent
SCRIPT_GENERATE_COMP_WRAPPER_BEGIN(RenderComponent)
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

bool GetButtonDown(std::string name)
{
	if (auto action{ ST<MagicInput>::Get()->GetAction<bool>(name) })
		return action->GetValue();
}

float GetAxis(std::string name)
{
	if (auto action{ ST<MagicInput>::Get()->GetAction<float>(name) })
		return action->GetValue();
}

Vec2 Get2DAxis(std::string name)
{
	if (auto action{ ST<MagicInput>::Get()->GetAction<Vec2>(name) })
		return action->GetValue();
}

void Lua_PlayAudio(std::string name,bool looping)
{
	ST<AudioManager>::Get()->PlaySound(util::GenHash(name), looping);
}
void Lua_PlayAudio3D(std::string name, bool looping, Vec3 position)
{
	ST<AudioManager>::Get()->PlaySound3D(util::GenHash(name), looping, position);
}
ecs::EntityHandle Lua_LoadPrefab(std::string name)
{
	return ST<PrefabManager>::Get()->LoadPrefab(name);
}

float Lua_RandomRangeFloat(float min, float max)
{
	return randomFloat(min, max);
}
int Lua_RandomRangeInt(int min, int max)
{
	return randomRange(min, max);
}
Vec3 Lua_RandomRangeVec(Vec3 min, Vec3 max)
{
	return randomVec(min, max);
}

bool Lua_NumberedDiceRoll(int sides)
{
	if (sides <= 0)
		return true;
	return randomRange(0, sides)==0;
}

void RegisterCppStuffToLua(luabridge::Namespace baseTable)
{
	// Reference for how to do stuff: https://kunitoki.github.io/LuaBridge3/Manual

	baseTable
		// ----- CLASSES -----
		.beginClass<Vec2>("Vec2")
		.addConstructor<void(*)(float, float)>()
		.addProperty("x", [](const Vec2* v) -> float { return v->x; }, [](Vec2* v, float x) { v->x = x; })
		.addProperty("y", [](const Vec2* v) -> float { return v->y; }, [](Vec2* v, float y) { v->y = y; })
		.addFunction("Normalized", [](const Vec2* v) -> Vec2 {return v->Normalized(); })
		.addFunction("Length", [](const Vec2* v) -> float {return v->Length(); })
		.addFunction("LengthSqr", [](const Vec2* v) -> float {return v->LengthSqr(); })
		.addFunction("Direction", [](const Vec2* a, const Vec2* b) -> Vec2 { return (*b) - (*a);	})
		.addFunction("Add", [](const Vec2* a, const Vec2* b) -> Vec2 { return (*a) + (*b);	})
		.addFunction("Subtract", [](const Vec2* a, const Vec2* b) -> Vec2 { return (*a) - (*b);	})
		.addFunction("Dot", [](const Vec2* a, const Vec2* b) -> float { return a->Dot(*b);	})
		.addFunction("Cross", [](const Vec2* a, const Vec2* b) -> Vec2 { return (*a) * (*b);	})
		.endClass()

		.beginClass<Vec3>("Vec3")
		.addConstructor<void(*)(float, float, float)>()
		.addProperty("x", [](const Vec3* v) -> float { return v->x; }, [](Vec3* v, float x) { v->x = x; })
		.addProperty("y", [](const Vec3* v) -> float { return v->y; }, [](Vec3* v, float y) { v->y = y; })
		.addProperty("z", [](const Vec3* v) -> float { return v->z; }, [](Vec3* v, float z) { v->z = z; })
		.addFunction("Normalized", [](const Vec3* v) -> Vec3 {return v->Normalized(); })
		.addFunction("Length", [](const Vec3* v) -> float {return v->Length(); })
		.addFunction("LengthSqr", [](const Vec3* v) -> float {return v->LengthSqr(); })
		.addFunction("Direction", [](const Vec3* a, const Vec3* b) -> Vec3 { return (*b) - (*a);	})
		.addFunction("Add", [](const Vec3* a, const Vec3* b) -> Vec3 { return (*a) + (*b);	})
		.addFunction("Subtract", [](const Vec3* a, const Vec3* b) -> Vec3 { return (*a) - (*b);	})
		.addFunction("Dot", [](const Vec3* a, const Vec3* b) -> float { return a->Dot(*b);	})
		.addFunction("Cross", [](const Vec3* a, const Vec3* b) -> Vec3 { return (*a)*(*b);	})
		.addFunction("Scale", [](const Vec3* a, const float b) -> Vec3 { return (*a)*b;	})
		//.addFunction("Scale", [](const Vec3* a, const float* s) -> Vec3 { return (*a) * (*s);	})
			//.addFunction("DirectionEntities", [](const ecs::EntityHandle a, const ecs::EntityHandle b) -> Vec3 { if (!a||!b) return Vec3{ 0 };  return  b->GetTransform().GetWorldPosition() - a->GetTransform().GetWorldPosition() ;	})
		.endClass()

		.beginClass<Vec4>("Vec4")
			.addConstructor<void(*)(float, float, float, float)>()
			.addProperty("x", [](const Vec4* v) -> float { return v->x; }, [](Vec4* v, float x) { v->x = x; })
			.addProperty("y", [](const Vec4* v) -> float { return v->y; }, [](Vec4* v, float y) { v->y = y; })
			.addProperty("z", [](const Vec4* v) -> float { return v->z; }, [](Vec4* v, float z) { v->z = z; })
			.addProperty("w", [](const Vec4* v) -> float { return v->w; }, [](Vec4* v, float w) { v->w = w; })
		.endClass()
		.beginClass<Transform>("Transform")
		.addProperty("localPosition", [](const Transform* t) -> Vec3 { return t->GetLocalPosition(); }, [](Transform* t, Vec3 v) { t->SetLocalPosition(v); })
		.addProperty("localRotation", [](const Transform* t) -> Vec3 { return t->GetLocalRotation(); }, [](Transform* t, Vec3 v) { t->SetLocalRotation(v); })
		.addProperty("localScale", [](const Transform* t) -> Vec3 { return t->GetLocalScale();    }, [](Transform* t, Vec3 v) { t->SetLocalScale(v);    })
		.addProperty("worldPosition", [](const Transform* t) -> Vec3 { return t->GetWorldPosition(); }, [](Transform* t, Vec3 v) { t->SetWorldPosition(v); })
		.addProperty("worldRotation", [](const Transform* t) -> Vec3 { return t->GetWorldRotation(); }, [](Transform* t, Vec3 v) { t->SetWorldRotation(v); })
		.addProperty("worldScale", [](const Transform* t) -> Vec3 { return t->GetWorldScale();    }, [](Transform* t, Vec3 v) { t->SetWorldScale(v);    })			// Parent
		// Child
		.endClass()
		.beginClass<ecs::Entity>("Entity")
		.addProperty("transform", [](ecs::EntityHandle entity) -> Transform* { return &entity->GetTransform(); })
		.addFunction("Destroy", [](ecs::EntityHandle entity) -> void { ecs::DeleteEntity(entity); })
		
		//=========================================== START REGISTER GETTER ================================================================================

		// This section allows lua to get components
		SCRIPT_REGISTER_COMP_GETTER(NameComponent)  // syntax e.g. - local nameComp = entity:GetNameComponent(); if nameComp:Exists() then ...;
		SCRIPT_REGISTER_COMP_GETTER(AudioSourceComponent)
		SCRIPT_REGISTER_COMP_GETTER(CameraComponent)
		SCRIPT_REGISTER_COMP_GETTER(AnchorToCameraComponent)
		SCRIPT_REGISTER_COMP_GETTER(ShakeComponent)
		SCRIPT_REGISTER_COMP_GETTER(LightComponent)
		SCRIPT_REGISTER_COMP_GETTER(LightBlinkComponent)
		SCRIPT_REGISTER_COMP_GETTER(BoxColliderComp)
		SCRIPT_REGISTER_COMP_GETTER(PhysicsComp)
		SCRIPT_REGISTER_COMP_GETTER(GameCameraControllerComponent)
		SCRIPT_REGISTER_COMP_GETTER(CharacterMovementComponent)
		SCRIPT_REGISTER_COMP_GETTER(PlayerMovementComponent)
		SCRIPT_REGISTER_COMP_GETTER(GrabbableItemComponent)
		SCRIPT_REGISTER_COMP_GETTER(HealthComponent)
		SCRIPT_REGISTER_COMP_GETTER(EntityLayerComponent)
		SCRIPT_REGISTER_COMP_GETTER(EntityReferenceHolderComponent)
		SCRIPT_REGISTER_COMP_GETTER(SpriteComponent)
		SCRIPT_REGISTER_COMP_GETTER(ScriptComponent)
		SCRIPT_REGISTER_COMP_GETTER(RenderComponent)
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

		// BoxColliderComponent
		SCRIPT_REGISTER_COMP_BEGIN(BoxColliderComp)
		SCRIPT_REGISTER_COMP_PROPERTY(BoxColliderComp, "center", GetCenter, SetCenter)
		SCRIPT_REGISTER_COMP_PROPERTY(BoxColliderComp, "size", GetSize, SetSize)
		SCRIPT_REGISTER_COMP_PROPERTY(BoxColliderComp, "enabled", GetEnabled, SetEnabled)
		SCRIPT_REGISTER_COMP_END()

		// PhysicsComp (Slightly different in how takumi use it in his component but is same functions)
		SCRIPT_REGISTER_COMP_BEGIN(PhysicsComp)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "linearVelocity", GetLinearVelocity, SetLinearVelocity)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "angularVelocity", GetAngularVelocity, SetAngularVelocity)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "isKinematic", GetIsKinematic, SetIsKinematic)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "useGravity", GetUseGravity, SetUseGravity)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "lockRotationX", GetRotLockedX, SetRotLockedX)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "lockRotationY", GetRotLockedY, SetRotLockedY)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "lockRotationZ", GetRotLockedZ, SetRotLockedZ)
		SCRIPT_REGISTER_COMP_PROPERTY(PhysicsComp, "enabled", GetEnabled, SetEnabled)
		SCRIPT_REGISTER_COMP_FUNCTION(PhysicsComp, "AddLinearVelocity", AddLinearVelocity)
		SCRIPT_REGISTER_COMP_END()

		// GameCameraControllerComponent
		SCRIPT_REGISTER_COMP_BEGIN(GameCameraControllerComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(GameCameraControllerComponent,"cameraAutoZoomSpeed", GetCameraAutoZoomSpeed, SetCameraAutoZoomSpeed)
		SCRIPT_REGISTER_COMP_PROPERTY(GameCameraControllerComponent,"targetCameraDistance", GetTargetCameraDistance, SetTargetCameraDistance)
		SCRIPT_REGISTER_COMP_PROPERTY(GameCameraControllerComponent,"currentCameraDistance", GetCurrentCameraDistance, SetCurrentCameraDistance)
		SCRIPT_REGISTER_COMP_PROPERTY(GameCameraControllerComponent,"offsetPosition", GetOffsetPosition, SetOffsetPosition)
		SCRIPT_REGISTER_COMP_PROPERTY(GameCameraControllerComponent,"cameraSensitivity", GetCameraSensitivity, SetCameraSensitivity)
		SCRIPT_REGISTER_COMP_END()

		// CharacterMovementComponent
		SCRIPT_REGISTER_COMP_BEGIN(CharacterMovementComponent)
		// properties
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "movementVector", GetMovementVector, SetMovementVector)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "moveSpeed", GetMoveSpeed, SetMoveSpeed)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "rotateSpeed", GetRotateSpeed, SetRotateSpeed)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "stunTimePerHit", GetStunTimePerHit, SetStunTimePerHit)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "groundFriction", GetGroundFriction, SetGroundFriction)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "dodgeCooldown", GetDodgeCooldown, SetDodgeCooldown)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "dodgeDuration", GetDodgeDuration, SetDodgeDuration)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "dodgeSpeed", GetDodgeSpeed, SetDodgeSpeed)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "currentStunTime", GetCurrentStunTime, SetCurrentStunTime)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "currentDodgeTime", GetCurrentDodgeTime, SetCurrentDodgeTime)
		SCRIPT_REGISTER_COMP_PROPERTY(CharacterMovementComponent, "currentDodgeCooldown", GetCurrentDodgeCooldown, SetCurrentDodgeCooldown)
		// functions
		SCRIPT_REGISTER_COMP_FUNCTION(CharacterMovementComponent, "Dodge", Dodge)
		SCRIPT_REGISTER_COMP_FUNCTION(CharacterMovementComponent, "RotateTowards", RotateTowards)
		SCRIPT_REGISTER_COMP_FUNCTION(CharacterMovementComponent, "SetMovementVector", SetMovementVectorLua)
		SCRIPT_REGISTER_COMP_FUNCTION(CharacterMovementComponent, "DropItem", DropItem)
		SCRIPT_REGISTER_COMP_FUNCTION(CharacterMovementComponent, "Throw", Throw)
		SCRIPT_REGISTER_COMP_FUNCTION(CharacterMovementComponent, "Attack", Attack)
		SCRIPT_REGISTER_COMP_END()

		// PlayerMovementComponent
		SCRIPT_REGISTER_COMP_BEGIN(PlayerMovementComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(PlayerMovementComponent,"grabDistance", GetGrabDistance, SetGrabDistance)
		SCRIPT_REGISTER_COMP_END()

		// GrabbableItemComponent
		SCRIPT_REGISTER_COMP_BEGIN(GrabbableItemComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(GrabbableItemComponent,"damage", GetDamage, SetDamage)
		SCRIPT_REGISTER_COMP_PROPERTY(GrabbableItemComponent,"isHeld", GetIsHeld, SetIsHeld)
		SCRIPT_REGISTER_COMP_FUNCTION(GrabbableItemComponent,"Attack", AttackLua)
		SCRIPT_REGISTER_COMP_END()

		// HealthComponent
		SCRIPT_REGISTER_COMP_BEGIN(HealthComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(HealthComponent,"health", GetCurrHealth, SetHealth)
		SCRIPT_REGISTER_COMP_PROPERTY(HealthComponent,"maxHealth", GetMaxHealth, SetMaxHealth)
		SCRIPT_REGISTER_COMP_FUNCTION(HealthComponent,"AddHealth", AddHealthLua)
		SCRIPT_REGISTER_COMP_FUNCTION(HealthComponent,"TakeDamage", TakeDamageLua)
		SCRIPT_REGISTER_COMP_FUNCTION(HealthComponent,"IsDead", IsDead)
		SCRIPT_REGISTER_COMP_FUNCTION(HealthComponent,"GetHealthFraction", GetHealthFractionLua)
		SCRIPT_REGISTER_COMP_END()

		// EntityLayerComponent
		SCRIPT_REGISTER_COMP_BEGIN(EntityLayerComponent)
		SCRIPT_REGISTER_COMP_PROPERTY(EntityLayerComponent,"layer", GetLayerLua, SetLayerLua)
		SCRIPT_REGISTER_COMP_END()

		// EntityReferenceHolderComponent
		SCRIPT_REGISTER_COMP_BEGIN(EntityReferenceHolderComponent)
			.addFunction("GetEntityReference", &LuaWrapperComp_EntityReferenceHolderComponent::GetEntityReference)
			.addFunction("SetEntityReference", &LuaWrapperComp_EntityReferenceHolderComponent::SetEntityReference)
			.addFunction("GetSize", &LuaWrapperComp_EntityReferenceHolderComponent::GetSize)
			//.addProperty("GetEntityReference", [](const EntityReferenceHolderComponent* comp) -> EntityReference { return comp->GetEntity() })
		SCRIPT_REGISTER_COMP_END()

		// SpriteComponent
		SCRIPT_REGISTER_COMP_BEGIN(SpriteComponent)
			SCRIPT_REGISTER_COMP_PROPERTY(SpriteComponent, "color", GetColor, SetColor)
		SCRIPT_REGISTER_COMP_END()
		// ScriptComponent
		SCRIPT_REGISTER_COMP_BEGIN(ScriptComponent)
			.addFunction("CallScriptFunction", &LuaWrapperComp_ScriptComponent::CallScriptFunction)
		SCRIPT_REGISTER_COMP_END()
		// RenderComponent
		SCRIPT_REGISTER_COMP_BEGIN(RenderComponent)
		SCRIPT_REGISTER_COMP_END()

		// ----- GLOBAL FUNCTIONS -----
		.addFunction("Log", Lua_Log)
		.addFunction("DeltaTime", []() -> float { return GameTime::Dt(); })
		.addFunction("LoadScene", [](const std::string& scenePath) {
			auto* sceneManager = ST<SceneManager>::Get();

			// Get current scene index before loading new one
			Scene* currentScene = sceneManager->GetActiveScene();
			int oldSceneIndex = currentScene ? currentScene->GetIndex() : -1;

			// Load new scene and set it as active
			int newSceneIndex = sceneManager->LoadScene(scenePath, true);

			// Defer unloading the old scene to avoid destroying the calling script mid-execution
			if (oldSceneIndex >= 0 && newSceneIndex >= 0) {
				ST<Scheduler>::Get()->Add([oldSceneIndex]() {
					ST<SceneManager>::Get()->UnloadScene(oldSceneIndex);
				});
			}
		})
		.addFunction("GetButtonDown", GetButtonDown)

		.beginNamespace("AudioManager")
			.addFunction("PlaySound", Lua_PlayAudio)
			.addFunction("PlaySound3D", Lua_PlayAudio3D)
		.endNamespace()

		.beginNamespace("PrefabManager")
			.addFunction("LoadPrefab", Lua_LoadPrefab)
		.endNamespace()

		.beginNamespace("Random")
			.addFunction("RangeFloat", Lua_RandomRangeFloat)
			.addFunction("RangeInt", Lua_RandomRangeInt)
			.addFunction("DiceRoll", Lua_NumberedDiceRoll)
			.addFunction("RangeVec3", Lua_RandomRangeVec)
		.endNamespace()


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
		.endNamespace()
		.beginNamespace("EntityLayer")
			.addVariable("Default", static_cast<int>(ENTITY_LAYER::DEFAULT))
			.addVariable("Environment", static_cast<int>(ENTITY_LAYER::ENVIRONMENT))
			.addVariable("Player", static_cast<int>(ENTITY_LAYER::PLAYER))
			.addVariable("Enemy", static_cast<int>(ENTITY_LAYER::ENEMY))
		.endNamespace();

	
}
