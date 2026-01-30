#include "Engine/BehaviorTree/Range/Range_DecoIsFacingPlayer.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"

void D_Range_IsFacingPlayer::OnInitialize()
{
	fieldOfViewAngle = 8;
}

NODE_STATUS D_Range_IsFacingPlayer::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!charComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	if (!enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 rotation{ entity->GetTransform().GetWorldRotation() };
	Vec3 diff{ enemyComp->playerReference->GetTransform().GetWorldPosition() - entity->GetTransform().GetWorldPosition() };
	diff = diff.Normalized();
	Vec3 forward{ std::sin(rotation.y), 0.f, std::cos(rotation.y) };

	float dotProduct{ forward.Dot(diff) };
	float halfAngleRadian{ math::ToRadians(fieldOfViewAngle / 2.f) };
	float threshold{ std::cos(halfAngleRadian) };

	if (dotProduct > threshold)
		return NODE_STATUS::SUCCESS;

	NODE_STATUS status{ childPtr->Tick(entity) };
	return status == NODE_STATUS::SUCCESS ? NODE_STATUS::FAILURE : status;

}