#include "Engine/NavMeshAgent.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"

namespace navmesh
{
	int const NavMeshAgentSystem::MAX_AGENT_COUNT = 256;
	float const NavMeshAgentSystem::MAX_AGENT_RADIUS = 1.f;
	int const NavMeshAgentComp::WALKABLE_HEIGHT = 4;
	int const NavMeshAgentComp::WALKABLE_RADIUS = 2;
	int const NavMeshAgentComp::WALKABLE_CLIMB = 2;

	

	NavMeshAgentComp::NavMeshAgentComp()
		: agent{ nullptr }
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

	void NavMeshAgentComp::SetAgent(dtCrowdAgent* agentPtr)
	{
		agent = agentPtr;
	}

	void NavMeshAgentComp::SetTargetPos(Vec3 const& pos)
	{
		if (!agent)
			return;

		dtCrowd* crowdSystem{ ecs::GetSystem<NavMeshAgentSystem>()->GetCrowdSystem() };
		const dtNavMeshQuery* query{crowdSystem->getNavMeshQuery()};
		dtQueryFilter filter;
		filter.setIncludeFlags(PolyFlags::WALKABLE);  // Include all polygon flags

		float halfExtent[3]{ 100.f, 100.f, 100.f };
		dtPolyRef nearestRef{};
		float position[3]{ pos.x, pos.y, pos.z };
		float nearestPos[3]{};
		dtStatus status{ query->findNearestPoly(position, halfExtent, &filter, &nearestRef, nearestPos) };
		if (dtStatusSucceed(status) && nearestRef != 0)
		{
			crowdSystem->requestMoveTarget(agentID, nearestRef, nearestPos);
		}
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

	void NavMeshAgentComp::EditorDraw()
	{

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
			
			params.radius = 0.5f;
			params.height = 0.7f;

			params.maxSpeed = 10.f;
			params.maxAcceleration = 1000.f;

			//Set the position so that the agent is on the navmesh.
			const dtNavMeshQuery* query{ crowdSystem->getNavMeshQuery() };
			dtQueryFilter filter;
			filter.setIncludeFlags(PolyFlags::WALKABLE);  // Include all polygon flags

			float halfExtent[3]{ 100.f, 100.f, 100.f };
			dtPolyRef nearestRef{};
			float nearestPos[3]{};
			dtStatus status{ query->findNearestPoly(pos, halfExtent, &filter, &nearestRef, nearestPos) };
			if (dtStatusSucceed(status) && nearestRef != 0)
			{
				compIter.GetEntity()->GetTransform().SetWorldPosition(Vec3{ nearestPos[0], nearestPos[1], nearestPos[2] });
			}

			int id = crowdSystem->addAgent(nearestPos, &params);
			if (id == -1)
			{
				CONSOLE_LOG(LEVEL_ERROR) << "Couldn't add agent to the agent system.";
				return;
			}
			compIter->SetAgentID(id);
			compIter->SetAgent(crowdSystem->getEditableAgent(id));
		}
	}

	bool NavMeshAgentSystem::PreRun()
	{
		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshAgentComp>() }, endIter{ ecs::GetCompsEnd<NavMeshAgentComp>() }; compIter != endIter; ++compIter)
		{
			compIter->SetAgentPos(compIter.GetEntity()->GetTransform().GetWorldPosition());
			if (compIter.GetEntity()->HasComp<EnemyComponent>())
			{
				auto comp{ compIter.GetEntity()->GetComp<EnemyComponent>() };
				compIter->SetTargetPos(comp->playerReference->GetTransform().GetWorldPosition());
			}
		}

		crowdSystem->update(GameTime::FixedDt(), nullptr);

		for (auto compIter{ ecs::GetCompsActiveBegin<NavMeshAgentComp>() }, endIter{ ecs::GetCompsEnd<NavMeshAgentComp>() }; compIter != endIter; ++compIter)
		{
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