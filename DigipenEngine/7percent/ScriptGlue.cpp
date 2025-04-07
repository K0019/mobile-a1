/******************************************************************************/
/*!
\file   ScriptGlue.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
  This contains the declarations and definitions of the internal call functions are.
  They are functions that can be called/invoked from the C# side.(Check InternalCalls.cs
  to see their C# counterpart they are linked to).

  This also contains the definition of the class ScriptGlue and its one static function
  Register Functions

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "ScriptGlue.h"
#include "TextComponent.h"
#include "Physics.h"
#include "Collision.h"
#include "AudioManager.h"
#include "ScriptComponent.h"
#include "NameComponent.h"
#include "CameraComponent.h"
#include "AnimatorComponent.h"
#include "RenderComponent.h"
#include "CameraController.h"
#include "PrefabManager.h"

#pragma region DummyContainers
/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyTransform
{
	Vector2 position;
	Vector2 localPosition;
	Vector2 scale;
	Vector2 localScale;
	float rotation;
	float localRotation;
	float zPosition;

	uint64_t eid; // This is just so C# side works :>

	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] p
		position of the transform to transfer over
	\param[in] s
		scale of the transform to transfer over
	\param[in] r
		rotation of the transform to transfer over
	\param[in] z
		zPos of the transform to transfer over
	\param[in] id
		Entitle handle of the entity the transform is attached to
		to transfer over
	\return
		None
	*//******************************************************************/
	DummyTransform(Vector2 p, Vector2 lp,Vector2 s, Vector2 ls, float r, float lr, float z, uint64_t id)
	{
		position = p;
		localPosition = lp;
		scale = s;
		localScale = ls;
		rotation = r;
		localRotation = lr;
		zPosition = z;
		eid = id;
	}
};

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyText
{
	Vector4 color;
	MonoString* text;
	uint64_t eid;

	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] c
		Color of the text to transfer over
	\param[in] id
		Entitle handle of the entity the transform is attached to
		to transfer over
	\return
		None
	*//******************************************************************/
	DummyText(Vector4 c, MonoString* t, uint64_t id)
	{
		color = c;
		text = t;
		eid = id;
	}
};

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyPhysics 
{
	float mass;
	//float restitutionCoeff;
	float frictionCoeff;
	Vector2 velocity;
	float angVelocity;

	uint64_t eid;

	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] m
		Mass of the entity to transfer over
	\param[in] fc
		Friction Coefficient of the entity to transfer over
	\param[in] vel
		Velocity of the entity to transfer over
	\param[in] aVel
		Angular velocity to transfer over
	\param[in] id
		Entitle handle of the entity the component is attached to
		to transfer over
	\return
		None
	*//******************************************************************/
	DummyPhysics(float m,/* float rc,*/ float fc, Vector2 vel, float aVel, uint64_t id)
	{
		mass = m;
		//restitutionCoeff = rc;
		frictionCoeff = fc;
		velocity = vel;
		angVelocity = aVel;

		eid = id;
	}
};

struct DummyEntity 
{
	///Addcomp, removecomp, GetComponent
};

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyRaycastHit
{
	DummyRaycastHit(Physics::RaycastResult raycastResult)
	{
		distance = raycastResult.distance;
		point = raycastResult.collisionPoint;
		//Collider Component
		if(raycastResult.hitComp !=nullptr)
		{
			entity = reinterpret_cast<uint64_t>(ecs::GetEntity(raycastResult.hitComp));
		}
		else
		{
			entity = 0;
		}
	}
	float distance;
	Vector2 point;
	uint64_t entity;
};

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyCamera
{
	bool active;
	int priority;
	float zoom;

	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] z
		Zoom of the camera to transfer over
	\param[in] a
		Active bool of the camera to transfer over
	\param[in] p
		Priority of the Camera to transfer over
	\return
		None
	*//******************************************************************/
	DummyCamera(float z, bool a, int p)
	{
		zoom = z;
		active = a;
		priority = p;
	}
};

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyAnimator
{
	uint64_t eid;
	
	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] id
		Entitle handle of the entity the component is attached to
		to transfer over
	\return
		None
	*//******************************************************************/
	DummyAnimator(uint64_t id)
	{
		eid = id;
	}
};

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyCollisionData
{
	Vector2 normal;
	Vector2 point;
	float depth;

	uint64_t eid;
	uint64_t other;

	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] m
		Mass of the entity to transfer over
	\param[in] fc
		Friction Coefficient of the entity to transfer over
	\param[in] vel
		Velocity of the entity to transfer over
	\param[in] aVel
		Angular velocity to transfer over
	\param[in] id
		Entitle handle of the entity the component is attached to
		to transfer over
	\return
		None
	*//******************************************************************/
	DummyCollisionData(Physics::CollisionEventData& collisionData)
	{
		normal = collisionData.collisionData->collisionNormal;
		point = collisionData.collisionData->collisionPoint;
		depth = collisionData.collisionData->penetrationDepth;
		
		ecs::EntityHandle thisEntity = ecs::GetEntity(collisionData.refComp);
		ecs::EntityHandle otherEntity = ecs::GetEntity(collisionData.otherComp);
		// DummyEntity maybe?
		eid = reinterpret_cast<uint64_t>(thisEntity);
		other = reinterpret_cast<uint64_t>(otherEntity);
	}
};

#pragma endregion


namespace CSharpScripts
{
	// Should be same name as in both c++ and c#
#define ADD_INTERNAL_CALL(name) mono_add_internal_call("EngineScripting.InternalCalls::"#name, name)


	/*****************************************************************//*!
	\brief
		Helper function to convert a MonoString* to a string.
	\param[in] mString
		MonoString* to convert to a string.
	\return
		Converted string of the MonoString*.
	*//******************************************************************/
	static std::string MonoStringToString(MonoString* mString)
	{
		char* cStr = mono_string_to_utf8(mString);
		std::string result(cStr);

		mono_free(cStr);
		return result;
	}

	/*****************************************************************//*!
	\brief
		Helper function to convert a string to a MonoString*.
	\param[in] str
		string to conver to a MonoString*.
	\return
		Converted MonoString* of the string.
	*//******************************************************************/
	static MonoString* StringToMonoString(const std::string& str)
	{
		const char* cStr = str.c_str();

		return mono_string_new(mono_domain_get(), cStr);
	}
#pragma region TestCalls

	/*****************************************************************//*!	
	\brief
		Function to test a logging function call from c# side.
	\param[in] messageStr
		Message from C# side to log to console
	\param[in] valueParam
		int value crom C# side transferred to c++ to add to the log
	\return
		None
	*//******************************************************************/
	static void CppNativeLog(MonoString* messageStr, int valueParam)
	{
		char* cStr = mono_string_to_utf8(messageStr);
		std::string str(cStr);

		mono_free(cStr);
		CONSOLE_LOG_EXPLICIT("This Cpp function was called in c# with a message: " + str + "and a value:" + std::to_string(valueParam), LogLevel::LEVEL_DEBUG);

	}

/*	static void CppNativeLog_Vector(glm::vec3* param, glm::vec3* outResult)
	{

		std::string str = "Logging glm::vec3 param with values(" + std::to_string(param->x)
			+ ", " + std::to_string(param->y)
			+ ", " + std::to_string(param->z)
			+ ")";
		CONSOLE_LOG_EXPLICIT(str, LogLevel::LEVEL_DEBUG);

		*outResult = glm::normalize(*param);

	}*/	
	/*****************************************************************//*!
	\brief
		Function to test transfer of structs over from c# to c++
	\param[in] param
		Vector3 originating from c# side to test transfer of 
		struct data.
	\param[in, out] outResult
		Vector3 originating from c# side to manipulate in this function
	\return
		None
	*//******************************************************************/
	static void CppNativeLog_Vector(Vector3* param, Vector3* outResult)
	{

		std::string str = "Logging glm::vec3 param with values(" + std::to_string(param->x)
			+ ", " + std::to_string(param->y)
			+ ", " + std::to_string(param->z)
			+ ")";
		CONSOLE_LOG_EXPLICIT(str, LogLevel::LEVEL_DEBUG);

		*outResult = *param;
		*outResult = outResult->Normalize();

	}

	/*****************************************************************//*!
	\brief
		Function to test return values when called from c# side.
	\param[in] param
		Vector3 originating from c# side to test transfer of
		struct data.
	\return
		returns a float derived from c++ side to transfer to c#
	*//******************************************************************/
	static float CppNativeLog_VectorDot(Vector3* param)
	{
		std::string str = "Execute Dot product of Vector3 param with values(" + std::to_string(param->x)
			+ ", " + std::to_string(param->y)
			+ ", " + std::to_string(param->z)
			+ ")";
		return param->Dot(*param);

	}

	/*****************************************************************//*!
\brief
	Function call from c# side to move the entity the c# script is attached
	to
\param[in] entityHandle
	The unique id of the entity to move
\param[in] unitsMoved
	How many units to move the entity by
\return
	None.
*//******************************************************************/
	static void MoveRight(uint64_t entityHandle, float unitsMoved)
	{
		CONSOLE_LOG_EXPLICIT("Handle passed: " + std::to_string(entityHandle), LogLevel::LEVEL_DEBUG);
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().AddLocalPosition(Vector2(unitsMoved, 0));
	}

	/*****************************************************************//*!
	\brief
		Function call from c# side to change the colour of the text component of
		the entity the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity to manipulate the text component of
	\param[in] dt
		delta time
	\return
		None.
	*//******************************************************************/
	static void ChangeColour(uint64_t entityHandle, float dt)
	{
		ecs::CompHandle<TextComponent> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>();
		if (t == nullptr)
		{
			return;
		}
		Vector4 c = t->GetColor();
		c.x = c.x + dt > 1.f ? 0.f : c.x + dt;
		c.y = c.y - dt < 0.f ? 1.f : c.y - dt;
		c.z = c.z + dt * 2 > 1.f ? 0.f : c.z + dt * 2;

		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>()->SetColor(c);
	}

#pragma endregion

#pragma region Time

	static float GetDeltaTime()
	{
		return GameTime::Dt();
	}

#pragma endregion

#pragma region Input
	// Add more key presses
	/*****************************************************************//*!
	\brief
		Function to Get a keypress from c++ side
	\return
		returns the char of the key pressed
	*//******************************************************************/
	static char GetKeyPress() 
	{
		// Loop to check for a key press
		for (int i = 0; i < 256; ++i) {
			// Check if the key is currently pressed
			if (GetAsyncKeyState(i) & 0x8000) {
				// Return uppercase for alphabetic characters
				if (i >= 'a' && i <= 'z') {
					return static_cast<char>(i - 32); // Convert to uppercase
				}
				else if (i >= 'A' && i <= 'Z') {
					return static_cast<char>(i); // Already uppercase
				}
				return static_cast<char>(i); // Return other keys as they are
			}
		}
		return '\0'; // Return null character if no key is pressed
	}


	/*****************************************************************//*!
	\brief
		Checks if the specified key is being pressed.
	\param[in] keycode
		Key to check.
	\return
		bool of checked key is being pressed.
	*//******************************************************************/
	static bool GetCurrKey(int keycode)
	{
		return Input::GetKeyCurr(KEY(keycode));
	}

	/*****************************************************************//*!
	\brief
		Checks if the specified key has been pressed.
	\param[in] keycode
		Key to check.
	\return
		bool of checked key has been pressed.
	*//******************************************************************/
	static bool GetKeyPressed(int keycode)
	{
		return Input::GetKeyPressed(KEY(keycode));
	}

	/*****************************************************************//*!
	\brief
		Checks if the specified key has been released.
	\param[in] keycode
		Key to check.
	\return
		bool of checked key has been released.
	*//******************************************************************/
	static bool GetKeyReleased(int keycode)
	{
		return Input::GetKeyReleased(KEY(keycode));
	}

	/*****************************************************************//*!
	\brief
		Returns the world position of the mouse.
	\param[in] pos
		Pointer of a Vector2 to contain the mouse position.
	\return
		None.
	*//******************************************************************/
	static void GetMouseWorldPos(Vector2* pos)
	{
		*pos = Input::GetMousePosWorld();
	}

#pragma endregion

#pragma region TransformComponent

	/*****************************************************************//*!
	\brief
		Function to retrieve the Transform data of the entity the script
		is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] out
		Pointer to the Transform component found inside the c# script to
		transfer the Transform data to.
	\return
		None.
	*//******************************************************************/
	static void GetTransform(uint64_t entityHandle, DummyTransform* out)
	{
		//CONSOLE_LOG_EXPLICIT("Handle passed: " + std::to_string(entityHandle), LogLevel::LEVEL_DEBUG);

		//Guard clause for invalid entityHandles
		if (!ecs::IsEntityHandleValid(reinterpret_cast<ecs::EntityHandle>(entityHandle)))
		{
			CONSOLE_LOG_EXPLICIT("INVALID ENTITY HANDLE: "+std::to_string(entityHandle), LogLevel::LEVEL_ERROR);
			return;
		}
		Transform t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform();

		*out = DummyTransform(t.GetWorldPosition(), t.GetLocalPosition(),t.GetWorldScale(), t.GetLocalScale(),t.GetWorldRotation(), t.GetLocalRotation(), t.GetZPos(), entityHandle);
	}

	/*****************************************************************//*!
	\brief
		Function to retrieve the Transform data of the entity the script
		is attached to
	\param[in] entityHandle
		Id of the entity.
	\param[in, out] out
		Pointer to the Transform component found inside the c# script to
		transfer the Transform data to.
	\return
		None.
	*//******************************************************************/
	//void GetChildTransform(uint64_t entityHandle, DummyTransform* out)
	//{
	//	ecs::EntityHandle e = reinterpret_cast<ecs::EntityHandle>(entityHandle);
	//	if (e->GetTransform().GetChildren().empty())
	//	{
	//		CONSOLE_LOG_EXPLICIT("Entity does not have any children!", LogLevel::LEVEL_ERROR);
	//		return;
	//	}

	//	// Need to recursive this
	//	for (const auto& child : e->GetTransform().GetChildren())
	//	{
	//		Transform t = child->GetEntity()->GetTransform();
	//		uint64_t id = reinterpret_cast<uint64_t>(child->GetEntity());
	//		*out = DummyTransform(t.GetWorldPosition(), t.GetLocalPosition(), t.GetWorldScale(), t.GetLocalScale(), t.GetWorldRotation(), t.GetLocalRotation(), t.GetZPos(), id);
	//		return;
	//	}
	//}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's local position that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		Pointer to the updated position of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void SetTransformLocalPos(uint64_t entityHandle, Vector2* value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetLocalPosition(*value);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's local position that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] targetVec2
		Pointer to the position of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformLocalPos(uint64_t entityHandle, Vector2* targetVec2)
	{
		*targetVec2 = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetLocalPosition();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's world position that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		Pointer to the updated position of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void SetTransformWorldPos(uint64_t entityHandle, Vector2* value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetWorldPosition(*value);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's world position that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] targetVec2
		Pointer to the position of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformWorldPos(uint64_t entityHandle, Vector2* targetVec2)
	{
		*targetVec2 = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetWorldPosition();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's local scale that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		Pointer to the scale of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void SetTransformLocalScale(uint64_t entityHandle, Vector2* value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetLocalScale(*value);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's local scale that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] targetVec2
		Pointer to the scale of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformLocalScale(uint64_t entityHandle, Vector2* targetVec2)
	{
		*targetVec2 = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetLocalScale();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's world scale that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		Pointer to the scale of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void SetTransformWorldScale(uint64_t entityHandle, Vector2* value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetWorldScale(*value);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's world scale that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] targetVec2
		Pointer to the scale of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformWorldScale(uint64_t entityHandle, Vector2* targetVec2)
	{
		*targetVec2 = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetWorldScale();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's local rotation that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		New value of the entity's rotation.
	\return
		None.
	*//******************************************************************/
	static void SetTransformLocalRotate(uint64_t entityHandle, float value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetLocalRotation(value);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's local rotation that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] rot
		Pointer to the rotation of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformLocalRotate(uint64_t entityHandle, float* rot)
	{
		*rot = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetLocalRotation();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's world rotation that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		New value of the entity's rotation.
	\return
		None.
	*//******************************************************************/
	static void SetTransformWorldRotate(uint64_t entityHandle, float value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetWorldRotation(value);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's world rotation that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] rot
		Pointer to the rotation of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformWorldRotate(uint64_t entityHandle, float* rot)
	{
		*rot = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetWorldRotation();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's Z Position that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] value
		New value of the entity's zPos.
	\return
		None.
	*//******************************************************************/
	static void SetTransformZPos(uint64_t entityHandle, float value)
	{
		reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().SetZPos(value);
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's Z Position that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] zPos
		Pointer to the zPos of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTransformZPos(uint64_t entityHandle, float* zPos)
	{
		*zPos = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetTransform().GetZPos();
	}

#pragma endregion

#pragma region PhysicsComponent

	/*****************************************************************//*!
	\brief
		Function to Get the Physics component that's attached to the entity.
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] out
		Pointer to the Physics component found inside the c# script to
		transfer the Physics data to.
	\return
		None.
	*//******************************************************************/
	static void GetPhysicsComp(uint64_t entityHandle, DummyPhysics* out)
	{
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		*out = DummyPhysics(t->GetMass(), t->GetFrictionCoeff(), t->GetVelocity(), t->GetAngVelocity(), entityHandle);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's Mass that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] mass
		Pointer to the mass of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetPhysicsMass(uint64_t entityHandle, float* mass)
	{
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		*mass = t->GetMass();
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's Friction that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] friction
		Pointer to the friction of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetPhysicsFriction(uint64_t entityHandle, float* friction)
	{
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		*friction = t->GetFrictionCoeff();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's Velocity that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] vel
		Pointer to the velocity of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void SetPhysicsVelocity(uint64_t entityHandle, Vector2* vel)
	{
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		t->SetVelocity(*vel);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's velocity that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] targetVel
		Pointer to the velocity of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetPhysicsVelocity(uint64_t entityHandle, Vector2* targetVel)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		*targetVel = t->GetVelocity();
	}


	/*****************************************************************//*!
	\brief
		Function to Set the entity's Angular Velocity that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] aVel
		New value of the entity's angular velocity.
	\return
		None.
	*//******************************************************************/
	static void SetPhysicsAngularVelocity(uint64_t entityHandle, float* aVel)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		t->SetAngVelocity(*aVel);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's Angular Velocity that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] targetAVel
		Pointer to the angular velocity of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetPhysicsAngularVelocity(uint64_t entityHandle, float* targetAVel)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<Physics::PhysicsComp> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<Physics::PhysicsComp>();
		if (t == nullptr)
		{
			// add a warning/error here
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		*targetAVel = t->GetAngVelocity();
	}

	/*****************************************************************//*!
	\brief
		Function to call a C++ Raycast and return the result.
	\param[in] origin
		The point to raycast from.
	\param[in] direction
		The dierction to raycast in.
	\param[in] layerMask
		The layer masks to test.
	\param[out] hit
		The result of the raycast.
	\return
		None
	*//******************************************************************/
	static void Raycast(Vector2 origin, Vector2 direction, int layerMask, DummyRaycastHit* hit)
	{
		EntityLayersMask mask(layerMask);
		Physics::RaycastResult raycastResult;
		Physics::Raycast(origin, direction, mask, &raycastResult);
		*hit = DummyRaycastHit(raycastResult);
	}
#pragma endregion

#pragma region TextComponent
	/*****************************************************************//*!
\brief
	Function to retrieve the Text data of the entity the script
	is attached to
\param[in] entityHandle
	The unique id of the entity the script is attached to
\param[in, out] out
	Pointer to the Text component found inside the c# script to
	transfer the Text data to.
\return
	None.
*//******************************************************************/
	static void GetText(uint64_t entityHandle, DummyText* out)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<TextComponent> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>();
		if (t == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		*out = DummyText(t->GetColor(), StringToMonoString(t->GetText()), entityHandle);
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's TextComponent text color that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] color
		Pointer to the updated color of the text in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void SetTextColor(uint64_t entityHandle, Vector4* color)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<TextComponent> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>();
		if (t == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		t->SetColor(*color);
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's Text Color that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] color
		Pointer to the color of the entity in the c# side inside the script.
	\return
		None.
	*//******************************************************************/
	static void GetTextColor(uint64_t entityHandle, Vector4* color)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<TextComponent> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>();
		if (t == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		*color = t->GetColor();
	}

	/*****************************************************************//*!
	\brief
		Function to Set the entity's Text string that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] message
		New string text of the entity.
	\return
		None.
	*//******************************************************************/
	static void SetTextString(uint64_t entityHandle, MonoString* message)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<TextComponent> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>();
		if (t == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		t->SetText(MonoStringToString(message));
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's Text string that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\return
		MonoString* of the text string.
	*//******************************************************************/
	static MonoString* GetTextString(uint64_t entityHandle)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return nullptr;
		}
		ecs::CompHandle<TextComponent> t = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<TextComponent>();
		if (t == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return nullptr;
		}
		return StringToMonoString(t->GetText());
	}
#pragma endregion

#pragma region Logging

	/*****************************************************************//*!
	\brief
		Function to log messages to the console.
	\param[in] msg
		string that contains the message to log.
	\param[in] level
		Logging level of the message.
	\return
		None.
	*//******************************************************************/
	static void Log(MonoString* msg, int level)
	{
		CONSOLE_LOG_EXPLICIT(MonoStringToString(msg), static_cast<LogLevel>(level));
	}
#pragma endregion

#pragma region Entity Related

	/*****************************************************************//*!
	\brief
		Function to Get the entity's script instance that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] name
		Name of the script class to retrieve.
	\return
		MonoObject* instance of the script
	*//******************************************************************/
	static MonoObject* GetScriptInstance(uint64_t entityHandle, MonoString* name)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return nullptr;
		}
		ecs::CompHandle<ScriptComponent> comp = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<ScriptComponent>();
		if (comp == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified script!", LogLevel::LEVEL_ERROR);
			return nullptr;
		}
		return comp->FindScriptInstance(MonoStringToString(name));
	}

	/*****************************************************************//*!
	\brief
		Function to Get the entity's child's script instance that the c# script is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] name
		Name of the script class to retrieve.
	\return
		MonoObject* instance of the script
	*//******************************************************************/
	static MonoObject* GetChildScriptInstance(uint64_t entityHandle, MonoString* name)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return nullptr;
		}
		ecs::EntityHandle e = reinterpret_cast<ecs::EntityHandle>(entityHandle);
		std::string tempDebug = MonoStringToString(name);
		if (e->GetTransform().GetChildren().empty())
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have any children! [" + tempDebug + "]", LogLevel::LEVEL_ERROR);
			return nullptr;
		}

		// Need to recursive this
		MonoObject* script = nullptr;
		for (const auto& child : e->GetTransform().GetChildren())
		{
			ecs::CompHandle<ScriptComponent> comp = child->GetEntity()->GetComp<ScriptComponent>();
			if (comp != nullptr)
			{
				// Try to find the script in the current child
				script = comp->FindScriptInstance(tempDebug);
				if (script != nullptr)
				{
					return script;
				}
			}

			// If current child does not have it go to their children
			script = GetChildScriptInstance(reinterpret_cast<uint64_t>(child->GetEntity()), name);
			if (script != nullptr)
			{
				return script;
			}
		}

		CONSOLE_LOG_EXPLICIT("Entity does not have specified script! [" + tempDebug + "]", LogLevel::LEVEL_ERROR);
		return nullptr;
	}

	/*****************************************************************//*!
	\brief
		Function to find an entity via their name.
	\param[in] name
		Name of the entity to find.
	\return
		Entity handle of the found entity
	*//******************************************************************/
	static uint64_t FindEntity(MonoString* name)
	{
		std::string nameToFind = MonoStringToString(name);
		std::string currEntityName = "";
		uint64_t handle = 0;
		for (auto ite = ecs::GetCompsBegin<NameComponent>(); ite != ecs::GetCompsEnd<NameComponent>(); ++ite)
		{
			currEntityName = ite.GetEntity()->GetComp<NameComponent>()->GetName();
			if (std::strcmp(nameToFind.c_str(), currEntityName.c_str()) == 0)
			{
				handle = reinterpret_cast<uint64_t>(ite.GetEntity());
				break;
			}
		}
		return handle;
	}

	/*****************************************************************//*!
	\brief
		Function to destroy an entity via their entity handle
	\param[in] entityHandle
		The unique id of the entity to destroy.
	\return
		None.
	*//******************************************************************/
	static void DestroyEntity(uint64_t entityHandle)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Default entity handle detected! (0), entity may not exist!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::DeleteEntity(reinterpret_cast<ecs::EntityHandle>(entityHandle));
	}

	/*****************************************************************//*!
	\brief
		Function to create a copy of an entity inside c# scripts.
	\param[in] originalEntityHandle
		Entity handle of the entity to copy.
	\param[in] parentToAttach
		Entity handle of the entity to parent the copied entity.
	\return
		Entity handle of the newly created/copied entity
	*//******************************************************************/
	static uint64_t InstanstiateGameObject(uint64_t originalEntityHandle, uint64_t parentToAttach)
	{
		if (originalEntityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Default entity handle detected! (0), entity may not exist!", LogLevel::LEVEL_ERROR);

			return 0;
		}
		bool hasParent = parentToAttach == 0 ? false : true;
		ecs::EntityHandle e = reinterpret_cast<ecs::EntityHandle>(originalEntityHandle);
		ecs::EntityHandle clone = ecs::CloneEntity(e); // Clone immediately is not safe since we're iterating script components currently
		if (hasParent)
		{
			ecs::EntityHandle parent = reinterpret_cast<ecs::EntityHandle>(parentToAttach);
			clone->GetTransform().SetParent(parent->GetTransform());
		}
		return reinterpret_cast<uint64_t>(clone);
	}

	static uint64_t InstantiatePrefab(MonoString* prefabName, uint64_t parentToAttach)
	{
		std::string name = MonoStringToString(prefabName);
		bool hasParent = parentToAttach == 0 ? false : true;
		try
		{
			ecs::EntityHandle e = PrefabManager::LoadPrefab(name);
			if (hasParent && e != nullptr)
			{
				ecs::EntityHandle parent = reinterpret_cast<ecs::EntityHandle>(parentToAttach);
				e->GetTransform().SetParent(parent->GetTransform());
			}
			return reinterpret_cast<uint64_t>(e);
		}
		catch (...)
		{
			CONSOLE_LOG_EXPLICIT("Failed to load prefab named: " + name + "!", LogLevel::LEVEL_ERROR);
			return 0;
		}
	}
#pragma endregion

#pragma region Audio
	/*****************************************************************//*!
	\brief
		ctrl + click to check associated usage in AudioManager.h.
	*//******************************************************************/
	static void StartSingleSound(float volume, MonoString* name, bool loop)
	{
		ST<AudioManager>::Get()->StartSingleSound(MonoStringToString(name), loop, std::nullopt, volume);
	}

	// Spatial audio
	static void StartSingleSoundWithPosition(float volume, MonoString* name, bool loop, Vector2 position)
	{
		ST<AudioManager>::Get()->StartSingleSound(MonoStringToString(name), loop, position, volume);
	}

	/*****************************************************************//*!
	\brief
		ctrl + click to check associated usage in AudioManager.h.
	*//******************************************************************/
	static void StartGroupedSound(float volume, MonoString* baseName, bool loop)
	{
		ST<AudioManager>::Get()->StartGroupedSound(MonoStringToString(baseName), loop, std::nullopt, volume);
	}

	// Spatial audio
	static void StartGroupedSoundWithPosition(float volume, MonoString* baseName, bool loop, Vector2 position)
	{
		ST<AudioManager>::Get()->StartGroupedSound(MonoStringToString(baseName), loop, position, volume);
	}

	/*****************************************************************//*!
	\brief
		ctrl + click to check associated usage in AudioManager.h.
	*//******************************************************************/
	static void StopSound(MonoString* name)
	{
		ST<AudioManager>::Get()->StopSound(MonoStringToString(name));
	}

	/*****************************************************************//*!
	\brief
		ctrl + click to check associated usage in AudioManager.h.
	*//******************************************************************/
	static void StopAllSounds()
	{
		ST<AudioManager>::Get()->StopAllSounds();
	}

	/*****************************************************************//*!
	\brief
		ctrl + click to check associated usage in AudioManager.h.
	*//******************************************************************/
	static void SetChannelGroup(MonoString* soundName, MonoString* channelName)
	{
		ST<AudioManager>::Get()->SetChannelGroup(MonoStringToString(soundName), MonoStringToString(channelName));
	}

	/*****************************************************************//*!
	\brief
		ctrl + click to check associated usage in AudioManager.h.
	*//******************************************************************/
	static void SetGroupVolume(float volume, MonoString* group)
	{
		ST<AudioManager>::Get()->SetGroupVolume(volume, MonoStringToString(group));
	}
#pragma endregion

#pragma region Camera

	/*****************************************************************//*!
	\brief
		Sets the current camera's zoom value.
	\param[in] newZoom
		The new zoom value of the camera.
	\return
		None.
	*//******************************************************************/
	static void SetCameraZoom(float newZoom)
	{
		ST<CameraController>::Get()->SetZoom(newZoom);
	}

	/*****************************************************************//*!
	\brief
		Gets the current camera's zoom value.
	\param[in] newZoom
		The new zoom value of the camera.
	\return
		None.
	*//******************************************************************/
	static float GetCameraZoom()
	{
		return ST<CameraController>::Get()->GetZoom();
	}
#pragma endregion

#pragma region Animator

	/*****************************************************************//*!
	\brief
		Function to retrieve the Animator data of the entity the script
		is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] out
		Pointer to the Animator component found inside the c# script to
		transfer the Animator data to.
	\return
		None.
	*//******************************************************************/
	static void GetAnimatorComp(uint64_t entityHandle, DummyAnimator* out)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<AnimatorComponent> a = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<AnimatorComponent>();
		if (a == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}
		*out = DummyAnimator(entityHandle);
	}

	/*****************************************************************//*!
	\brief
		Function to retrieve the Animator data of the entity's child the script
		is attached to
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in, out] out
		Pointer to the Animator component found inside the c# script to
		transfer the Animator data to.
	\return
		None.
	*//******************************************************************/
	static void GetChildAnimatorComp(uint64_t entityHandle, DummyAnimator* out)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::EntityHandle e = reinterpret_cast<ecs::EntityHandle>(entityHandle);
		if (e->GetTransform().GetChildren().empty())
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have any children!", LogLevel::LEVEL_ERROR);
			return;
		}

		// Need to recursive this
		for (const auto& child : e->GetTransform().GetChildren())
		{
			ecs::CompHandle<AnimatorComponent> comp = child->GetEntity()->GetComp<AnimatorComponent>();
			if (comp != nullptr)
			{
				// Try to find the script in the current child
				*out = DummyAnimator(reinterpret_cast<uint64_t>(child->GetEntity()));
				return;
			}

			// If current child does not have it go to their children
			GetChildAnimatorComp(reinterpret_cast<uint64_t>(child->GetEntity()), out);
		}

		CONSOLE_LOG_EXPLICIT("Entity does not have specified script!", LogLevel::LEVEL_ERROR);
		return;
	}

	/*****************************************************************//*!
	\brief
		Function to Set the current animation of the entity.
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] name
		Name of the animation.
	\return
		None.
	*//******************************************************************/
	static void SetAnimation(uint64_t entityHandle, MonoString* name)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<AnimatorComponent> a = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<AnimatorComponent>();
		if (a == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		a->SetAnimation(MonoStringToString(name));
	}

	/*****************************************************************//*!
	\brief
		Function to Set the animation speed of the entity.
	\param[in] entityHandle
		The unique id of the entity the script is attached to.
	\param[in] speed
		New speed value of the animation.
	\return
		None.
	*//******************************************************************/
	static void SetAnimationSpeed(uint64_t entityHandle, float speed)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<AnimatorComponent> a = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<AnimatorComponent>();
		if (a == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		a->SetSpeed(speed);
	}

	/*****************************************************************//*!
	\brief
		Function to Set the looping flag of the animation of the entity.
	\param[in] entityHandle
		The unique id of the entity the script is attached to
	\param[in] looping
		New value of the looping flag.
	\return
		None.
	*//******************************************************************/
	static void SetAnimationLooping(uint64_t entityHandle, bool looping)
	{
		if (entityHandle == 0)
		{
			CONSOLE_LOG_EXPLICIT("Component has default entity handle(0), entity may not have component attached!", LogLevel::LEVEL_ERROR);

			return;
		}
		ecs::CompHandle<AnimatorComponent> a = reinterpret_cast<ecs::EntityHandle>(entityHandle)->GetComp<AnimatorComponent>();
		if (a == nullptr)
		{
			CONSOLE_LOG_EXPLICIT("Entity does not have specified component!", LogLevel::LEVEL_ERROR);
			return;
		}

		a->SetLooping(looping);
	}
#pragma endregion

	void ScriptGlue::RegisterFunctions()
	{
#pragma region Tests

		ADD_INTERNAL_CALL(CppNativeLog);
		ADD_INTERNAL_CALL(CppNativeLog_Vector);
		ADD_INTERNAL_CALL(CppNativeLog_VectorDot);
		ADD_INTERNAL_CALL(GetKeyPress);
		ADD_INTERNAL_CALL(MoveRight);
		ADD_INTERNAL_CALL(ChangeColour);

#pragma endregion

#pragma region Transfrom Calls

		ADD_INTERNAL_CALL(GetTransform);
		//ADD_INTERNAL_CALL(GetChildTransform);

		ADD_INTERNAL_CALL(SetTransformLocalPos);
		ADD_INTERNAL_CALL(GetTransformLocalPos);
		ADD_INTERNAL_CALL(SetTransformWorldPos);
		ADD_INTERNAL_CALL(GetTransformWorldPos);

		ADD_INTERNAL_CALL(SetTransformLocalScale);
		ADD_INTERNAL_CALL(GetTransformLocalScale);
		ADD_INTERNAL_CALL(SetTransformWorldScale);
		ADD_INTERNAL_CALL(GetTransformWorldScale);

		ADD_INTERNAL_CALL(SetTransformLocalRotate);
		ADD_INTERNAL_CALL(GetTransformLocalRotate);
		ADD_INTERNAL_CALL(SetTransformWorldRotate);
		ADD_INTERNAL_CALL(GetTransformWorldRotate);

		ADD_INTERNAL_CALL(SetTransformZPos);
		ADD_INTERNAL_CALL(GetTransformZPos);

#pragma endregion

#pragma region Physics Calls
		ADD_INTERNAL_CALL(GetPhysicsComp);
		ADD_INTERNAL_CALL(GetPhysicsMass);
		ADD_INTERNAL_CALL(GetPhysicsFriction);
		ADD_INTERNAL_CALL(SetPhysicsVelocity);
		ADD_INTERNAL_CALL(GetPhysicsVelocity);
		ADD_INTERNAL_CALL(SetPhysicsAngularVelocity);
		ADD_INTERNAL_CALL(GetPhysicsAngularVelocity);

		ADD_INTERNAL_CALL(Raycast);

#pragma endregion

#pragma region Text Calls
		ADD_INTERNAL_CALL(GetText);
		ADD_INTERNAL_CALL(SetTextColor);
		ADD_INTERNAL_CALL(GetTextColor);
		ADD_INTERNAL_CALL(SetTextString);
		ADD_INTERNAL_CALL(GetTextString);
#pragma endregion

#pragma region Time Calls
		ADD_INTERNAL_CALL(GetDeltaTime);
#pragma endregion

#pragma region Input Calls
		ADD_INTERNAL_CALL(GetCurrKey);
		ADD_INTERNAL_CALL(GetKeyPressed);
		ADD_INTERNAL_CALL(GetKeyReleased);
		ADD_INTERNAL_CALL(GetMouseWorldPos);
#pragma endregion

#pragma region Log Calls
		ADD_INTERNAL_CALL(Log);
#pragma endregion

#pragma region EntityRelated
		ADD_INTERNAL_CALL(GetScriptInstance);
		ADD_INTERNAL_CALL(GetChildScriptInstance);
		ADD_INTERNAL_CALL(FindEntity);
		ADD_INTERNAL_CALL(DestroyEntity);
		ADD_INTERNAL_CALL(InstanstiateGameObject);
		ADD_INTERNAL_CALL(InstantiatePrefab);
#pragma endregion

#pragma region Audio Calls
		ADD_INTERNAL_CALL(StartSingleSound);
		ADD_INTERNAL_CALL(StartSingleSoundWithPosition);
		ADD_INTERNAL_CALL(StartGroupedSound);
		ADD_INTERNAL_CALL(StartGroupedSoundWithPosition);
		ADD_INTERNAL_CALL(StopSound);
		ADD_INTERNAL_CALL(StopAllSounds);
		ADD_INTERNAL_CALL(SetChannelGroup);
		ADD_INTERNAL_CALL(SetGroupVolume);
#pragma endregion

#pragma region Camera Calls
		ADD_INTERNAL_CALL(SetCameraZoom);
		ADD_INTERNAL_CALL(GetCameraZoom);
#pragma endregion

#pragma region Animator Calls
		ADD_INTERNAL_CALL(GetAnimatorComp);
		ADD_INTERNAL_CALL(GetChildAnimatorComp);
		ADD_INTERNAL_CALL(SetAnimation);
		ADD_INTERNAL_CALL(SetAnimationSpeed);
		ADD_INTERNAL_CALL(SetAnimationLooping);
#pragma endregion


#pragma region Collider

#pragma endregion
	}
}