/******************************************************************************/
/*!
\file	NavMeshAgent.cpp
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   16/11/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Definition of system, components, and objects that are used for the
	navmesh agent.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/NavMeshAgent.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Editor/Containers/GUICollection.h"

namespace navmesh
{
	int const NavMeshAgentSystem::MAX_AGENT_COUNT = 256;
	float const NavMeshAgentSystem::MAX_AGENT_RADIUS = 1.f;

	NavMeshAgentComp::NavMeshAgentComp()
		: agentData{}
		, agent{ nullptr }
		, agentID{ -1 }
	{
	}

	void NavMeshAgentComp::OnAttached()
	{
	}

	void NavMeshAgentComp::OnDetached()
	{
		if (!agent || agentID == -1 || !ecs::GetSystem<NavMeshAgentSystem>())
			return;

		//Get Crowd system to add the agent.
		dtCrowd* crowdSystem{ ecs::GetSystem<NavMeshAgentSystem>()->GetCrowdSystem() };

		if (!crowdSystem)
			return;

		crowdSystem->removeAgent(agentID);
	}

	NavMeshPath NavMeshAgentComp::FindPath()
	{
		NavMeshPath result{ std::vector<Vec3>{}, NavMeshPathStatus::PATH_INVALID };
		if (!agent || agent->state == DT_CROWDAGENT_STATE_INVALID)
			return result;

		if (agent->state == DT_CROWDAGENT_STATE_WALKING)
			result.status = agent->partial ? NavMeshPathStatus::PATH_PARTIAL : NavMeshPathStatus::PATH_COMPLETE;

		for (int count{}; count < agent->ncorners; ++count)
		{
			Vec3 corner{ agent->cornerVerts[3 * count], agent->cornerVerts[3 * count + 1], agent->cornerVerts[3 * count + 2] };
			result.corners.push_back(corner);
		}

		return result;
	}

	dtCrowdAgent* NavMeshAgentComp::GetAgent()
	{
		return agent;
	}

	Vec3 NavMeshAgentComp::GetTargetPos() const
	{
		if (!agent)
			return Vec3{};
		return Vec3{ agent->targetPos[0], agent->targetPos[1], agent->targetPos[2] };
	}

	float NavMeshAgentComp::GetRadius() const
	{
		return agentData.param.radius;
	}

	float NavMeshAgentComp::GetHeight() const
	{
		return agentData.param.height;
	}

	float NavMeshAgentComp::GetClimbHeight() const
	{
		return agentData.param.height;
	}

	float NavMeshAgentComp::GetSlopeAngle() const
	{
		return agentData.param.angle;
	}

	float NavMeshAgentComp::GetMaxSpeed() const
	{
		return agentData.speed;
	}

	float NavMeshAgentComp::GetMaxAcceleration() const
	{
		return agentData.acceleration;
	}
	
	bool NavMeshAgentComp::IsActive() const
	{
		return agentData.active;
	}

	void NavMeshAgentComp::SetAgent(dtCrowdAgent* agentPtr)
	{
		agent = agentPtr;
	}

	void NavMeshAgentComp::SetTargetPos(Vec3 const& pos)
	{
		if (!agent)
			return;

		dtCrowd* crowdSystem{ ecs::GetSystem<NavMeshAgentSystem>()->GetCrowdSystem() };
		NavMeshHit hit{};
		if (SamplePosition(pos, hit, 100.f, +PolyFlags::WALKABLE))
		{
			float hitPos[3]{ hit.position.x, hit.position.y, hit.position.z };
			crowdSystem->requestMoveTarget(agentID, hit.poly, hitPos);
		}
	}

	void NavMeshAgentComp::SetRadius(float radius)
	{
		agentData.param.radius = radius;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetHeight(float height)
	{
		agentData.param.height = height;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetClimbHeight(float climbHeight)
	{
		agentData.param.climb = climbHeight;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetSlopeAngle(float angle)
	{
		agentData.param.angle = angle;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetMaxSpeed(float speed)
	{
		agentData.speed = speed;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetMaxAcceleration(float acceleration)
	{
		agentData.acceleration = acceleration;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetActive(bool val)
	{
		agentData.active = val;
		dtCrowdAgentParams params{};

		params.radius = agentData.param.radius;
		params.height = agentData.param.height;
		params.maxSpeed = 0.f;
		params.maxAcceleration = 0.f;

		params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;
		SetAgentParam(params);
	}

	void NavMeshAgentComp::SetAgentPos(Vec3 const& pos)
	{
		if (!agent)
			return;

		agent->npos[0] = pos.x;
		agent->npos[1] = pos.y;
		agent->npos[2] = pos.z;
	}

	void NavMeshAgentComp::SetAgentID(int id)
	{
		agentID = id;
	}

	void NavMeshAgentComp::SetAgentParam()
	{
		auto crowdSystem{ ecs::GetSystem<NavMeshAgentSystem>()->GetCrowdSystem() };
		if (!crowdSystem)
			return;

		dtCrowdAgentParams params{};

		params.radius = agentData.param.radius;
		params.height = agentData.param.height;
		params.maxSpeed = agentData.speed;
		params.maxAcceleration = agentData.acceleration;

		params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;

		crowdSystem->updateAgentParameters(agentID, &params);
	}

	void NavMeshAgentComp::SetAgentParam(dtCrowdAgentParams const& param)
	{
		auto crowdSystem{ ecs::GetSystem<NavMeshAgentSystem>()->GetCrowdSystem() };
		if (!crowdSystem)
			return;

		crowdSystem->updateAgentParameters(agentID, &param);
	}

	void NavMeshAgentComp::EditorDraw()
	{
		if (gui::Checkbox("Active", &agentData.active))
			SetActive(agentData.active);

		bool radChanged{ gui::VarDefault("Radius", &agentData.param.radius) };
		bool heightChanged{ gui::VarDefault("Height", &agentData.param.height) };
		bool climbChanged{ gui::VarDefault("Climb Height", &agentData.param.climb) };
		bool angleChanged{ gui::VarDefault("Slope Angle", &agentData.param.angle) };
		bool speedChanged{ gui::VarDefault("Max Speed", &agentData.speed) };
		bool accelChanged{ gui::VarDefault("Max Acceleration", &agentData.acceleration) };

		if (agentID != -1 && (radChanged || heightChanged || climbChanged || angleChanged || speedChanged || accelChanged))
			SetAgentParam();
	}

	NavMeshAgentSystem::NavMeshAgentSystem()
		: crowdSystem{ nullptr }
	{
	}

	void NavMeshAgentSystem::OnAdded()
	{
		crowdSystem = dtAllocCrowd();
		if (!crowdSystem)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Couldn't allocate memory for NavMesh Agent System.";
			return;
		}

		crowdSystem->init(MAX_AGENT_COUNT, MAX_AGENT_RADIUS, ecs::GetSystem<NavMeshSystem>()->GetNavMesh());

		//Add the agent to the system.
		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshAgentComp>() }, endIter{ ecs::GetCompsEnd<NavMeshAgentComp>() }; compIter != endIter; ++compIter)
		{
			//Get necessary data.
			const Vec3& transPos{ compIter.GetEntity()->GetTransform().GetWorldPosition()};
			float pos[3]{ transPos.x, transPos.y, transPos.z };
			dtCrowdAgentParams params{};
			
			params.radius = compIter->GetRadius();
			params.height = compIter->GetHeight();

			if (compIter->IsActive())
			{
				params.maxSpeed = compIter->GetMaxSpeed();
				params.maxAcceleration = compIter->GetMaxAcceleration();
			}
			else
			{
				params.maxSpeed = 0.f;
				params.maxAcceleration = 0.f;
			}

			params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;

			int id{-1};
			NavMeshHit hit{};
			if (SamplePosition(transPos, hit, 100.f, +PolyFlags::WALKABLE))
			{
				//Update the entity transform.
				compIter.GetEntity()->GetTransform().SetWorldPosition(Vec3{hit.position});

				//Create agent.
				float nearestPos[3]{ hit.position.x, hit.position.y, hit.position.z };
				id = crowdSystem->addAgent(nearestPos, &params);
				compIter->SetAgentID(id);
				compIter->SetAgent(crowdSystem->getEditableAgent(id));
			}

			if (id == -1)
			{
				CONSOLE_LOG(LEVEL_ERROR) << "Couldn't add agent to the agent system.";
				return;
			}
		}
	}

	bool NavMeshAgentSystem::PreRun()
	{
		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshAgentComp>() }, endIter{ ecs::GetCompsEnd<NavMeshAgentComp>() }; compIter != endIter; ++compIter)
		{
			if (!compIter->GetAgent())
				continue;

			compIter->SetAgentPos(compIter.GetEntity()->GetTransform().GetWorldPosition());
		}

		crowdSystem->update(GameTime::Dt(), nullptr);

		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshAgentComp>() }, endIter{ ecs::GetCompsEnd<NavMeshAgentComp>() }; compIter != endIter; ++compIter)
		{
			if (!compIter->GetAgent())
				continue;

			Vec3 pos{ compIter->GetAgent()->npos[0], compIter->GetAgent()->npos[1], compIter->GetAgent()->npos[2] };
			compIter.GetEntity()->GetTransform().SetWorldPosition(pos);
		}
		return true;
	}

	void NavMeshAgentSystem::OnRemoved()
	{
		dtFreeCrowd(crowdSystem);
	}

	dtCrowd* NavMeshAgentSystem::GetCrowdSystem()
	{
		return crowdSystem;
	}
}