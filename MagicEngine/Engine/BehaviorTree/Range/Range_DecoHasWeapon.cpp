#include "Engine/BehaviorTree/Range/Range_DecoHasWeapon.h"
#include "Game/Character.h"

NODE_STATUS D_Range_HasWeapon::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	if (!charComp)
		return NODE_STATUS::FAILURE;

	if (charComp->heldItem)
		return NODE_STATUS::SUCCESS;

	NODE_STATUS status{ childPtr->Tick(entity) };
	return status == NODE_STATUS::SUCCESS ? NODE_STATUS::FAILURE : status;
}