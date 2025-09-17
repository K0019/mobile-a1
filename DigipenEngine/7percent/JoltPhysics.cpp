/******************************************************************************/
/*!
\file   JoltPhysics.cpp
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Contains definitions for the JoltPhysics class.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "JoltPhysics.h"
#include "GameTime.h"

namespace physics {
	JoltPhysics::JoltPhysics()
		: tempAllocator{ 10 * 1024 * 1024 }
		, jobSystem{ JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(JPH::thread::hardware_concurrency()) - 1 }
		, cMaxBodies{ 65536 } 
		, cNumBodyMutexes{ 0 }
		, cMaxBodyPairs{ 65536 }
		, cMaxContactConstraints{ 10240 }
		, broadPhaseLayerInterface{}
		, objectVsBroadphaseLayerFilter{}
		, objectVsObjectLayerFilter{}
		, physicsSystem{}
		, bodyInterface{physicsSystem.GetBodyInterface()}
		, contactListener{}
		, bodyManager{}
	{
	}

	void JoltPhysics::Initialize()
	{
		//Create the physics system.
		physicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);

		//Set contact listener.
		physicsSystem.SetContactListener(&contactListener);

		//Initialize the body manager.
		bodyManager.Init(cMaxBodies, cNumBodyMutexes, broadPhaseLayerInterface);
	}

	JPH::BodyID JoltPhysics::CreateAndAddEmptyBody(const Transform& transform, JPH::EMotionType motionType, JPH::ObjectLayer collisionLayer, bool activate)
	{
		//Settings of the empty shape.
		JPH::EmptyShapeSettings emptyShapeSetting{};

		// A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		emptyShapeSetting.SetEmbedded();

		//Create the shape.
		JPH::ShapeSettings::ShapeResult emptyShapeResult{ emptyShapeSetting.Create() };
		JPH::ShapeRefC emptyShape{ emptyShapeResult.Get() };

		Vec3 pos{ transform.GetWorldPosition() };
		JPH::RVec3Arg position{pos.x, pos.y, pos.z };
		Vec3 scale{ transform.GetWorldScale() };
		JPH::Vec3 scaleJolt{ scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f };
		Vec3 rot{ transform.GetWorldRotation() };
		JPH::QuatArg rotation{ JPH::Quat::sEulerAngles(JPH::Vec3{math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z)}) };

		JPH::BodyCreationSettings settings{ new JPH::ScaledShape(new JPH::EmptyShape(), scaleJolt), position, rotation, motionType, collisionLayer};
		settings.mAllowDynamicOrKinematic = true;
		return bodyInterface.CreateAndAddBody(settings, (activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate));
	}

	void JoltPhysics::RemoveAndDestroyBody(JPH::BodyID bodyID)
	{
		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);
	}

	JPH::BodyInterface& JoltPhysics::GetBodyInterface()
	{
		return bodyInterface;
	}

	void JoltPhysics::UpdatePhysicsSystem()
	{
		physicsSystem.Update((GameTime::IsFixedDtMode() ? GameTime::FixedDt() : GameTime::RealDt()), 1, &tempAllocator, &jobSystem);
	}

	void JoltPhysics::OptimizeBroadPhase()
	{
		physicsSystem.OptimizeBroadPhase();
	}

	JoltPhysics::~JoltPhysics()
	{
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void JoltPhysics::SetBodyPosition(JPH::BodyID bodyID, const Vec3& pos)
	{
		JPH::RVec3Arg position{ pos.x, pos.y, pos.z };
		bodyInterface.SetPosition(bodyID, position, JPH::EActivation::Activate);
	}

	void JoltPhysics::ScaleShape(JPH::BodyID bodyID, const Vec3& scale)
	{
		if (scale.x == 0.f || scale.y == 0.f || scale.z == 0.f)
		{
			bodyInterface.SetIsSensor(bodyID, true);
			return;
		}
		if (bodyInterface.IsSensor(bodyID))
			bodyInterface.SetIsSensor(bodyID, false);

		JPH::BodyLockWrite lock(physicsSystem.GetBodyLockInterface(), bodyID);
		if (lock.Succeeded())
		{
			JPH::Body& body = lock.GetBody();
			JPH_ASSERT(body.GetShape()->GetSubType() == JPH::EShapeSubType::Scaled);
			const JPH::ScaledShape* scaledShape = static_cast<const JPH::ScaledShape*>(body.GetShape());
			const JPH::Shape* nonScaledShape = scaledShape->GetInnerShape();

			JPH::Shape::ShapeResult newShape = nonScaledShape->ScaleShape(JPH::Vec3{scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f});
			JPH_ASSERT(newShape.IsValid());

			physicsSystem.GetBodyInterfaceNoLock().SetShape(bodyID, newShape.Get(), true, JPH::EActivation::Activate);
		}
		else
			CONSOLE_LOG(LEVEL_ERROR) << "Body could not be properly scaled!";
	}

	// Callback for traces, connect this to your own trace function if you have one
	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);

		// Print to the TTY
		CONSOLE_LOG(LEVEL_ERROR) << buffer;
	}

#ifdef JPH_ENABLE_ASSERTS

	// Callback for asserts, connect this to your own assert handler if you have one
	static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
	{
		// Print to the TTY
		CONSOLE_LOG(LEVEL_ERROR) << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "");

		// Breakpoint
		return true;
	};

#endif // JPH_ENABLE_ASSERTS

	void JoltRegister()
	{
		// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
		// This needs to be done before any other Jolt function is called.
		JPH::RegisterDefaultAllocator();

		// Install trace and assert callbacks
		JPH::Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

		// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
		// It is not directly used in this example but still required.
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
		// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
		// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
		JPH::RegisterTypes();
	}
}
