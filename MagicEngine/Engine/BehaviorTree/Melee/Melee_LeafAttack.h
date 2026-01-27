#pragma once
#include "Engine/BehaviorTree/BehaviourTree.h"

class L_Melee_Attack : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    float windUpTime;
    float attackCoolDown;
};