#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_Invincibility : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
    void OnTerminate(NODE_STATUS) override;

private:
    float currentInvincibilityTime;
    static float invincibilityTime;
    bool inited = false;
};