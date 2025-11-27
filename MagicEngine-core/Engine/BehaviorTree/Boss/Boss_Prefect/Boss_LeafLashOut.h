#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_LashOut : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    float currentAttackCooldown;
    int currentAttackCount;

    static float speedMultiplier;
    static float attackCooldown;
    static float attackDistance;
    static int attackCount;
};