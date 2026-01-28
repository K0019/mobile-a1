#pragma once
#include "ECS/EntityUID.h"
#include "Engine/BehaviorTree/BehaviourTree.h"

class L_Range_MoveToWeapon : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    EntityReference weapon;
    float collectRange;
};