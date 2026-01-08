#pragma once
#include "ECS/IRegisteredComponent.h"

class BillboardComponent : public IRegisteredComponent<BillboardComponent>
{
public:

private:

};

class BillboardSystem : public ecs::System<BillboardSystem, BillboardComponent>
{
public:
	BillboardSystem();

	bool PreRun() override;

private:
	void UpdateComp(BillboardComponent& comp);

private:
	Vec3 camPos{};
};
