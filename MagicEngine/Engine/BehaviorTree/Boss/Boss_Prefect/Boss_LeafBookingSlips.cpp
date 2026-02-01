#include "Boss_LeafBookingSlips.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"
#include "Engine/PrefabManager.h"
#include "Components/EntityReferenceHolder.h"
#include "Graphics/AnimationComponent.h"
#include "Managers/AudioManager.h"

int L_Boss_Prefect_BookingSlips::burstCount = 4;
float L_Boss_Prefect_BookingSlips::burstDelay = 1.f;
float L_Boss_Prefect_BookingSlips::attackDistance = 25.0f * 25.0f;  // Squared distance for range check

void L_Boss_Prefect_BookingSlips::OnInitialize()
{
    currentBurstDelay = 0.0f;  // Start ready to fire first burst immediately
    currentBurstCount = 0;
    hasStartedBurst = false;
}

NODE_STATUS L_Boss_Prefect_BookingSlips::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
    {
        // Use direct direction to player for rotation so boss faces the player
        Vec2 dirToPlayer = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
        Boss_Prefect_Util::RotateTowards(entity, dirToPlayer);

        // Check distance condition ONLY before the first burst is fired
        if (!hasStartedBurst)
        {
            // If player is too far, fail immediately (let BT pick another node)
            if (dirToPlayer.LengthSqr() > attackDistance)
            {
                return NODE_STATUS::FAILURE;
            }
        }

        // Burst attack logic
        if (currentBurstDelay <= 0.0f)
        {
            hasStartedBurst = true;  // Mark that we've started the burst sequence
            ++currentBurstCount;
            currentBurstDelay = burstDelay;

            auto animComp = entity->GetComp<AnimationComponent>();
            if (animComp && animComp->animHandleA.GetResource())
            {
                animComp->TransitionTo(5142979933624197533, 0.1f);
                animComp->timeA = 0.0f;
            }

            // Spawn the booking slip and set the entity reference
            ST<Scheduler>::Get()->Add([enemyComp, spawnPos = entity->GetTransform().GetWorldPosition()]() {
                ecs::EntityHandle spawnedSlip = ST<PrefabManager>::Get()->LoadPrefab("prefect_bookingslip"); 
                spawnedSlip->GetComp<EntityReferenceHolderComponent>()->SetEntity(0, enemyComp->playerReference);

                spawnedSlip->GetTransform().SetWorldPosition(spawnPos);
                spawnedSlip->GetTransform().SetWorldScale(Vec3{ 0.4f });
            });

            ST<AudioManager>::Get()->PlaySound3D("boss throw " + std::to_string(currentBurstCount), false, entity->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);

            if (currentBurstCount >= burstCount)
                return NODE_STATUS::SUCCESS;
        }
    }
    currentBurstDelay -= GameTime::Dt();
    return NODE_STATUS::RUNNING;
}