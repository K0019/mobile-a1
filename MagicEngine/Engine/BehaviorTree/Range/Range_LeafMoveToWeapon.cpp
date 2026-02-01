#include "Range_LeafMoveToWeapon.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Engine/NavMeshAgent.h"
#include "Components/NameComponent.h"

void L_Range_MoveToWeapon::OnInitialize()
{
    //reset pos;
    collectRange = 1.3f;
}

NODE_STATUS L_Range_MoveToWeapon::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    ecs::CompHandle<CharacterMovementComponent> characterComp = entity->GetComp<CharacterMovementComponent>();
    ecs::CompHandle<navmesh::NavMeshAgentComp> agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
    if (!enemyComp || !characterComp || !agentComp)
        return NODE_STATUS::FAILURE;

    if (!weapon || weapon->GetComp<GrabbableItemComponent>()->isHeld == true)
    {
        weapon = nullptr;
		for (auto itemComp = ecs::GetCompsBegin<GrabbableItemComponent>(); itemComp != ecs::GetCompsEnd<GrabbableItemComponent>(); ++itemComp)
		{
            //Check if it's a valid weapon.
			if (itemComp.GetEntity() == nullptr || 
				itemComp.GetEntity()->GetComp<CharacterMovementComponent>() ||
				itemComp->isHeld == true ||
                !itemComp.GetEntity()->GetComp<NameComponent>() || 
                itemComp.GetEntity()->GetComp<NameComponent>()->GetName() != "WeaponChair")
				continue;

            if (agentComp->FindPath(itemComp.GetEntity()->GetTransform().GetWorldPosition()).status != navmesh::NavMeshPathStatus::PATH_COMPLETE)
                continue;

            weapon = itemComp.GetEntity();
            break;
		}
    }

    if (!weapon)
        return NODE_STATUS::FAILURE;

    Vec3 weaponPos = weapon->GetTransform().GetWorldPosition();
    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 distance{ weaponPos - enemyPos };

    agentComp->SetActive(true);
    agentComp->SetTargetPos(weaponPos);

    if (distance.LengthSqr() > collectRange * collectRange)
        return NODE_STATUS::RUNNING;

    characterComp->GrabItem(weapon->GetComp<GrabbableItemComponent>());
    return NODE_STATUS::SUCCESS;
}