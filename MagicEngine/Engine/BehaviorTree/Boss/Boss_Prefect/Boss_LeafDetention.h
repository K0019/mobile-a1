#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_Detention : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    static float triggerDistanceSqr;
    static const float triggerDistance;
    static const float burstDelay;
    static const int burstCount;

    float currentBurstDelay;
    int currentBurstCount;
    bool hasTriggered;  // Track if the attack sequence has started

    std::vector<float> explosionSizes;
};