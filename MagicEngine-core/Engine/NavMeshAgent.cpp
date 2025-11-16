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
		NavMeshHit hit{};
		if (SamplePosition(pos, hit, 100.f, WALKABLE))
		{
			float hitPos[3]{ hit.position.x, hit.position.y, hit.position.z };
			crowdSystem->requestMoveTarget(agentID, hit.poly, hitPos);
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

			params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_SEPARATION | DT_CROWD_OBSTACLE_AVOIDANCE;

			int id{-1};
			NavMeshHit hit{};
			if (SamplePosition(transPos, hit, 100.f, PolyFlags::WALKABLE))
			{
				//Update the entity transform.
				compIter.GetEntity()->GetTransform().SetWorldPosition(Vec3{hit.position});

				//Create agent.
				float nearestPos[3]{ hit.position.x, hit.position.y, hit.position.z };
				int id = crowdSystem->addAgent(nearestPos, &params);
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
			if (compIter.GetEntity()->HasComp<EnemyComponent>())
			{
				auto comp{ compIter.GetEntity()->GetComp<EnemyComponent>() };
				compIter->SetTargetPos(comp->playerReference->GetTransform().GetWorldPosition());
			}
		}

		crowdSystem->update(GameTime::FixedDt(), nullptr);

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