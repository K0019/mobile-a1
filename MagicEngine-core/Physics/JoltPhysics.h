/******************************************************************************/
/*!
\file   JoltPhysics.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	JoltPhysics contains the Joly Physics System that updates all the bodies. 

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
//Need to include this before including any other Jolt file
#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "ECS/ECS.h"
#include "Editor/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "Physics/Collision.h"

JPH_SUPPRESS_WARNINGS

namespace physics {
	enum class ShapeType
	{
		EMPTY,
		BOX
	};

	class JoltPhysics
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		JoltPhysics();

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~JoltPhysics();

		/*****************************************************************//*!
		\brief
			Initializes the Jolt Physics System.
		*//******************************************************************/
		void Initialize();

		/*****************************************************************//*!
		\brief
			Remove and destroy the body from the Jolt Physics System.
		\param bodyID
			bodyID of the body to remove.
		*//******************************************************************/
		void RemoveAndDestroyBody(JPH::BodyID bodyID);

		/*****************************************************************//*!
		\brief
			Get a reference of the body interface.
		\return
			Reference of the body interface.
		*//******************************************************************/
		JPH::BodyInterface& GetBodyInterface();

		/*****************************************************************//*!
		\brief
			Get a reference of the body manager.
		\return
			Reference of the body manager.
		*//******************************************************************/
		JPH::BodyManager& GetBodyManager();

		/*****************************************************************//*!
		\brief
			Update the Jolt Physics System.
		*//******************************************************************/
		void UpdatePhysicsSystem();

		/*****************************************************************//*!
		\brief
			Optimize the broad face collision to increase efficiency.
		*//******************************************************************/
		void OptimizeBroadPhase();

	private:
		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		JPH::TempAllocatorImpl tempAllocator;

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		JPH::JobSystemThreadPool jobSystem;

		// This is the max amount of rigid bodies that you can add to the physics system. 
		// If you try to add more you'll get an error.
		JPH::uint cMaxBodies;

		// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
		JPH::uint cNumBodyMutexes;

		// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
		// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
		// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
		JPH::uint cMaxBodyPairs;

		// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
		// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
		JPH::uint cMaxContactConstraints;

		// Create mapping table from object layer to broadphase layer
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
		BPLayerInterfaceImpl broadPhaseLayerInterface;

		// Create class that filters object vs broadphase layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
		ObjectVsBroadPhaseLayerFilterImpl objectVsBroadphaseLayerFilter;

		// Create class that filters object vs object layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
		ObjectLayerPairFilterImpl objectVsObjectLayerFilter;

		// Jolt's physics system.
		JPH::PhysicsSystem physicsSystem;

		// This interface allows to to create / remove bodies and to change their properties.
		JPH::BodyInterface& bodyInterface;

		// This contact listener prints to the console when a collision occurs.
		MyContactListener contactListener;	

		// Body manager that allows certain bodies to change the scale.
		JPH::BodyManager bodyManager;
	};

	class JoltBodyComp
		: public ecs::IComponentCallbacks
		, public IHiddenComponent<JoltBodyComp>
	{
	public:
		/*****************************************************************//*!
		\brief
			Default Constructor.
		*//******************************************************************/
		JoltBodyComp();

		JoltBodyComp(JPH::EMotionType motion, ShapeType shape, Layers layer);

	private:
		//Body ID of the containing JPH::Body
		JPH::BodyID bodyID;
		//Static, Dynamic, or Kinematic
		JPH::EMotionType motionType;
		//The type of shape of the body.
		ShapeType shapeType;
		//Collision layer.
		Layers collisionLayer;
		//Previous Transform stored.
		Transform prevTrans;

	public:
		void OnAttached() override;
		void OnDetached() override;

		JPH::BodyID GetBodyID() const;
		JPH::EMotionType GetMotionType() const;
		ShapeType GetShapeType() const;
		Layers GetCollisionLayer() const;
		const Transform& GetPrevTrans() const;
		Vec3 GetPosition() const;
		Vec3 GetScale() const;
		Vec3 GetRotation() const;

		void SetMotionType(JPH::EMotionType type);
		void SetShapeType(ShapeType type);
		void SetCollisionLayer(Layers layer);
		void SetGravityFactor(float gravity);
		void SetPosition(const Vec3& pos);
		void SetRotation(const Vec3& rot);
		void SetScale(const Vec3& scale);
		void SetPrevTrans(const Transform& trans);
		void SetLinearVelocity(const Vec3& vel);

		void UpdateBody();
		void UpdateEntity();
	};

	/*****************************************************************//*!
	\brief
		Register the Jolt Default Allocator, Factory, and Types needed
		to use any of the Jolt Types.
	*//******************************************************************/
	void JoltRegister();
}