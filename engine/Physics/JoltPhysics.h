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

All content © 2025 DigiPen Institute of Technology Singapore.
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

#include "Collision.h"

JPH_SUPPRESS_WARNINGS

namespace physics {
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
			Create and add a body to the bodyInterface.
		\param transform
			Transform component of the entity.
		\param motionType
			Motion type of the body (DYNAMIC, STATIC, KINEMATIC)
		\param collisionLayer
			collision layer of the body (currently NON_MOVING and MOVING)
		\param activate
			shared pointer of the body created.
		*//******************************************************************/
		JPH::BodyID CreateAndAddEmptyBody(const Transform& transform, JPH::EMotionType motionType, JPH::ObjectLayer collisionLayer, bool activate = true);

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
			Update the Jolt Physics System.
		*//******************************************************************/
		void UpdatePhysicsSystem();

		/*****************************************************************//*!
		\brief
			Optimize the broad face collision to increase efficiency.
		*//******************************************************************/
		void OptimizeBroadPhase();

		/*****************************************************************//*!
		\brief
			Scale a body in certain size.
		\param bodyID
			ID of the body to scale.
		\param scaleFactor
			scale value to scale the body.
		*//******************************************************************/
		void ScaleShape(JPH::BodyID bodyID, const Vec3& scale);

		/*****************************************************************//*!
		\brief
			Set the position of the body in the body interface.
		\param bodyID
			The ID of the body to get the position.
		\param pos
			Position to set the body.
		*//******************************************************************/
		void SetBodyPosition(JPH::BodyID bodyID, const Vec3& pos);
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

	/*****************************************************************//*!
	\brief
		Register the Jolt Default Allocator, Factory, and Types needed
		to use any of the Jolt Types.
	*//******************************************************************/
	void JoltRegister();
}