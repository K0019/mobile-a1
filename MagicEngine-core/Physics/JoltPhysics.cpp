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

All content   2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Physics/JoltPhysics.h"
#include "Physics/Physics.h"
#include "Engine/EntityEvents.h"
#include "Utilities/GameTime.h"
#include <imgui.h>

#include "Graphics/CameraController.h"

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
		, bodyInterface{ physicsSystem.GetBodyInterface() }
		, contactListener{}
		, bodyManager{}
	{
	}

	void JoltPhysics::Initialize()
	{
		//Create the physics system.
		physicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);

		//Set contact listener.
		contactListener.Init();
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

	JPH::PhysicsSystem& JoltPhysics::GetPhysicsSystem()
	{
		return physicsSystem;
	}

	void JoltPhysics::UpdatePhysicsSystem()
	{
		physicsSystem.Update((GameTime::IsFixedDtMode() ? GameTime::FixedDt() : GameTime::RealDt()), 1, &tempAllocator, &jobSystem);
	}

	void JoltPhysics::OptimizeBroadPhase()
	{
		physicsSystem.OptimizeBroadPhase();
	}

	JPH::AABox JoltPhysics::CollectAllTriangles(std::vector<float>& outVertices, std::vector<int>& outTriIndex)
	{
		OptimizeBroadPhase();
		std::unordered_map<JPH::Float3, int> vert2Index{};
		int highestIndex{};
		JPH::AABox bound{ physicsSystem.GetBounds() };
		bound.mMin -= JPH::Vec3{ 1.f, 1.f, 1.f };
		bound.mMax += JPH::Vec3{ 1.f, 1.f, 1.f };

		for (auto colCompIter{ ecs::GetCompsActiveBegin<physics::BoxColliderComp>() }, endIter{ ecs::GetCompsEnd<physics::BoxColliderComp>() }; colCompIter != endIter; ++colCompIter)
		{
			//Update the body first.
			colCompIter.GetEntity()->GetComp<JoltBodyComp>()->UpdateBody();

			// If the collider is not enabled or if it has a physics component, it's not a static object so go to the next collider.
			if (!colCompIter->GetFlag(physics::COLLIDER_COMP_FLAG::ENABLED) || colCompIter.GetEntity()->HasComp<physics::PhysicsComp>())
				continue;

			auto bodyCompPtr{ colCompIter.GetEntity()->GetComp<physics::JoltBodyComp>() };

			//Get necessary data.
			JPH::Shape::GetTrianglesContext triContext{};

			const JPH::Shape* shape{ bodyInterface.GetShape(bodyCompPtr->GetBodyID()) };
			const JPH::ScaledShape* scaledShape{ static_cast<const JPH::ScaledShape*>(shape) };
			shape = scaledShape->GetInnerShape();

			JPH::Vec3 pos{ bodyInterface.GetPosition(bodyCompPtr->GetBodyID()) };
			Vec3 temp{ bodyCompPtr->GetScale() / 2.f };
			JPH::Vec3 scale{ temp.x, temp.y, temp.z };
			JPH::Quat rot{ bodyInterface.GetRotation(bodyCompPtr->GetBodyID()) };

			//Get triangles
			shape->GetTrianglesStart(triContext, bound, pos, rot, scale);
			std::size_t maxTri{ 512 };
			std::vector<JPH::Float3> triBuffer(maxTri * 3);
			int triCount{};


			while ((triCount = shape->GetTrianglesNext(triContext, static_cast<int>(maxTri), triBuffer.data())) > 0)
			{
				for (std::size_t count{}; count < triCount; ++count)
				{
					std::array<JPH::Float3, 3> vert{ triBuffer[count * 3], triBuffer[count * 3 + 1], triBuffer[count * 3 + 2] };
					std::vector<int> indices{};
					for (std::size_t vertexNum{}; vertexNum < 3; ++vertexNum)
					{
						auto iter{ vert2Index.find(vert[vertexNum]) };
						if (iter != vert2Index.end())
						{
							indices.push_back(iter->second);
						}
						else
						{
							indices.push_back(highestIndex);
							vert2Index[vert[vertexNum]] = highestIndex++;
							outVertices.push_back(vert[vertexNum][0]);
							outVertices.push_back(vert[vertexNum][1]);
							outVertices.push_back(vert[vertexNum][2]);
						}
					}

					for (int index : indices)
						outTriIndex.push_back(index);
				}
			}
		}

		return bound;
	}

	TransformValues& TransformValues::operator=(Transform& trans)
	{
		pos = trans.GetWorldPosition();
		scale = trans.GetWorldScale();
		rot = trans.GetWorldRotation();
		return *this;
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
		, motionType{ JPH::EMotionType::Static }
		, shapeType{ ShapeType::EMPTY }
		, collisionLayer{ Layers::NON_COLLIDABLE }
		, prevTrans{}
		, prevQuat{}
		, dof{ JPH::EAllowedDOFs::All }
	{
	}

	JoltBodyComp::JoltBodyComp(JPH::EMotionType motion, ShapeType shape, Layers layer)
		: bodyID{}
		, motionType{ motion }
		, shapeType{ shape }
		, collisionLayer{ layer }
		, prevTrans{}
		, prevQuat{}
		, dof{ JPH::EAllowedDOFs::All }
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
			scaleSetting.mInnerShape = JPH::RefConst<JPH::BoxShapeSettings>(new JPH::BoxShapeSettings(JPH::Vec3{ 1.f, 1.f, 1.f }));
			break;
		default:
			CONSOLE_LOG(LEVEL_ERROR) << "Cannot recognize shape of the created body";
		}

		//Convert Vec3 to JPH::RVec3Arg
		const Transform& transform{ ecs::GetEntityTransform(this) };
		Vec3 pos{ transform.GetWorldPosition() };
		JPH::RVec3Arg position{ pos.x, pos.y, pos.z };

		Vec3 rot{ transform.GetWorldRotation() };
		JPH::QuatArg rotation{ JPH::Quat::sEulerAngles(JPH::Vec3{math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z)}) };

		// A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		scaleSetting.SetEmbedded();

		//Create the shape.
		JPH::ShapeSettings::ShapeResult scaleShapeResult{ scaleSetting.Create() };
		JPH::ShapeRefC scaleShape{ scaleShapeResult.Get() };

		JPH::BodyCreationSettings bodySettings{ scaleShape, position, rotation, motionType, +collisionLayer };
		bodySettings.mAllowDynamicOrKinematic = true;
		bodySettings.mAllowSleeping = false;
		bodySettings.mUserData = ecs::GetEntity(this)->GetHash();
		bodySettings.mGravityFactor = 0.f;

		bodyID = ST<JoltPhysics>::Get()->GetBodyInterface().CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

		Vec3 scale{ transform.GetWorldScale() };
		JPH::Vec3 scaleJolt{ scale.x, scale.y, scale.z };
		if (JPH::ScaleHelpers::IsZeroScale(scaleJolt))
		{
			//Set the scale to minimum so that it doesn't crash.
			scaleJolt = JPH::ScaleHelpers::MakeNonZeroScale(scaleJolt);
			ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +Layers::NON_COLLIDABLE);
		}
		SetScale(scale);

		if (ecs::GetEntity(this)->HasComp<BoxColliderComp>())
		{
			SetShapeType(ShapeType::BOX);
		}

		ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("JoltBodyCompAttached", this);
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

	Vec3 JoltBodyComp::GetPosition() const
	{
		JPH::RVec3 joltPos{ ST<JoltPhysics>::Get()->GetBodyInterface().GetPosition(bodyID) };
		return Vec3{ joltPos.GetX(), joltPos.GetY(), joltPos.GetZ() };
	}

	Vec3 JoltBodyComp::GetScale() const
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

	JPH::Quat JoltBodyComp::GetRotation() const
	{
		return ST<JoltPhysics>::Get()->GetBodyInterface().GetRotation(bodyID);
	}

	Vec3 JoltBodyComp::GetLinearVelocity() const
	{
		JPH::Vec3 vec{ ST<JoltPhysics>::Get()->GetBodyInterface().GetLinearVelocity(bodyID) };
		return Vec3{ vec.GetX(), vec.GetY(), vec.GetZ() };
	}

	Vec3 JoltBodyComp::GetAngularVelocity() const
	{
		JPH::Vec3 vec{ ST<JoltPhysics>::Get()->GetBodyInterface().GetAngularVelocity(bodyID) };
		return Vec3{ vec.GetX(), vec.GetY(), vec.GetZ() };
	}

	bool JoltBodyComp::IsTrigger() const
	{
		JPH::BodyLockWrite lock(ST<JoltPhysics>::Get()->GetPhysicsSystem().GetBodyLockInterface(), bodyID);
		if (lock.Succeeded())
			return lock.GetBody().IsSensor();

		return false;
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
		const JPH::Shape* shapePtr{ ST<JoltPhysics>::Get()->GetBodyInterface().GetShape(bodyID) };
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
			shape = new JPH::BoxShape(JPH::Vec3{ 1.f, 1.f, 1.f });
			break;
		default:
			CONSOLE_LOG(LEVEL_ERROR) << "Input shape cannot be recognized";
			return;
		}

		JPH::ShapeRefC scaledShape{ new JPH::ScaledShape{shape, scale} };

		ST<JoltPhysics>::Get()->GetBodyInterface().SetShape(bodyID, scaledShape.GetPtr(), true, JPH::EActivation::Activate);

		shapeType = type;
	}

	void JoltBodyComp::SetCollisionLayer(Layers layer)
	{
		collisionLayer = layer;
		Vec3 scale{ ecs::GetEntityTransform(this).GetWorldScale() };
		if (auto colliderPtr{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
			scale *= colliderPtr->GetSize();
		if (!scale.x || !scale.y || !scale.z)
			layer = Layers::NON_COLLIDABLE;
		ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +layer);
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

	void JoltBodyComp::SetScale(const Vec3& scale)
	{
		JPH::Vec3 joltScale{ scale.x / 2.f, scale.y / 2.f, scale.z / 2.f };
		const JPH::Shape* shapePtr{ ST<JoltPhysics>::Get()->GetBodyInterface().GetShape(bodyID) };
		if (shapePtr->GetSubType() != JPH::EShapeSubType::Scaled)
			return;

		const JPH::ScaledShape* scaledShapePtr{ static_cast<const JPH::ScaledShape*>(shapePtr) };
		const JPH::Shape* nonScaledShapePtr{ scaledShapePtr->GetInnerShape() };
		if (!nonScaledShapePtr->IsValidScale(joltScale) || !scale.x || !scale.y || !scale.z)
		{
			joltScale = JPH::ScaleHelpers::MakeNonZeroScale(joltScale);
			ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +Layers::NON_COLLIDABLE);
		}
		else
			ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +collisionLayer);

		JPH::Shape::ShapeResult result{ nonScaledShapePtr->ScaleShape(joltScale) };
		if (!result.IsValid())
			CONSOLE_LOG(LEVEL_ERROR) << "Error when scaling the rigid body.";

		ST<JoltPhysics>::Get()->GetBodyInterface().SetShape(bodyID, result.Get(), true, JPH::EActivation::Activate);
	}

	void JoltBodyComp::SetRotation(const Vec3& rot)
	{
		JPH::Quat joltRot{ JPH::Quat::sEulerAngles(JPH::Vec3{math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z)}) };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetRotation(bodyID, joltRot, JPH::EActivation::Activate);
	}

	void JoltBodyComp::SetLinearVelocity(const Vec3& vel)
	{
		JPH::Vec3 joltVel{ vel.x, vel.y, vel.z };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetLinearVelocity(bodyID, joltVel);
	}

	void JoltBodyComp::AddLinearVelocity(const Vec3& vel)
	{
		JPH::Vec3 joltVel{ vel.x, vel.y, vel.z };
		ST<JoltPhysics>::Get()->GetBodyInterface().AddLinearVelocity(bodyID, joltVel);
	}

	void JoltBodyComp::SetAngularVelocity(const Vec3& vel)
	{
		JPH::Vec3 joltVel{ math::ToRadians(vel.x), math::ToRadians(vel.y), math::ToRadians(vel.z) };
		ST<JoltPhysics>::Get()->GetBodyInterface().SetAngularVelocity(bodyID, joltVel);
	}

	void JoltBodyComp::SetLockRotationX(bool val)
	{
		val ? dof &= ~JPH::EAllowedDOFs::RotationX : dof |= JPH::EAllowedDOFs::RotationX;
		SetDOF(dof);
	}

	void JoltBodyComp::SetLockRotationY(bool val)
	{
		val ? dof &= ~JPH::EAllowedDOFs::RotationY : dof |= JPH::EAllowedDOFs::RotationY;
		SetDOF(dof);
	}

	void JoltBodyComp::SetLockRotationZ(bool val)
	{
		val ? dof &= ~JPH::EAllowedDOFs::RotationZ : dof |= JPH::EAllowedDOFs::RotationZ;
		SetDOF(dof);
	}

	void JoltBodyComp::SetDOF(JPH::EAllowedDOFs val)
	{
		JPH::BodyLockWrite lock{ ST<JoltPhysics>::Get()->GetPhysicsSystem().GetBodyLockInterface(), bodyID };
		if (!lock.Succeeded())
			return;

		JPH::Body& body{ lock.GetBody() };
		if (!body.IsDynamic())
			return;

		JPH::MotionProperties* property{ body.GetMotionProperties() };
		JPH::MassProperties massProp{ body.GetShape()->GetMassProperties() };
		float invMass{ property->GetInverseMass() };
		if (invMass > 0.f)
			massProp.ScaleToMass(1.f / invMass);
		property->SetMassProperties(val, massProp);
	}

	void JoltBodyComp::SetIsTrigger(bool val)
	{
		JPH::BodyLockWrite lock(ST<JoltPhysics>::Get()->GetPhysicsSystem().GetBodyLockInterface(), bodyID);
		if (lock.Succeeded())
			lock.GetBody().SetIsSensor(val);
	}

	void JoltBodyComp::UpdateBody()
	{
		Transform const& trans{ ecs::GetEntityTransform(this) };

		Vec3 pos{ trans.GetWorldPosition() };
		if (pos != prevTrans.pos)
		{
			if (auto colliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
				pos += colliderComp->GetCenter();
			SetPosition(pos);
		}

		Vec3 scale{ trans.GetWorldScale() };
		if (scale != prevTrans.scale)
		{
			if (auto colliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
				scale *= colliderComp->GetSize();
			SetScale(scale);
		}

		Vec3 rot{ trans.GetWorldRotation() };
		if (rot != prevTrans.rot)
		{
			SetRotation(rot);
		}

		prevQuat = ST<JoltPhysics>::Get()->GetBodyInterface().GetRotation(bodyID);
	}

	void JoltBodyComp::UpdateEntity()
	{
		if (motionType == JPH::EMotionType::Static)
		{
			prevTrans = ecs::GetEntityTransform(this);
			return;
		}

		JPH::RVec3 joltPos{ ST<JoltPhysics>::Get()->GetBodyInterface().GetPosition(bodyID) };
		Vec3 pos{ joltPos.GetX(), joltPos.GetY(), joltPos.GetZ() };
		if (auto colliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
			pos -= colliderComp->GetCenter();
		ecs::GetEntityTransform(this).SetWorldPosition(Vec3{ pos.x, pos.y, pos.z });

		auto wrapDeg = [](float deg)
			{
				float temp{ std::fmod(deg + 180.f, 360.f) };
				if (temp < 0.f)
					temp += 360.f;
				return temp - 180.0f;
			};

		JPH::Quat quat{ GetRotation() };
		JPH::Quat deltaQuat{ (quat * prevQuat.Inversed()).Normalized() };
		JPH::Vec3 deltaEuler{ deltaQuat.GetEulerAngles() };
		Vec3 deltaEulerDeg{ math::ToDegrees(deltaEuler.GetX()), math::ToDegrees(deltaEuler.GetY()), math::ToDegrees(deltaEuler.GetZ()) };
		Vec3 estimateRot{ ecs::GetEntityTransform(this).GetWorldRotation() + deltaEulerDeg };
		JPH::Vec3 joltRot{ quat.GetEulerAngles() };
		Vec3 rot{ math::ToDegrees(joltRot.GetX()), math::ToDegrees(joltRot.GetY()), math::ToDegrees(joltRot.GetZ()) };
		float xDiff{ std::abs(rot.x - estimateRot.x) };
		float zDiff{ std::abs(rot.z - estimateRot.z) };
		if (estimateRot.y >= 90.f || estimateRot.y <= -90.f)
		{
			rot.x = wrapDeg(xDiff < 1e-4 ? rot.x : rot.x + 180.f);
			rot.y = wrapDeg(180.f - rot.y);
			rot.z = wrapDeg(zDiff < 1e-4 ? rot.z : rot.z + 180.f);
		}

		ecs::GetEntityTransform(this).SetWorldRotation(rot);

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

	void JoltPhysics::DebugDraw()
	{
#if defined(JPH_DEBUG_RENDERER)
		Vec3 v = ST<CameraController>::Get()->GetCameraData().position;
		joltDebugger.SetCameraPos(JPH::RVec3(v.x, v.y, v.z));

		JPH::BodyManager::DrawSettings settings;
		settings.mDrawShape = true;
		settings.mDrawShapeWireframe = true;

		physicsSystem.DrawBodies(settings, &joltDebugger);
#endif
	}
}
