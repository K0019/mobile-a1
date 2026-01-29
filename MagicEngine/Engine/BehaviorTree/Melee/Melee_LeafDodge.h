#pragma once
#include "Engine/BehaviorTree/BehaviourTree.h"

class L_Melee_Dodge : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    //Probability to dodge. (1 to 100)
    int probability;
    bool dodged;
};