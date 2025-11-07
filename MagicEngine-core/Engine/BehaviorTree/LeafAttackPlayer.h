#pragma once
#include "BehaviourNode.h"

enum class AttackPhase { Idle, Windup, Attack, Cooldown };

class L_AttackPlayer : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    AttackPhase phase = AttackPhase::Idle;
    float attackTimer = 0.0f;
    bool attackHit = false;

    // configurable parameters
    float attackRange = 5.0f;
    float attackWidth = 3.0f;
    float attackWindup = 1.0f;
    float attackCooldown = 2.0f;
    float attackDamage = 20.0f;
};