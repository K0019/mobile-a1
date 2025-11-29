#include "Boss_LeafBookingSlips.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"
#include "Engine/PrefabManager.h"
#include "Components/EntityReferenceHolder.h"
#include "Graphics/AnimationComponent.h"

int L_Boss_Prefect_BookingSlips::burstCount = 5;
float L_Boss_Prefect_BookingSlips::burstDelay = 1.f;

void L_Boss_Prefect_BookingSlips::OnInitialize()
{
    currentBurstDelay = burstDelay;
    currentBurstCount = 0;
}

NODE_STATUS L_Boss_Prefect_BookingSlips::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    //if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    //{
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            // Use orbiting movement here
            //characterComp->SetMovementVector(Boss_Prefect_Util::GetMovementDirection(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition()));
			Vec2 dir = Boss_Prefect_Util::GetMovementDirection(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
			Vec3 movementDir3D{ dir.x, 0.0f, dir.y };
            Boss_Prefect_Util::MoveInDirection(entity,movementDir3D);
			Boss_Prefect_Util::RotateTowards(entity, dir);
            if (currentBurstDelay < 0.0f)
            {
                ++currentBurstCount;
                currentBurstDelay = burstDelay;
                auto animComp = entity->GetComp<AnimationComponent>();
                if (animComp)
                {
                    animComp->TransitionTo(5142979933624197533, 0.1f);
                    animComp->timeA = 0.0f;
				}
                // Spawn the booking slip and set the entity reference
                ST<Scheduler>::Get()->Add([enemyComp, spawnPos = entity->GetTransform().GetWorldPosition()]() {
                    //ecs::EntityHandle spawnedSlip = ST<PrefabManager>::Get()->LoadPrefab("explosion"); 
                    ecs::EntityHandle spawnedSlip = ST<PrefabManager>::Get()->LoadPrefab("prefect_bookingslip"); 
                    spawnedSlip->GetComp<EntityReferenceHolderComponent>()->SetEntity(0, enemyComp->playerReference);

                    spawnedSlip->GetTransform().SetWorldPosition(spawnPos);
                    spawnedSlip->GetTransform().SetWorldScale(Vec3{ 0.4f });
                });

            
                if (currentBurstCount >= burstCount)
                    return NODE_STATUS::SUCCESS;
            }
            
        }
    //}
    currentBurstDelay -= GameTime::Dt();
    return NODE_STATUS::RUNNING;
}