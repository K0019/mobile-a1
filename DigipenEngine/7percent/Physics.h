/******************************************************************************/
/*!
\file   Physics.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the interface file for the Physics system and component, which implement
  values and behaviors such as velocity, mass, gravity and collision resolution.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "EntityLayers.h"

/*****************************************************************//*!
\class PointTestComp
\brief
	Debug component used to help visualize collision points.
*//******************************************************************/
class PointTestComp : public IRegisteredComponent<PointTestComp>
{
public:

private:

	property_vtable()
};
property_begin(PointTestComp)
{
}
property_vend_h(PointTestComp)

namespace Physics
{
	/* Forward declarations */
	class ColliderComp;
	struct CollisionData;
	struct CollisionEventData;
	enum class PHYSICS_COMP_FLAG;

	using PhysicsCompFlags = MaskTemplate<PHYSICS_COMP_FLAG>;

	/*****************************************************************//*!
	\brief
		Enums that differentiates flags within the physics component.
	*//******************************************************************/
#define D_PHYSICS_COMP_FLAG \
X(IS_DYNAMIC, "Is Dynamic") \
X(ENABLE_GRAVITY, "Enable Gravity") \
X(ROTATION_LOCKED, "Lock Rotation")

#define X(name, str) name,
	enum class PHYSICS_COMP_FLAG : int
	{
		D_PHYSICS_COMP_FLAG

		TOTAL,
		ALL = TOTAL
	};
#undef X

	/*****************************************************************//*!
	\class PhysicsCompParams
	\brief
		Contains parameters to specify the attributes of a PhysicsComp.
	*//******************************************************************/
	struct PhysicsCompParams
	{
	public:
		//! Whether velocity is applied to the object.
		bool isDynamic{ true };
		//! The amount of mass the object has.
		float mass{ 1.0f };
		//! The restitution coefficient of the object.
		float restitutionCoeff{ 0.4f };
		//! The friction coefficient of the object.
		float frictionCoeff{ 0.4f };
	};

	/*****************************************************************//*!
	\class PhysicsComp
	\brief
		This component class contains physics data such as velocity, restitution coefficients and others.
	*//******************************************************************/
	class PhysicsComp : public IRegisteredComponent<PhysicsComp>
#ifdef IMGUI_ENABLED
		, IEditorComponent<PhysicsComp>
#endif
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructs a physics component with the provided parameters.
		\param params
			The parameters that this physics component will have.
		*//******************************************************************/
		PhysicsComp(const PhysicsCompParams& params = {});

		/*****************************************************************//*!
		\brief
			Applies a momentary impulse at a point relative to the entity's location, affecting this component's velocity.
		\param impulse
			The momentary impulse to be applied.
		\param relativePoint
			The point relative to this entity's location where the impulse is applied at.
		\param momentOfInertia
			The moment of inertia of this entity, obtained via ColliderComp.
		\param canRotate
			Whether the collision component attached to the entity can rotate.
		*//******************************************************************/
		void ApplyImpulse(const Vector2& impulse, const Vector2& relativePoint, float momentOfIntertia, bool canRotate = true);

		/*****************************************************************//*!
		\brief
			Gets this component's velocity.
		\return
			Const reference to this component's velocity vector.
		*//******************************************************************/
		const Vector2& GetVelocity() const;

		/*****************************************************************//*!
		\brief
			Sets this component's velocity.
		\param newVel
			The new velocity vector.
		*//******************************************************************/
		void SetVelocity(const Vector2& newVel);

		/*****************************************************************//*!
		\brief
			Adds a velocity vector to this component's velocity.
		\param addVel
			The velocity vector to be added.
		*//******************************************************************/
		void AddVelocity(const Vector2& addVel);

		/*****************************************************************//*!
		\brief
			Gets this component's angular velocity.
		\return
			This component's angular velocity.
		*//******************************************************************/
		float GetAngVelocity() const;

		/*****************************************************************//*!
		\brief
			Sets this component's angular velocity.
		\param newAngVel
			The new angular velocity.
		*//******************************************************************/
		void SetAngVelocity(float newAngVel);

		/*****************************************************************//*!
		\brief
			Adds an angular velocity amount to this component's angular velocity.
		\param addVel
			The amount of angular velocity to be added.
		*//******************************************************************/
		void AddAngVelocity(float addAngVel);

		/*****************************************************************//*!
		\brief
			Gets this component's mass.
		\return
			This component's mass.
		*//******************************************************************/
		float GetMass() const;

		/*****************************************************************//*!
		\brief
			Gets the reciprocal of this component's mass.
		\return
			The reciprocal of this component's mass.
		*//******************************************************************/
		float GetMassReciprocal() const;

		/*****************************************************************//*!
		\brief
			Gets this component's restitution coefficient.
		\return
			This component's restitution coefficient.
		*//******************************************************************/
		float GetRestitutionCoeff() const;

		/*****************************************************************//*!
		\brief
			Gets this component's friction coefficient.
		\return
			This component's friction coefficient.
		*//******************************************************************/
		float GetFrictionCoeff() const;

		/*****************************************************************//*!
		\brief
			Gets a flag of this component.
		\param flag
			The flag to check.
		\return
			The flag's value.
		*//******************************************************************/
		bool GetFlag(PHYSICS_COMP_FLAG flag) const;

		/*****************************************************************//*!
		\brief
			Sets a flag of this component.
		\param flag
			The flag to set.
		\param value
			The flag's new value.
		*//******************************************************************/
		void SetFlag(PHYSICS_COMP_FLAG flag, bool value);

		/*****************************************************************//*!
		\brief
			Gets whether this component is dynamic, meaning whether velocity
			affects the entity's position over time.
		\return
			True if this component is dynamic. False otherwise.
		*//******************************************************************/
		bool IsDynamic() const;

		/*****************************************************************//*!
		\brief
			Gets whether this component is gravity enabled.
		\return
			True if this component is gravity enabled. False otherwise.
		*//******************************************************************/
		bool IsGravityEnabled() const;

		/*****************************************************************//*!
		\brief
			Sets whether this component is dynamic.
		\param isDynamic
			Whether this component is set to be dynamic or not.
		*//******************************************************************/
		void SetIsDynamic(bool isDynamic);

		/*****************************************************************//*!
		\brief
			Gets whether this component is rotation locked, meaning angular
			velocity is not applied to the entity's rotation.
		\return
			True if this component is dynamic. False otherwise.
		*//******************************************************************/
		bool IsRotationLocked() const;

		/*****************************************************************//*!
		\brief
			Sets whether this component is rotation locked.
		\param isDynamic
			Whether this component is set to be rotation locked or not.
		*//******************************************************************/
		void SetIsRotationLocked(bool isRotationLocked);

	private:
		//! Bitflags of each physics component attribute.
		PhysicsCompFlags flags;

		//! The mass of this component.
		float mass;
		//! The restitution coefficient of this component.
		float restitutionCoeff; // Coefficient of restitution; Elasticity. 1 -> Fully elastic. 0 -> Not elastic at all.
		//! The friction coefficient of this component.
		float frictionCoeff; // Coefficient of friction. 1 -> Equal to applied force; stop on a dime. 0 -> No friction; icy.
		//! The velocity of this component.
		Vector2 velocity;
		//! The angular velocity of this component.
		float angVelocity;

	private:
#ifdef IMGUI_ENABLED
		/*****************************************************************//*!
		\brief
			Draws a physics component to the ImGui editor window.
		\param comp
			A reference to the physics component to be drawn.
		*//******************************************************************/
		static void EditorDraw(PhysicsComp& comp);
#endif

	public:
		// Custom serialization support (for the bitset)
		void Serialize(Serializer& writer) const override;
		void Deserialize(Deserializer& reader) override;

	private:
		property_vtable()
	};

	/*****************************************************************//*!
	\class PhysicsSystem
	\brief
		This system class implements physics behaviors such as gravity.
	*//******************************************************************/
	class PhysicsSystem : public ecs::SystemOperatingByLayer<PhysicsSystem, PhysicsComp>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructs a PhysicsSystem.
		*//******************************************************************/
		PhysicsSystem();

		/*****************************************************************//*!
		\brief
			This is called by ecs when this system is registered.
			In here we subscribe to the collision system's OnCollision event.
		*//******************************************************************/
		void OnAdded() override;

		/*****************************************************************//*!
		\brief
			This is called by ecs when this system is unregistered.
			In here we unsubscribe from the collision system's OnCollision event.
		*//******************************************************************/
		void OnRemoved() override;

		/*****************************************************************//*!
		\brief
			This is called by ecs right before this system starts processing entities.
			In here, we set our dt variable with an upper limit of 1/60fps to prevent tunnelling.
		\return
			True. The system will process components every time.
		*//******************************************************************/
		bool PreRun() override;

		/*****************************************************************//*!
		\brief
			Resolves physics changes when a collision happens, such as velocity changes.
			This is called by the CollisionSystem when a collision happens.
		\param refComp
			The ColliderComp from which collision parameters are relative to.
		\param refPhysComp
			The PhysicsComp of the reference entity.
		\param otherComp
			The ColliderComp which the reference entity is colliding with.
		\param otherPhysComp
			The PhysicsComp of the entity which the reference entity is colliding with.
		\param collisionData
			The collision parameters.
		*//******************************************************************/
		void ProcessCollision(ColliderComp& refComp, ecs::CompHandle<PhysicsComp> refPhysComp,
			ColliderComp& otherComp, ecs::CompHandle<PhysicsComp> otherPhysComp, const CollisionData* collisionData);

	private:
		/*****************************************************************//*!
		\brief
			Updates a PhysicsComp and the entity with physics calculations.
		\param physComp
			The PhysicsComp to be updated.
		*//******************************************************************/
		void UpdatePhysComp(PhysicsComp& physComp);

		/*****************************************************************//*!
		\brief
			Informs the physics system that a collision occured.
			Temporary until messaging system is able to store functions bound to objects.
		\param param
			The CollisionEventData struct.
		*//******************************************************************/
		static void OnCollisionOccured(const CollisionEventData& param);

	private:
		//! The strength of gravity that this system applies to all PhysicsComp that it processes.
		float gravity;

		//! This update's deltatime.
		float dt;

		//! The center of the physics simulation area
		Vector2 simulationCenter;
		//! The half length of the physics simulation area
		float simulationHalfLength;

		//! For resolving physics collisions, this is substituted for cases where an entity has a ColliderComp but does not have a PhysicsComp.
		PhysicsComp emptyComp;
	};

	/*****************************************************************//*!
	\class PhysicsVelocityDebugSystem
	\brief
		A system that renders lines to indicate velocity of physics components.
	*//******************************************************************/
	class PhysicsVelocityDebugSystem : public ecs::System<PhysicsVelocityDebugSystem, PhysicsComp>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		PhysicsVelocityDebugSystem();

	private:
		/*****************************************************************//*!
		\brief
			Render a line at a PhysicsComp to indicate its velocity.
		\param comp
			The PhysicsComp to be rendered.
		*//******************************************************************/
		void RenderComp(PhysicsComp& comp);
	};

}

property_begin(Physics::PhysicsComp)
{
}
property_vend_h(Physics::PhysicsComp)
