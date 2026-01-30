#pragma once
#include "Engine/BehaviorTree/BehaviourTree.h"

class L_Melee_CheckAttack : public BehaviorNode
{
public:
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};