#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_DontRun : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    float currentInvincinilityTime;
    static float invincibilityTime;
};