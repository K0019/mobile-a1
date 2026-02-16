#pragma once
#include "ECS/ECS.h"
#include "ECS/IRegisteredComponent.h"
#include "UI/SpriteComponent.h"

class NiceThrowComponent
	: public IRegisteredComponent<NiceThrowComponent>
{
};
class NiceThrowSystem : public ecs::System<NiceThrowSystem, NiceThrowComponent, SpriteComponent>
{
public:
	NiceThrowSystem();

private:
	void UpdateComp(NiceThrowComponent& niceThrowComp, SpriteComponent& spriteComp);
};