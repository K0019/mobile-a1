#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_DisciplinaryAction : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    static int lungeCount;
    static float explosionSize;
    
    int currentLungeCount;
    bool hasExploded;
};