#include "JoltPhysics.h"

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
	{
		physicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);
	}

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

	void JoltUnRegister()
	{
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}
