#include "Boss_LeafDisciplinaryAction.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"
#include "Graphics/AnimationComponent.h"

float L_Boss_Prefect_DisciplinaryAction::explosionSize = 5.0f;
float L_Boss_Prefect_DisciplinaryAction::animationSpeed = 2.5f;
float L_Boss_Prefect_DisciplinaryAction::jumpSpeed = 15.0f;

void L_Boss_Prefect_DisciplinaryAction::OnInitialize()
{
    currentPhase = Phase::EXPLODE_START;
    animationStarted = false;
}

NODE_STATUS L_Boss_Prefect_DisciplinaryAction::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
    {
        // Always rotate towards player
        Vec2 dir = Boss_Prefect_Util::GetMovementTowards(
            entity->GetTransform().GetWorldPosition(), 
            enemyComp->playerReference->GetTransform().GetWorldPosition()
        );
        Boss_Prefect_Util::RotateTowards(entity, dir);

        auto animComp = entity->GetComp<AnimationComponent>();

        switch (currentPhase)
        {
        case Phase::EXPLODE_START:
        {
            // Phase 1: Spawn initial explosion, then start jump animation
            if (Boss_Prefect_Util::SpawnExplosion(entity, explosionSize))
            {
                // Start the jump animation
                if (animComp)
                {
                    animComp->TransitionTo(5860832383719118265, 0.1f);
                    animComp->timeA = 0.0f;
                    animComp->loop = false;  // Don't loop - we need to detect when it ends
                    animComp->isPlaying = true;
                    animComp->speed = animationSpeed;  // Speed up the jump animation
                }
                animationStarted = false;
                currentPhase = Phase::JUMPING;
            }
            break;
        }

        case Phase::JUMPING:
        {
            // Phase 2: Move towards player while jump animation plays
            if (animComp)
            {
                // Ensure animation speed stays set each frame
                animComp->speed = animationSpeed;
                
                // Check if animation has started playing (timeA > 0)
                if (animComp->timeA > 0.0f)
                {
                    animationStarted = true;
                }

                // Check if animation finished (isPlaying becomes false when non-looping animation ends)
                // Also check if we've progressed through most of the animation as a fallback
                const ResourceAnimation* clip = animComp->GetAnimationClipA();
                float duration = clip ? animComp->GetClipDuration(clip) : 0.0f;
                
                // Move based on animation progress - use cosine curve for natural jump arc
                // cos(0) = 1, cos(pi/2) = 0 - starts fast, decelerates to landing
                if (duration > 0.0f && animationStarted)
                {
                    float progress = animComp->timeA / duration;  // 0.0 to 1.0
                    // Cosine curve: cos(pi/2 * progress) gives 1 at start, 0 at end
                    float movementScale = std::cos(1.5708f * progress);  // 1.5708 = pi/2
                    Vec2 dirNorm = dir.LengthSqr() > 0.0f ? dir.Normalized() : Vec2{ 0.0f, 0.0f };
                    Boss_Prefect_Util::MoveInDirection(entity, Vec3{ jumpSpeed * dirNorm.x * movementScale, 0.0f, jumpSpeed * dirNorm.y * movementScale });
                }
                
                bool animationFinished = false;
                if (animationStarted)
                {
                    // Animation is finished if isPlaying is false, or if we've reached near the end
                    if (!animComp->isPlaying || (duration > 0.0f && animComp->timeA >= duration * 0.95f))
                    {
                        animationFinished = true;
                    }
                }

                if (animationFinished)
                {
                    // Transition to final explosion phase
                    currentPhase = Phase::EXPLODE_END;
                }
            }
            else
            {
                // No animation component, skip to explosion
                currentPhase = Phase::EXPLODE_END;
            }
            break;
        }

        case Phase::EXPLODE_END:
        {
            // Phase 3: Spawn final explosion after landing
            if (Boss_Prefect_Util::SpawnExplosion(entity, explosionSize))
            {
                // Reset animation state so next node can play animations
                if (animComp)
                {
                    animComp->loop = true;
                    animComp->isPlaying = true;
                    animComp->speed = 1.0f;  // Reset animation speed to normal
                }
                return NODE_STATUS::SUCCESS;
            }
            break;
        }
        }
    }

    return NODE_STATUS::RUNNING;
}