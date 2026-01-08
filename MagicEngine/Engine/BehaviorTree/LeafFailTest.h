#pragma once
#include "Engine/BehaviorTree/BehaviourNode.h"

class LeafFailTest 
    : public BehaviorNode
{
protected:
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};