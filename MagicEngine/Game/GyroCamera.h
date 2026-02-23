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

	// Wrap an angle delta into [-pi, pi] to handle atan2 discontinuities.
	static float WrapDelta(float delta);

private:
	// Latest sensor Euler angles (radians) from the event stream.
	glm::quat currentSensorQuat{};

	// First sensor reading — used as the zero-point for deltas.
	glm::quat referenceSensorQuat{};
	bool hasReference = false;

	// Camera's scene-authored rotation (degrees) captured before we touch it.
	Vec3 baseRotation{0.0f};
	bool hasBaseRotation = false;

	EventsReader<Events::GyroRotation> orientationReader;
};
