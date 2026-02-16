#include "GyroCamera.h"

GyroCameraSystem::GyroCameraSystem()
	: System_Internal{ &GyroCameraSystem::UpdateComp }
{
}

bool GyroCameraSystem::PreRun()
{
	while (auto gyroEvent{ orientationReader.ExtractEvent() })
		orientation = math::ToDegrees(gyroEvent->rotation);
	return true;
}

void GyroCameraSystem::UpdateComp([[maybe_unused]] GyroCameraComponent& gyroComp, CameraComponent& camComp)
{
	ecs::GetEntityTransform(&camComp).SetWorldRotation(orientation);
}
