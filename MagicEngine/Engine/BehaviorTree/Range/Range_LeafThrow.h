#pragma once
#include "Engine/BehaviorTree/BehaviourTree.h"

class L_Range_Throw : public BehaviorNode
{
public:
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};