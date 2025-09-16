/******************************************************************************/
/*!
\file   Physics.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "JoltPhysics.h"
#include "IGameComponentCallbacks.h"

namespace physics {
	class ColliderComp;

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
		// ID to identify the assigned Jolt Body.
		JPH::BodyID bodyID;
	public:

		void OnAttached() override;
		void OnDetached() override;

		JPH::BodyID GetBodyID();

		/*****************************************************************//*!
		\brief
			Get the position of the body in the body interface.
		\return
			Vec3 value that represents the position of the body.
		*//******************************************************************/
		Vec3 const& GetBodyPosition();

	private:
		virtual void EditorDraw() override;

	public:
		property_vtable()
	};

	class PhysicsSystem
		: public ecs::System<PhysicsSystem>
	{
	public:
		bool PreRun() override;
	};
}
property_begin(physics::PhysicsComp)
{

}
property_vend_h(physics::PhysicsComp)


