/******************************************************************************/
/*!
\file   JoltPhysics.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/09/2025

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
			activate the body or not. Default set to true.
		*//******************************************************************/
		JPH::BodyID const& CreateAndAddEmptyBody(Transform const& transform, JPH::EMotionType motionType, JPH::ObjectLayer collisionLayer, bool activate = true);

		/*****************************************************************//*!
		\brief
			Remove and destroy the body from the Jolt Physics System.
		\param bodyID
			bodyID of the body to remove.
		*//******************************************************************/
		void RemoveAndDestroyBody(JPH::BodyID bodyID);

		/*****************************************************************//*!
		\brief
			Update the Jolt Physics System.
		*//******************************************************************/
		void UpdatePhysicsSystem();

		/*****************************************************************//*!
		\brief
			Get the position of the body in the body interface.
		\param bodyID
			The ID of the body to get the position.
		\return
			Vec3 value that represents the position of the body.
		*//******************************************************************/
		Vec3 const& GetBodyPosition(JPH::BodyID bodyID);

		/*****************************************************************//*!
		\brief
			Set the position of the body in the body interface.
		\param bodyID
			The ID of the body to get the position.
		\param pos
			Position to set the body.
		*//******************************************************************/
		void SetBodyPosition(JPH::BodyID bodyID, Vec3 const& pos);
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
	};

	/*****************************************************************//*!
	\brief
		Register the Jolt Default Allocator, Factory, and Types needed
		to use any of the Jolt Types.
	*//******************************************************************/
	void JoltRegister();
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