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

All content   2025 DigiPen Institute of Technology Singapore.
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
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

#include "ECS/ECS.h"
#include "ECS/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "Physics/Collision.h"
#include "Engine/EntityLayers.h"

#if defined(JPH_DEBUG_RENDERER)
// YC: For debugging
#include "Physics/JoltDebugRenderer.h"
#endif
JPH_SUPPRESS_WARNINGS

namespace physics {
	enum class ShapeType
	{
		EMPTY,
		BOX
	};

	struct RaycastHit
	{
		Vec3 point;
		Vec3 normal;
		ecs::EntityHandle entityHit;
		float distance;
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
			Get a reference of the physics system.
		\return
			Reference of the physics system.
		*//******************************************************************/
		JPH::PhysicsSystem& GetPhysicsSystem();

		JPH::TempAllocatorImpl& GetTempAllocator();

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


		void DebugDraw();

		void SetCharacterRadius(JPH::Ref<JPH::CharacterVirtual> character, float radius);
		void SetCharacterHeight(JPH::Ref<JPH::CharacterVirtual> character, float height);
		JPH::Ref<JPH::CharacterVirtual> CreateCharacterBody(ecs::EntityHash entityHash);
		void UpdateCharacterBody(ecs::EntityHandle entity, JPH::Ref<JPH::CharacterVirtual> character, const Vec3& velocity);

		JPH::AABox CollectAllTriangles(std::vector<float>& outVertices, std::vector<int>& outTriIndex);

		bool Raycast(const Vec3& origin, const Vec3& direction, RaycastHit& hitInfo, float maxDistance, EntityLayersMask layerMask);
		bool RaycastAll(const Vec3& origin, const Vec3& direction, std::vector<RaycastHit>& allHitInfo, float maxDistance, EntityLayersMask layerMask);
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

#if defined(JPH_DEBUG_RENDERER)
		JoltDebugRenderer joltDebugger;
#endif
	};

	struct TransformValues
	{
		Vec3 pos;
		Vec3 rot;
		Vec3 scale;

		TransformValues();
		TransformValues(Transform& trans);
		TransformValues& operator=(Transform& trans);
	};

	bool operator==(TransformValues const& lhs, TransformValues const& rhs);
	bool operator!=(TransformValues const& lhs, TransformValues const& rhs);

	class JoltBodyComp
		: public ecs::IComponentCallbacks
		, public IHiddenComponent<JoltBodyComp>
		//, public IEditorComponent<JoltBodyComp>
	{
	public:
		/*****************************************************************//*!
		\brief
			Default Constructor.
		*//******************************************************************/
		JoltBodyComp();

		/*****************************************************************//*!
		\brief
			Constructor.
		\param motion
			motion type to set.
		\param shape
			shape type to set.
		\param layer
			collision layer to set.
		*//******************************************************************/
		JoltBodyComp(JPH::EMotionType motion, ShapeType shape, Layers layer);

	private:
		//Previous Transform stored.
		TransformValues prevTrans;
		//Quaternion before the physics update.
		JPH::Quat prevQuat;
		//Body ID of the containing JPH::Body
		JPH::BodyID bodyID;
		//Static, Dynamic, or Kinematic
		JPH::EMotionType motionType;
		//The type of shape of the body.
		ShapeType shapeType;
		//Collision layer.
		Layers collisionLayer;
		//Degree of Freedom
		JPH::EAllowedDOFs dof;

	public:
		void OnCreation() override;
		void OnDetached() override;

		/*****************************************************************//*!
		\brief
			Get the body ID of the body.
		\return
			body ID value.
		*//******************************************************************/
		JPH::BodyID GetBodyID() const;

		/*****************************************************************//*!
		\brief
			Get the motion type of the body. DYNAMIC, KINEMATIC, STATIC
		\return
			motion type value.
		*//******************************************************************/
		JPH::EMotionType GetMotionType() const;

		/*****************************************************************//*!
		\brief
			Get the shape type of the body. BOX, EMPTY
		\return
			shape type value.
		*//******************************************************************/
		ShapeType GetShapeType() const;

		/*****************************************************************//*!
		\brief
			Get the collision layer of the body. NON_MOVING, MOVING, NON_COLLIDABLE
		\return
			collision layer value.
		*//******************************************************************/
		Layers GetCollisionLayer() const;

		/*****************************************************************//*!
		\brief
			Get the position of the body.
		\return
			position value.
		*//******************************************************************/
		Vec3 GetPosition() const;

		/*****************************************************************//*!
		\brief
			Get the scale of the body.
		\return
			scale value.
		*//******************************************************************/
		Vec3 GetScale() const;

		/*****************************************************************//*!
		\brief
			Get the rotation of the body.
		\return
			rotation value.
		*//******************************************************************/
		JPH::Quat GetRotation() const;

		/*****************************************************************//*!
		\brief
			Get the linear velocity of the body.
		\return
			linear velocity value.
		*//******************************************************************/
		Vec3 GetLinearVelocity() const;

		/*****************************************************************//*!
		\brief
			Get the angular velocity of the body.
		\return
			angular velocity value.
		*//******************************************************************/
		Vec3 GetAngularVelocity() const;

		/*****************************************************************//*!
		\brief
			Get the previous transform of the body.
		\return
			previous transform value.
		*//******************************************************************/
		const TransformValues& GetPrevTrans();

		/*****************************************************************//*!
		\brief
			Check if the body is a trigger or not.
		\return
			true if the sensor is true, else false.
		*//******************************************************************/
		bool IsTrigger() const;

		/*****************************************************************//*!
		\brief
			Set the motion type of the body. DYNAMIC, KINEMATIC, STATIC
		\param type
			motion type value.
		*//******************************************************************/
		void SetMotionType(JPH::EMotionType type);

		/*****************************************************************//*!
		\brief
			Set the shape type of the body. BOX, EMPTY
		\param type
			shape type value.
		*//******************************************************************/
		void SetShapeType(ShapeType type);

		/*****************************************************************//*!
		\brief
			Set the collision layer of the body. NON_MOVING, MOVING, NON_COLLIDABLE
		\param layer
			collision layer value.
		*//******************************************************************/
		void SetCollisionLayer(Layers layer);

		/*****************************************************************//*!
		\brief
			Set the gravity factor of the body.
		\param gravity
			gravity factor value.
		*//******************************************************************/
		void SetGravityFactor(float gravity);

		/*****************************************************************//*!
		\brief
			Set the position of the body.
		\param pos
			position value.
		*//******************************************************************/
		void SetPosition(const Vec3& pos);

		/*****************************************************************//*!
		\brief
			Set the rotation of the body.
		\param rot
			rotation value.
		*//******************************************************************/
		void SetRotation(const Vec3& rot);

		/*****************************************************************//*!
		\brief
			Set the scale of the body.
		\param rot
			scale value.
		*//******************************************************************/
		void SetScale(const Vec3& scale);

		/*****************************************************************//*!
		\brief
			Set the linear velocity of the body.
		\param vel
			linear velocity value.
		*//******************************************************************/
		void SetLinearVelocity(const Vec3& vel);

		/*****************************************************************//*!
		\brief
			Add linear velocity to the current linear velocity of the body.
		\param vel
			linear velocity value to add.
		*//******************************************************************/
		void AddLinearVelocity(const Vec3& vel);

		/*****************************************************************//*!
		\brief
			Set the angular velocity of the body.
		\param vel
			angular velocity value.
		*//******************************************************************/
		void SetAngularVelocity(const Vec3& vel);

		/*****************************************************************//*!
		\brief
			Set the previous transform of the body.
		\param val
			previous transform value.
		*//******************************************************************/
		void SetPrevTrans(const TransformValues& val);

		/*****************************************************************//*!
		\brief
			Set the degree of freedom on the X axis.
		\param vel
			true if lock, false if unlock
		*//******************************************************************/
		void SetLockRotationX(bool val);

		/*****************************************************************//*!
		\brief
			Set the degree of freedom on the Y axis.
		\param vel
			true if lock, false if unlock
		*//******************************************************************/
		void SetLockRotationY(bool val);

		/*****************************************************************//*!
		\brief
			Set the degree of freedom on the Z axis.
		\param vel
			true if lock, false if unlock
		*//******************************************************************/
		void SetLockRotationZ(bool val);

		/*****************************************************************//*!
		\brief
			Set the degree of freedom to the body
		\param val
			degree of freedom value.
		*//******************************************************************/
		void SetDOF(JPH::EAllowedDOFs val);

		/*****************************************************************//*!
		\brief
			Set the is trigger value to the body.
		\param val
			trigger value.
		*//******************************************************************/
		void SetIsTrigger(bool val);

		void AddImpulse(const Vec3& dir, float power);

		void MoveTo(const Vec3& pos, float time);

		/*****************************************************************//*!
		\brief
			Update the jolt body before the physics update..
		*//******************************************************************/
		void UpdateBody();

		/*****************************************************************//*!
		\brief
			Update the entity's transform after the physics update.
		*//******************************************************************/
		void UpdateEntity();

		//void EditorDraw() override;
	};

	/*****************************************************************//*!
	\brief
		Register the Jolt Default Allocator, Factory, and Types needed
		to use any of the Jolt Types.
	*//******************************************************************/
	void JoltRegister();
}