#pragma once
#include "BehaviourNode.h"

class L_MoveTowardsPlayer : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    float moveSpeed{ 5.0f };
};