#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_DontRun : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    static float attackDistance;      // Distance to start attack
    static float followThroughTime;   // Delay after attack completes

    float currentFollowThroughTime;
    bool hasStartedAttack;      // Has the attack sequence begun (animation played)
    bool hasSpawnedProjectile;  // Has the projectile been spawned
    bool isFollowingThrough;    // Is in post-attack delay
};