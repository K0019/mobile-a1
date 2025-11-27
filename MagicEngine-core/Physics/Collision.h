/******************************************************************************/
/*!
\file   Collision.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Contains the definition of collision layers and the mapping between the 
	different layers.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "pch.h"
#include "Utilities/MaskTemplate.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"

#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace physics {
	class BoxColliderComp;
	class JoltBodyComp;

	/*****************************************************************//*!
	\brief
		Enums that differentiates flags within the collider component.
	*//******************************************************************/
#define D_COLLIDER_COMP_FLAG \
X(ENABLED, "Enabled") \
X(IS_TRIGGER, "Is Trigger")

#define X(name, str) name,
	enum class COLLIDER_COMP_FLAG : int
	{
		D_COLLIDER_COMP_FLAG

		TOTAL,
		ALL = TOTAL
	};

	enum class ContactTiming
	{
		ENTER,
		STAY,
		EXIT
	};

	/*****************************************************************//*!
	\brief
		Layer that objects can be in, determines which other objects 
		it can collide with. Typically you at least want to have 1 
		layer for moving bodies and 1 layer for static bodies, but 
		you can have more layers if you want. E.g. you could have a 
		layer for high detail collision (which is not used by the physics 
		simulation but only if you do collision testing).
	*//******************************************************************/
	enum class Layers : JPH::ObjectLayer
	{
		NON_MOVING = 0,
		MOVING,
		NON_COLLIDABLE,
		NUM_LAYERS
	};

	/*****************************************************************//*!
	\brief
		Each broadphase layer results in a separate bounding volume 
		tree in the broad phase. You at least want to have a layer for 
		non-moving and moving objects to avoid having to update a tree 
		full of static objects every frame. You can have a 1-on-1 mapping 
		between object layers and broadphase layers (like in this case) 
		but if you have many object layers you'll be creating many broad 
		phase trees, which is not efficient. If you want to fine tune
		your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look 
		at the stats reported on the TTY.
	*//******************************************************************/
	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::BroadPhaseLayer NON_COLLIDABLE(2);
		static constexpr JPH::uint NUM_LAYERS(3);
	};

	/*****************************************************************//*!
	\brief
		Class that determines if two object layers can collide
	*//******************************************************************/
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
	};

	/*****************************************************************//*!
	\brief
		BroadPhaseLayerInterface implementation
		This defines a mapping between object and broadphase layers.
	*//******************************************************************/
	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl();

		virtual JPH::uint GetNumBroadPhaseLayers() const override;

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[+Layers::NUM_LAYERS];
	};

	/*****************************************************************//*!
	\brief
		Class that determines if an object layer can collide with a broadphase layer
	*//******************************************************************/
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
	};

	/*****************************************************************//*!
	\brief
		Contact Listener for the Jolt Physics.
		Writes to the console when certain collision occures and disoccurs
	*//******************************************************************/
	class MyContactListener : public JPH::ContactListener
	{
	public:
		static void Init();

		virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

		static void ClearContactPair();
		static void CallContactFunc();
	private:
		static std::vector<std::pair<std::pair<JPH::BodyID, JPH::BodyID>, ContactTiming>> contactPair;
	};

	using ColliderCompFlag = MaskTemplate<COLLIDER_COMP_FLAG>;

	class BoxColliderComp
		: public IRegisteredComponent<BoxColliderComp>
		, public IEditorComponent<BoxColliderComp>
		, public ecs::IComponentCallbacks
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		BoxColliderComp();

	private:
		// Collider component flags
		ColliderCompFlag flags;
		// position difference from the center of the object.
		Vec3 center;
		// size difference from the object's scale.
		Vec3 size;

		std::array<std::function<void()>, 6> contactFunctions;

	private:
		void InitializeJoltBodyComp(JoltBodyComp* comp);

	public:
		/*****************************************************************//*!
		\brief
			Called when the component is attached. It creates a body if the
			entity doesn't have a physics compoenent and set the shape to
			be a box.
		*//******************************************************************/
		void OnAttached() override;

		/*****************************************************************//*!
		\brief
			Called when the component is detached. It destroys the body if the 
			entity doesn't have a physics component. If not, set the shape of 
			the body to be empty.
		*//******************************************************************/
		void OnDetached() override;

		/*****************************************************************//*!
		\brief
			Gets the value of the corresponding flag.
		\param flag
			Flag to get the value.
		\return
			Boolean value of the flag.
		*//******************************************************************/
		bool GetFlag(COLLIDER_COMP_FLAG flag) const;

		/*****************************************************************//*!
		\brief
			Set a value to a certain flag.
		\param flag
			Flag to set the value
		\param val
			Boolean value to set to the flag.
		*//******************************************************************/
		void SetFlag(COLLIDER_COMP_FLAG flag, bool val);

		/*****************************************************************//*!
		\brief
			Get the position difference of the collider.
		\return
			The value of center.
		*//******************************************************************/
		const Vec3& GetCenter() const;

		/*****************************************************************//*!
		\brief
			Set the position difference of the collider.
		\param val
			The value of center to set.
		*//******************************************************************/
		void SetCenter(const Vec3& val);

		/*****************************************************************//*!
		\brief
			Get the scale factor of the collider.
		\return
			The value of size.
		*//******************************************************************/
		const Vec3& GetSize() const;

		/*****************************************************************//*!
		\brief
			Set the scale factor of the collider.
		\param val
			The value of size to set.
		*//******************************************************************/
		void SetSize(const Vec3& val);

		void Serialize(Serializer& writer) const override;
		void Deserialize(Deserializer& reader) override;

		// ===== Lua helpers =====
		bool GetEnabled() const
		{
			return GetFlag(COLLIDER_COMP_FLAG::ENABLED);
		}
		bool IsTrigger() const
		{
			return GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER);
		}

		void SetEnabled(bool v)
		{
			SetFlag(COLLIDER_COMP_FLAG::ENABLED, v);
		}
		void SetIsTrigger(bool v)
		{
			SetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER, v);
		}

	private:
		/*****************************************************************//*!
		\brief
			Draws a box collider component to the ImGui editor window.
		*//******************************************************************/
		virtual void EditorDraw() override;

	public:
		property_vtable()
	};
}
property_begin(physics::BoxColliderComp)
{
	property_var(center),
	property_var(size)
}
property_vend_h(physics::BoxColliderComp)