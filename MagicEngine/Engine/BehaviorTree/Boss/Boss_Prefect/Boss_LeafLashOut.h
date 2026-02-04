#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_LashOut : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
float currentAttackCooldown;
float currentAttackDelay;
int currentAttackCount;

bool impendingAttack;
bool waitingForAnimation;

    static float speedMultiplier;
    static float attackCooldown;
    static float attackDelay;
    static float attackDistance;
    static int attackCount;
};