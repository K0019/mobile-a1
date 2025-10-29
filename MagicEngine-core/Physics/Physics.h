/******************************************************************************/
/*!
\file   Physics.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/


#include "Engine/EntityLayers.h"
#include "Physics/JoltPhysics.h"
#include "Utilities/MaskTemplate.h"
#include "ECS/IRegisteredComponent.h"
#include "Game/IGameComponentCallbacks.h"

namespace physics {
	class ColliderComp;
	enum class PHYSICS_COMP_FLAG;

	using PhysicsCompFlags = MaskTemplate<PHYSICS_COMP_FLAG>;

	/*****************************************************************//*!
	\brief
		Enums that differentiates flags within the physics component.
	*//******************************************************************/
#define D_PHYSICS_COMP_FLAG \
X(IS_KINEMATIC, "Is Kinematic") \
X(USE_GRAVITY, "Use Gravity") \
X(ROTATION_LOCKED_X, "Lock Rotation X") \
X(ROTATION_LOCKED_Y, "Lock Rotation Y") \
X(ROTATION_LOCKED_Z, "Lock Rotation Z") \
X(ENABLED, "Enabled")

#define X(name, str) name,
	enum class PHYSICS_COMP_FLAG : int
	{
		D_PHYSICS_COMP_FLAG

		TOTAL,
		ALL = TOTAL
	};
#undef X

	class PhysicsComp
		: public IRegisteredComponent<PhysicsComp>
		, public IEditorComponent<PhysicsComp>
		, public ecs::IComponentCallbacks
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		PhysicsComp();

	private:
		// Physics component flags
		PhysicsCompFlags flags;
		// linear velocity
		Vec3 linearVel;
		// angular velocity
		Vec3 angularVel;

	public:

		/*****************************************************************//*!
		\brief
			Called when the component is attached. If the entity has a collider
			component, get the body id of the body in the collider component.
			If not, create a body.
		*//******************************************************************/
		void OnAttached() override;

		/*****************************************************************//*!
		\brief
			Called when the component is detached. If the entity has a collider
			component, set the motion type to be static. If not, create a body.
			Destroy the body.
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
		bool GetFlag(PHYSICS_COMP_FLAG flag) const;

		/*****************************************************************//*!
		\brief
			Set a value to a certain flag.
		\param flag
			Flag to set the value
		\param val
			Boolean value to set to the flag.
		*//******************************************************************/
		void SetFlag(PHYSICS_COMP_FLAG flag, bool val);

		/*****************************************************************//*!
		\brief
			Gets the value of the linear velocity.
		\return
			3d vector that represents the linear velocity.
		*//******************************************************************/
		const Vec3& GetLinearVelocity() const;

		/*****************************************************************//*!
		\brief
			Set the value of the linear velocity.
		\param flag
			Flag to set the value
		\param vel
			3d vector value to set to the linear velocity.
		*//******************************************************************/
		void SetLinearVelocity(const Vec3& vel);

		/*****************************************************************//*!
		\brief
			Gets the value of the angular velocity.
		\return
			3d vector that represents the angular velocity.
		*//******************************************************************/
		const Vec3& GetAngularVelocity() const;

		/*****************************************************************//*!
		\brief
			Set the value of the angular velocity.
		\param vel
			3d vector value to set to the angular velocity in degrees.
		*//******************************************************************/
		void SetAngularVelocity(const Vec3& vel);

	private:
		/*****************************************************************//*!
		\brief
			Draws a physics component to the ImGui editor window.
		*//******************************************************************/
		virtual void EditorDraw() override;

	public:
		void Serialize(Serializer& writer) const override;
		void Deserialize(Deserializer& reader) override;

	public:
		property_vtable()
	};

	class PhysicsSystem
		: public ecs::System<PhysicsSystem>
	{
	public:
		void OnAdded() override;
		bool PreRun() override;
	};

	/*****************************************************************//*!
	\brief
		Check if there's colliders that overlap the given sphere.
	\param outColliders
		out param to store all the pointers of the colliders that overlaped.
	\param origin
		center of the sphere.
	\param radius
		radius of the sphere.
	\param layers
		entity layers to check overlap. By default, it checks all the layers.
	\return
		true if there is an overlap. False if not.
	*//******************************************************************/
	bool OverlapSphere(std::vector<BoxColliderComp*>& outColliders, const Vec3& origin, float radius, EntityLayersMask layers = EntityLayersMask{});

	/*****************************************************************//*!
	\brief
		Check if there's colliders that overlap the given box.
	\param outColliders
		out param to store all the pointers of the colliders that overlaped.
	\param origin
		center of the box.
	\param halfExtent
		half of the scale of the box.
	\param orientation
		rotation of the box. **Currently not working**
	\param layers
		entity layers to check overlap. By default, it checks all the layers.
	\return
		true if there is an overlap. False if not.
	*//******************************************************************/
	bool OverlapBox(std::vector<BoxColliderComp*>& outColliders, const Vec3& origin, const Vec3& halfExtent, const Vec3& orientation = Vec3{}, EntityLayersMask layers = EntityLayersMask{});
} 
property_begin(physics::PhysicsComp)
{
	property_var(linearVel),
	property_var(angularVel)
}
property_vend_h(physics::PhysicsComp)


