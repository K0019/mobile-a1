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
#include "Physics/Physics.h"
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
		if (auto agentSystem{ ecs::GetSystem<NavMeshAgentSystem>() })
		{
			dtCrowd* crowdSystem{ agentSystem->GetCrowdSystem() };
			if (!crowdSystem)
				return;

			ST<Scheduler>::Get()->Add([entity = ecs::GetEntity(this), crowdSystem] {
				//Get necessary data.
				const Vec3& transPos{ entity->GetTransform().GetWorldPosition() };
				float pos[3]{ transPos.x, transPos.y, transPos.z };
				dtCrowdAgentParams params{};

				auto agentComp{ entity->GetComp<NavMeshAgentComp>() };
				if (!agentComp)
					return;
				params.radius = agentComp->GetRadius();
				params.height = agentComp->GetHeight();
				params.maxAcceleration = agentComp->GetMaxAcceleration();
				params.maxSpeed = agentComp->IsActive() ? agentComp->GetMaxSpeed() : 0.f;
				params.collisionQueryRange = params.radius * 5.f;
				params.pathOptimizationRange = params.radius * 30.f;
				params.separationWeight = 2.0f;
				params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;

				int id{ -1 };
				NavMeshHit hit{};
				if (SamplePosition(transPos, hit, 100.f, +PolyFlags::WALKABLE))
				{
					//Create agent.
					float nearestPos[3]{ hit.position.x, hit.position.y, hit.position.z };
					id = crowdSystem->addAgent(nearestPos, &params);
					agentComp->SetAgentID(id);
					agentComp->SetAgent(crowdSystem->getEditableAgent(id));

					//Update the entity transform.
					nearestPos[1] += agentComp->GetBaseOffset();
					entity->GetTransform().SetWorldPosition(Vec3{ nearestPos[0], nearestPos[1], nearestPos[2] });
				}

				if (id == -1)
				{
					CONSOLE_LOG(LEVEL_ERROR) << "Couldn't add agent to the agent system.";
					return;
				}
				});
		}
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

	NavMeshPath NavMeshAgentComp::FindPath(const Vec3& targetPos)
	{
		dtCrowd* crowdSystem{ ecs::GetSystem<NavMeshAgentSystem>()->GetCrowdSystem() };
		NavMeshPath result{ std::vector<Vec3>{}, NavMeshPathStatus::PATH_INVALID };
		if (!agent || !crowdSystem)
			return result;

		const dtNavMeshQuery* navQuery{ crowdSystem->getNavMeshQuery() };
		if (!navQuery)
			return result;

		Vec3 currPos{ ecs::GetEntityTransform(this).GetWorldPosition() };
		dtPolyRef startPoly;
		dtPolyRef endPoly;
		float startPos[3]{};
		float endPos[3]{};
		float startPolyPos[3]{ currPos.x, currPos.y, currPos.z };
		float endPolyPos[3]{ targetPos.x, targetPos.y, targetPos.z };
		const float extent[3]{ 2.f, 4.f, 2.f };

		dtQueryFilter filter;
		filter.setIncludeFlags(static_cast<unsigned short>(+PolyFlags::WALKABLE));

		navQuery->findNearestPoly(startPolyPos, extent, &filter, &startPoly, startPos);
		navQuery->findNearestPoly(endPolyPos, extent, &filter, &endPoly, endPos);

		if (!startPoly || !endPoly)
			return result;

		const int maxPolyCount{ 256 };
		dtPolyRef pathPoly[maxPolyCount]{};
		int pathCount{};

		navQuery->findPath(startPoly, endPoly, startPos, endPos, &filter, pathPoly, &pathCount, maxPolyCount);

		if (pathCount == 0)
			return result;

		const int maxCorners{ 256 };
		float straightPath[3 * maxCorners]{};
		unsigned char straightPathFlag[maxCorners]{};
		dtPolyRef straightPathRefs[maxCorners]{};
		int straightPathCount{};

		navQuery->findStraightPath(startPos, endPos, pathPoly, pathCount, straightPath, straightPathFlag,
			straightPathRefs, &straightPathCount, maxCorners);

		for (int count{}; count < straightPathCount; ++count)
			result.corners.push_back(Vec3(straightPath[3 * count], straightPath[3 * count + 1], straightPath[3 * count + 2]));

		if (pathCount == maxPolyCount && !result.corners.empty())
			result.status = NavMeshPathStatus::PATH_PARTIAL;
		else
			result.status = NavMeshPathStatus::PATH_COMPLETE;

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
	
	float NavMeshAgentComp::GetBaseOffset() const
	{
		return agentData.baseOffset;
	}

	int NavMeshAgentComp::GetAgentID() const
	{
		return agentID;
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

	void NavMeshAgentComp::SetBaseOffset(float offset)
	{
		agentData.baseOffset = offset;
		SetAgentParam();
	}

	void NavMeshAgentComp::SetActive(bool val)
	{
		agentData.active = val;
		dtCrowdAgentParams params{};
		if (agent)
			params = agent->params;

		params.radius = agentData.param.radius;
		params.height = agentData.param.height;
		params.maxSpeed = val ? agentData.speed : 0.f;
		params.maxAcceleration = agentData.acceleration;
		params.collisionQueryRange = params.radius * 5.f;
		params.pathOptimizationRange = params.radius * 30.f;
		params.separationWeight = 2.0f;

		params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;
		SetAgentParam(params);
	}

	void NavMeshAgentComp::Deserialize(Deserializer& reader)
	{
		ISerializeable::Deserialize(reader);
		SetAgentParam();
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
		auto agentSystem{ ecs::GetSystem<NavMeshAgentSystem>() };
		if (!agentSystem)
			return;

		auto crowdSystem{ agentSystem->GetCrowdSystem() };
		if (!crowdSystem || agentID == -1)
			return;

		dtCrowdAgentParams params{};
		if (agent)
			params = agent->params;

		params.radius = agentData.param.radius;
		params.height = agentData.param.height;
		params.maxSpeed = agentData.speed;
		params.maxAcceleration = agentData.acceleration;

		params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;

		params.collisionQueryRange = params.radius * 5.f;
		params.pathOptimizationRange = params.radius * 30.f;
		params.separationWeight = 2.0f;
		crowdSystem->updateAgentParameters(agentID, &params);

	}

	void NavMeshAgentComp::SetAgentParam(dtCrowdAgentParams const& param)
	{
		auto agentSystem{ ecs::GetSystem<NavMeshAgentSystem>() };
		if (!agentSystem)
			return;

		dtCrowd* crowdSystem{ agentSystem->GetCrowdSystem() };
		if (!crowdSystem)
			return;

		crowdSystem->updateAgentParameters(agentID, &param);
	}

	void NavMeshAgentComp::EditorDraw()
	{
		if (gui::Checkbox("Active", &agentData.active))
			SetActive(agentData.active);

		bool offsetChanged{ gui::VarDefault("Base Offset", &agentData.baseOffset) };
		bool radChanged{ gui::VarDefault("Radius", &agentData.param.radius) };
		bool heightChanged{ gui::VarDefault("Height", &agentData.param.height) };
		bool climbChanged{ gui::VarDefault("Climb Height", &agentData.param.climb) };
		bool angleChanged{ gui::VarDefault("Slope Angle", &agentData.param.angle) };
		bool speedChanged{ gui::VarDefault("Max Speed", &agentData.speed) };
		bool accelChanged{ gui::VarDefault("Max Acceleration", &agentData.acceleration) };

		if (agentID != -1 && (offsetChanged || radChanged || heightChanged || climbChanged || angleChanged || speedChanged || accelChanged))
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
			params.maxAcceleration = compIter->GetMaxAcceleration();
			params.maxSpeed = compIter->IsActive() ? compIter->GetMaxSpeed() : 0.f;
			params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;
			params.collisionQueryRange = params.radius * 5.f;
			params.pathOptimizationRange = params.radius * 30.f;
			params.separationWeight = 2.0f;

			int id{-1};
			NavMeshHit hit{};
			if (SamplePosition(transPos, hit, 100.f, +PolyFlags::WALKABLE))
			{
				//Create agent.
				float nearestPos[3]{ hit.position.x, hit.position.y, hit.position.z };
				id = crowdSystem->addAgent(nearestPos, &params);
				compIter->SetAgentID(id);
				compIter->SetAgent(crowdSystem->getEditableAgent(id));

				//Update the entity transform.
				nearestPos[1] += compIter->GetBaseOffset();
				compIter.GetEntity()->GetTransform().SetWorldPosition(Vec3{nearestPos[0], nearestPos[1], nearestPos[2]});
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
			NavMeshHit hit{};
			if (SamplePosition(compIter.GetEntity()->GetTransform().GetWorldPosition(), hit, compIter->GetBaseOffset() + 1.f, +PolyFlags::WALKABLE))
				compIter->SetAgentPos(hit.position);
		}

		crowdSystem->update(GameTime::Dt(), nullptr);

		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshAgentComp>() }, endIter{ ecs::GetCompsEnd<NavMeshAgentComp>() }; compIter != endIter; ++compIter)
		{
			if (!compIter->GetAgent())
				continue;

			if (auto charComp{ compIter.GetEntity()->GetComp<CharacterMovementComponent>() })
			{
				Vec3 vel{ compIter->GetAgent()->nvel[0], compIter->GetAgent()->nvel[1] + compIter->GetBaseOffset(), compIter->GetAgent()->nvel[2] };
				charComp->SetMovementVector(Vec2{ vel.x, vel.z });
			}
			else
			{
				Vec3 pos{ compIter->GetAgent()->npos[0], compIter->GetAgent()->npos[1] + compIter->GetBaseOffset(), compIter->GetAgent()->npos[2] };
				compIter.GetEntity()->GetTransform().SetWorldPosition(pos);
			}
			crowdSystem->resetMoveTarget(compIter->GetAgentID());
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