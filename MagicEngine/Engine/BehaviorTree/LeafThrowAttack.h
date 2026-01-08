#include "Engine/BehaviorTree/BehaviourTree.h"

class L_ThrowAttack : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    float currSpentTime;
};