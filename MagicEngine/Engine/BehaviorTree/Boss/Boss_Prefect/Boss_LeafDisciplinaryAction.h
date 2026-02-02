#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_DisciplinaryAction : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    // Phase enumeration for the 3-phase attack
    enum class Phase
    {
        EXPLODE_START,  // Phase 1: Initial explosion before jump
        JUMPING,        // Phase 2: Playing jump animation
        EXPLODE_END     // Phase 3: Final explosion after landing
    };

    static float explosionSize;
    static float animationSpeed;  // Speed multiplier for the jump animation
    static float jumpSpeed;       // Movement speed during jump phase
    
    Phase currentPhase = Phase::EXPLODE_START;
    bool animationStarted = false;
};