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

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Physics/JoltPhysics.h"
#include "Utilities/GameTime.h"

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

	void JoltPhysics::RemoveAndDestroyBody(JPH::BodyID bodyID)
	{
		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);
	}

	JPH::BodyInterface& JoltPhysics::GetBodyInterface()
	{
		return bodyInterface;
	}

	JPH::BodyManager& JoltPhysics::GetBodyManager()
	{
		return bodyManager;
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

	JoltBodyComp::JoltBodyComp()
		: bodyID{}
		, motionType{JPH::EMotionType::Static}
		, shapeType{ShapeType::EMPTY}
		, collisionLayer{Layers::NON_COLLIDABLE}
		, prevTrans{}
	{
	}
	
	JoltBodyComp::JoltBodyComp(JPH::EMotionType motion, ShapeType shape, Layers layer)
		: bodyID{}
		, motionType{motion}
		, shapeType{shape}
		, collisionLayer{layer}
		, prevTrans{}
	{
	}

	void JoltBodyComp::OnAttached()
	{
		JPH::ScaledShapeSettings scaleSetting{};
		switch (shapeType)
		{
		case ShapeType::EMPTY:
			scaleSetting.mInnerShape = JPH::RefConst<JPH::EmptyShapeSettings>(new JPH::EmptyShapeSettings());
			break;
		case ShapeType::BOX:
			scaleSetting.mInnerShape = JPH::RefConst<JPH::BoxShapeSettings>(new JPH::BoxShapeSettings(JPH::Vec3{0.5f, 0.5f, 0.5f}));
			break;
		default:
			CONSOLE_LOG(LEVEL_ERROR) << "Cannot recognize shape of the created body";
		}

		//Convert Vec3 to JPH::RVec3Arg
		const Transform& transform{ ecs::GetEntityTransform(this) };
		Vec3 pos{ transform.GetWorldPosition() };
		JPH::RVec3Arg position{ pos.x, pos.y, pos.z };

		Vec3 scale{ transform.GetWorldScale() };
		JPH::Vec3 scaleJolt{ scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f };
		Layers layer{ collisionLayer };
		if (JPH::ScaleHelpers::IsZeroScale(scaleJolt))
		{
			//Set the scale to minimum so that it doesn't crash.
			scaleJolt = JPH::ScaleHelpers::MakeNonZeroScale(scaleJolt);
			layer = Layers::NON_COLLIDABLE;
		}
		scaleSetting.mScale = scaleJolt;

		Vec3 rot{ transform.GetWorldRotation() };
		JPH::QuatArg rotation{ JPH::Quat::sEulerAngles(JPH::Vec3{math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z)}) };

		// A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		scaleSetting.SetEmbedded();

		//Create the shape.
		JPH::ShapeSettings::ShapeResult scaleShapeResult{ scaleSetting.Create() };
		JPH::ShapeRefC scaleShape{ scaleShapeResult.Get() };

		JPH::BodyCreationSettings bodySettings{ scaleShape, position, rotation, motionType, +layer };
		bodySettings.mAllowDynamicOrKinematic = true;

		bodyID = ST<JoltPhysics>::Get()->GetBodyInterface().CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
	}

	void JoltBodyComp::OnDetached()
	{
		ST<JoltPhysics>::Get()->RemoveAndDestroyBody(bodyID);
	}

	JPH::BodyID JoltBodyComp::GetBodyID() const
	{
		return bodyID;
	}

	JPH::EMotionType JoltBodyComp::GetMotionType() const
	{
		return motionType;
	}	
	
	ShapeType JoltBodyComp::GetShapeType() const
	{
		return shapeType;
	}

	Layers JoltBodyComp::GetCollisionLayer() const 
	{
		return collisionLayer;
	}

	const Transform& JoltBodyComp::GetPrevTrans() const 
	{
		return prevTrans;
	}

	const Vec3& JoltBodyComp::GetPosition() const
	{
		JPH::RVec3 joltPos{ ST<JoltPhysics>::Get()->GetBodyInterface().GetPosition(bodyID) };
		return Vec3{ joltPos.GetX(), joltPos.GetY(), joltPos.GetZ() };
	}
	
	const Vec3& JoltBodyComp::GetScale() const
	{
		JPH::Vec3 joltScale{};
		const JPH::Shape* shapePtr{ ST<JoltPhysics>::Get()->GetBodyInterface().GetShape(bodyID) };
		if (shapePtr->GetSubType() == JPH::EShapeSubType::Scaled)
		{
			const JPH::ScaledShape* scaledShapePtr{ static_cast<const JPH::ScaledShape*>(shapePtr) };
			joltScale = scaledShapePtr->GetScale() * 2.f;
		}
		return Vec3{ joltScale.GetX(), joltScale.GetY(), joltScale.GetZ() };
	}

	const Vec3& JoltBodyComp::GetRotation() const
	{
		JPH::RVec3 joltRot{ ST<JoltPhysics>::Get()->GetBodyInterface().GetRotation(bodyID).GetEulerAngles() };
		return Vec3{ joltRot.GetX(), joltRot.GetY(), joltRot.GetZ() };
	}

	void JoltBodyComp::SetMotionType(JPH::EMotionType type)
	{
		ST<JoltPhysics>::Get()->GetBodyInterface().SetMotionType(bodyID, type, JPH::EActivation::Activate);
		motionType = type;
	}

	void JoltBodyComp::SetShapeType(ShapeType type)
	{
		if (shapeType == type)
			return;

		JPH::Vec3 scale{};
		const JPH::Shape* shapePtr{ ST<JoltPhysics>::Get()->GetBodyInterface().GetShape(bodyID)};
		if (shapePtr->GetSubType() == JPH::EShapeSubType::Scaled)
		{
			const JPH::ScaledShape* scaledShapePtr{ static_cast<const JPH::ScaledShape*>(shapePtr) };
			scale = scaledShapePtr->GetScale();
		}
		else
		{
			CONSOLE_LOG(LEVEL_ERROR) << "The body shape is not scaled shape";
			return;
		}

		JPH::Shape* shape{ nullptr };
		switch (type)
		{
		case ShapeType::EMPTY:
			shape = new JPH::EmptyShape();
			break;
		case ShapeType::BOX:
			shape = new JPH::BoxShape(JPH::Vec3{ 0.5f, 0.5f, 0.5f });
			break;
		default:
			CONSOLE_LOG(LEVEL_ERROR) << "Input shape cannot be recognized";
			return;
		}

		JPH::RefConst<JPH::Shape> scaledShape{ new JPH::ScaledShape{shape, scale} };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetShape(bodyID, scaledShape.GetPtr(), true, JPH::EActivation::Activate);

		shapeType = type;
	}

	void JoltBodyComp::SetCollisionLayer(Layers layer)
	{
		ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +layer);
		collisionLayer = layer;
	}

	void JoltBodyComp::SetGravityFactor(float gravity)
	{
		ST<JoltPhysics>::Get()->GetBodyInterface().SetGravityFactor(bodyID, gravity);
	}

	void JoltBodyComp::SetPosition(const Vec3& pos)
	{
		JPH::Vec3 joltPos{ pos.x, pos.y, pos.z };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetPosition(bodyID, joltPos, JPH::EActivation::Activate);
	}

	void JoltBodyComp::SetRotation(const Vec3& rot)
	{
		JPH::Quat joltRot{ JPH::Quat::sEulerAngles(JPH::Vec3{math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z)}) };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetRotation(bodyID, joltRot, JPH::EActivation::Activate);
	}

	void JoltBodyComp::SetScale(const Vec3& scale)
	{
		JPH::Vec3 joltScale{ scale.x / 2.f, scale.y / 2.f, scale.z / 2.f };
		const JPH::Shape* shapePtr{ ST<JoltPhysics>::Get()->GetBodyInterface().GetShape(bodyID) };
		if (shapePtr->GetSubType() == JPH::EShapeSubType::Scaled)
		{
			const JPH::ScaledShape* scaledShapePtr{ static_cast<const JPH::ScaledShape*>(shapePtr) };
			if (!scaledShapePtr->IsValidScale(joltScale))
			{
				joltScale = JPH::ScaleHelpers::MakeNonZeroScale(joltScale);
				ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +Layers::NON_COLLIDABLE);
			}
			else 
				ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +collisionLayer);
		}

		JPH::Shape::ShapeResult result{ shapePtr->ScaleShape(joltScale) };
		if (!result.IsValid())
			CONSOLE_LOG(LEVEL_ERROR) << "Error when scaling the rigid body.";
	}

	void JoltBodyComp::SetPrevTrans(const Transform& trans)
	{
		prevTrans = trans;
	}

	void JoltBodyComp::SetLinearVelocity(const Vec3& vel)
	{
		JPH::Vec3 joltVel{ vel.x, vel.y, vel.z };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetLinearVelocity(bodyID, joltVel);
	}

	void JoltBodyComp::UpdateBody()
	{
		Transform const& trans{ ecs::GetEntityTransform(this) };

		Vec3 pos{ trans.GetWorldPosition() };
		if (pos != prevTrans.GetWorldPosition())
		{
			if (auto colliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
				pos += colliderComp->GetCenter();			
			SetPosition(pos);
		}

		Vec3 scale{ trans.GetWorldScale() };
		if (scale != prevTrans.GetWorldScale())
		{
			if (auto colliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
				scale *= colliderComp->GetSize();
			SetScale(scale);
		}

		Vec3 rot{ trans.GetWorldRotation() };
		if (rot != prevTrans.GetWorldRotation())
		{
			SetRotation(rot);
		}
	}

	void JoltBodyComp::UpdateEntity()
	{
		if (motionType == JPH::EMotionType::Static)
			return;

		JPH::RVec3 joltPos{ ST<JoltPhysics>::Get()->GetBodyInterface().GetPosition(bodyID) };
		Vec3 pos{ joltPos.GetX(), joltPos.GetY(), joltPos.GetZ() };
		if (auto colliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
			pos -= colliderComp->GetCenter();
		ecs::GetEntityTransform(this).SetWorldPosition(Vec3{ pos.x, pos.y, pos.z });

		JPH::RVec3 rot{ ST<JoltPhysics>::Get()->GetBodyInterface().GetRotation(bodyID).GetEulerAngles() };
		ecs::GetEntityTransform(this).SetWorldRotation(Vec3{ rot.GetX(), rot.GetY(), rot.GetZ() });

		prevTrans = ecs::GetEntityTransform(this);
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
