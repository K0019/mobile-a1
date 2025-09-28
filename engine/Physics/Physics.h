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
	

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "JoltPhysics.h"
#include "MaskTemplate.h"
#include "IGameComponentCallbacks.h"

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
#ifdef IMGUI_ENABLED
		, public IEditorComponent<PhysicsComp> 
#endif
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
		// ID to identify the assigned Jolt Body.
		JPH::BodyID bodyID;
		// entity's transform in the previous frame.
		Transform prevTransform;
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
			Get the body id of the corresponding body.
		\return
			The id of the body.
		*//******************************************************************/
		JPH::BodyID GetBodyID() const;

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
			Set a value to a certain. flag.
		\param flag
			Flag to set the value
		\param val
			Boolean value to set to the flag.
		*//******************************************************************/
		void SetFlag(PHYSICS_COMP_FLAG flag, bool val);

		/*****************************************************************//*!
		\brief
			Get the previous transform values.
		\return
			Transform value that represent the previous transform of the entity.
		*//******************************************************************/
		const Transform& GetPrevTransform() const;

		/*****************************************************************//*!
		\brief
			Set a value to the previous transform.
		\param transform
			The value to set the previous transform.
		*//******************************************************************/
		void SetPrevTransform(const Transform& transform);

		/*****************************************************************//*!
		\brief
			Updates the Jolt Body based on the entity.
		*//******************************************************************/
		void UpdateJoltBody();

		/*****************************************************************//*!
		\brief
			Updates the entity transform based on the Jolt Body.
		*//******************************************************************/
		void UpdateTransform();

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
}
property_begin(physics::PhysicsComp)
{
}
property_vend_h(physics::PhysicsComp)


