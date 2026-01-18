/******************************************************************************/
/*!
\file	NavMeshAgent.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   16/11/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Declaration of system, components, and objects that are used for the
	navmesh agent.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include <Engine/NavMesh.h>

namespace navmesh 
{
	/*****************************************************************//*!
	\brief
		Necessary Data of the agent.
	*//******************************************************************/
	struct NavMeshAgentData
		: public ISerializeable
	{
		NavMeshParams param;
		float speed;
		float acceleration;
		float baseOffset;
		bool active;

		property_vtable()
	};

	/*****************************************************************//*!
	\brief
		Navmesh agent component.
	*//******************************************************************/
	class NavMeshAgentComp
		: public IRegisteredComponent<NavMeshAgentComp>
		, public IEditorComponent<NavMeshAgentComp>
		, public ecs::IComponentCallbacks
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		NavMeshAgentComp();

		/*****************************************************************//*!
		\brief
			Called when the component is attached.
		*//******************************************************************/
		void OnAttached() override;

		/*****************************************************************//*!
		\brief
			Called when the component is detached.
		*//******************************************************************/
		void OnDetached() override;

		/*****************************************************************//*!
		\brief
			Get the path that the agent is taking.
		\return 
			Data of the path.
		*//******************************************************************/
		NavMeshPath FindPath(const Vec3& targetPos);

		/*****************************************************************//*!
		\brief
			Get the pointer to the agent.
		\return
			Pointer to the agent.
		*//******************************************************************/
		dtCrowdAgent* GetAgent();

		/*****************************************************************//*!
		\brief
			Get the target poisition.
		\return
			target position.
		*//******************************************************************/
		Vec3 GetTargetPos() const;

		/*****************************************************************//*!
		\brief
			Get the agent radius.
		\return
			agent radius.
		*//******************************************************************/
		float GetRadius() const;

		/*****************************************************************//*!
		\brief
			Get the agent height.
		\return
			agent height.
		*//******************************************************************/
		float GetHeight() const;

		/*****************************************************************//*!
		\brief
			Get the agent climb height.
		\return
			agent climb height.
		*//******************************************************************/
		float GetClimbHeight() const;

		/*****************************************************************//*!
		\brief
			Get the agent slope angle that it can climb.
		\return
			agent slope angle.
		*//******************************************************************/
		float GetSlopeAngle() const;

		/*****************************************************************//*!
		\brief
			Get the agent maximum speed.
		\return
			agent maximum speed.
		*//******************************************************************/
		float GetMaxSpeed() const;

		/*****************************************************************//*!
		\brief
			Get the agent maximum acceleration.
		\return
			agent maximum acceleration.
		*//******************************************************************/
		float GetMaxAcceleration() const;

		/*****************************************************************//*!
		\brief
			Get the agent base offset.
		\return
			agent base offset.
		*//******************************************************************/
		float GetBaseOffset() const;

		/*****************************************************************//*!
		\brief
			Get the agent activeness.
		\return
			true if active, else false.
		*//******************************************************************/
		bool IsActive() const;

		/*****************************************************************//*!
		\brief
			Set the pointer to the agent.
		\param agentPtr
			Pointer to the agent.
		*//******************************************************************/
		void SetAgent(dtCrowdAgent* agentPtr);

		/*****************************************************************//*!
		\brief
			Set the target poisition.
		\param pos
			target position.
		*//******************************************************************/
		void SetTargetPos(Vec3 const& pos);

		/*****************************************************************//*!
		\brief
			Set the agent's poisition.
		\param pos
			agent's position.
		*//******************************************************************/
		void SetAgentPos(Vec3 const& pos);

		/*****************************************************************//*!
		\brief
			Set the agent's id.
		\param id
			agent's id.
		*//******************************************************************/
		void SetAgentID(int id);

		/*****************************************************************//*!
		\brief
			Set the agent's radius.
		\param rad
			agent's radius.
		*//******************************************************************/
		void SetRadius(float rad);

		/*****************************************************************//*!
		\brief
			Set the agent's height.
		\param height
			agent's height.
		*//******************************************************************/
		void SetHeight(float height);

		/*****************************************************************//*!
		\brief
			Set the agent's climb height.
		\param climbHeight
			agent's climb height.
		*//******************************************************************/
		void SetClimbHeight(float climbHeight);

		/*****************************************************************//*!
		\brief
			Set the agent's slope angle.
		\param angle
			agent's slope angle.
		*//******************************************************************/
		void SetSlopeAngle(float angle);

		/*****************************************************************//*!
		\brief
			Set the agent's maximum speed.
		\param speed
			agent's maximum speed.
		*//******************************************************************/
		void SetMaxSpeed(float speed);

		/*****************************************************************//*!
		\brief
			Set the agent's maximum acceleration.
		\param acceleration
			agent's maximum acceleration.
		*//******************************************************************/
		void SetMaxAcceleration(float acceleration);

		/*****************************************************************//*!
		\brief
			Set the agent's base offset.
		\param acceleration
			agent's base offset.
		*//******************************************************************/
		void SetBaseOffset(float offset);

		/*****************************************************************//*!
		\brief
			Set the agent's activeness.
		\param val
			agent's activeness.
		*//******************************************************************/
		void SetActive(bool val);

		void Deserialize(Deserializer& reader) override;

	private:
		//data of the agent.
		NavMeshAgentData agentData;
		//pointer to the agent in the crowd system.
		dtCrowdAgent* agent;
		//id of the agent in the crowd system.
		int agentID;

		void SetAgentParam();
		void SetAgentParam(dtCrowdAgentParams const& param);
		virtual void EditorDraw() override;

	public:
		property_vtable()
	};

	/*****************************************************************//*!
	\brief
		Navmesh agent system.
	*//******************************************************************/
	class NavMeshAgentSystem
		: public ecs::System<NavMeshAgentSystem>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		NavMeshAgentSystem();

		void OnAdded() override;
		bool PreRun() override;
		void OnRemoved() override;

		/*****************************************************************//*!
		\brief
			Get the pointer to the crowd system.
		*//******************************************************************/
		dtCrowd* GetCrowdSystem();

		static const int MAX_AGENT_COUNT;
		static const float MAX_AGENT_RADIUS;

	private:
		dtCrowd* crowdSystem;
	};
}
property_begin(navmesh::NavMeshAgentComp)
{
	property_var(agentData)
}
property_vend_h(navmesh::NavMeshAgentComp)
property_begin(navmesh::NavMeshAgentData)
{
	property_var(param),
	property_var(speed),
	property_var(acceleration),
	property_var(baseOffset),
	property_var(active)
}
property_vend_h(navmesh::NavMeshAgentData)