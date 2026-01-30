#include "Engine/BehaviorTree/BehaviourTree.h"

class D_Melee_IsInAttackRange
	: public Decorator
{
public:
	void OnInitialize() override;
	NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
	float attackRange;
};