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
X(USE_GRAVITY, "Use Gravity")
//X(ROTATION_LOCKED, "Lock Rotation")

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
		// Is Kinematic and Use Gravity
		PhysicsCompFlags flags;
		// linear velocity
		Vec3 linearVel;

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

	bool OverlapSphere(std::vector<BoxColliderComp*>& outColliders, const Vec3& origin, float radius, EntityLayersMask layers = EntityLayersMask{});
} 
property_begin(physics::PhysicsComp)
{
	property_var(linearVel)
}
property_vend_h(physics::PhysicsComp)


