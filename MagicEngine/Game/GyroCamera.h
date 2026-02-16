#pragma once
#include "ECS/ECS.h"
#include "ECS/IRegisteredComponent.h"
#include "Graphics/CameraComponent.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

class GyroCameraComponent
	: public IRegisteredComponent<GyroCameraComponent>
{
};

class GyroCameraSystem : public ecs::System<GyroCameraSystem, GyroCameraComponent, CameraComponent>
{
public:
	GyroCameraSystem();

	bool PreRun() override;

private:
	void UpdateComp(GyroCameraComponent& gyroComp, CameraComponent& camComp);

private:
	Vec3 orientation;
	EventsReader<Events::GyroRotation> orientationReader;
};
