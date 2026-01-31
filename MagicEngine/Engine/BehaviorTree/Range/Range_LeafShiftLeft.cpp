#include "Engine/BehaviorTree/Range/Range_LeafShiftLeft.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"
#include "Engine/NavMeshAgent.h"

NODE_STATUS L_Range_ShiftLeft::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	auto agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
	if (!charComp || !enemyComp || !agentComp)
		return NODE_STATUS::FAILURE;

	if (!enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 playerPos{ enemyComp->playerReference->GetTransform().GetWorldPosition() };
	Vec3 enemyPos{ entity->GetTransform().GetWorldPosition() };
	Vec3 diff{ enemyPos - playerPos };

	float angle{ math::ToDegrees(std::atan2(diff.x, diff.z)) };
	angle += 3;
	Vec3 vec{ std::sin(math::ToRadians(angle)), 0.f, std::cos(math::ToRadians(angle)) };
	vec *= diff.Length();

	agentComp->SetActive(true);
	agentComp->SetTargetPos(playerPos + vec);
	return NODE_STATUS::SUCCESS;
}