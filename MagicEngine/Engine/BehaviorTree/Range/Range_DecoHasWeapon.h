#include "Engine/BehaviorTree/BehaviourTree.h"

class D_Range_HasWeapon
	: public Decorator
{
public:
	NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};