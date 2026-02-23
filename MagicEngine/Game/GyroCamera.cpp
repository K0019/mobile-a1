#include "GyroCamera.h"

GyroCameraSystem::GyroCameraSystem()
	: System_Internal{ &GyroCameraSystem::UpdateComp }
{
}

float GyroCameraSystem::WrapDelta(float delta)
{
	// Normalize to [-pi, pi] to handle atan2 discontinuity at +/-180 degrees.
	while (delta >  3.14159265f) delta -= 6.28318530f;
	while (delta < -3.14159265f) delta += 6.28318530f;
	return delta;
}

bool GyroCameraSystem::PreRun()
{
	while (auto gyroEvent{ orientationReader.ExtractEvent() })
	{
		const Vec3& rot = gyroEvent->rotation;

		// Skip zero-initialized events — Engine.cpp dispatches GyroRotation
		// every frame, but the first frame(s) may fire before the sensor has
		// produced any real data, leaving gyroRotation at default (0,0,0).
		// Capturing (0,0,0) as the reference would make every subsequent
		// delta equal the phone's absolute orientation instead of relative.
		if (rot.x == 0.0f && rot.y == 0.0f && rot.z == 0.0f)
			continue;

		currentSensorEuler = rot; // radians from android_main.cpp

		// Capture first real reading as reference zero-point.
		if (!hasReference)
		{
			referenceSensorEuler = currentSensorEuler;
			hasReference = true;
		}
	}
	return true;
}

void GyroCameraSystem::UpdateComp([[maybe_unused]] GyroCameraComponent& gyroComp, CameraComponent& camComp)
{
	// Capture camera's scene-authored rotation before we modify it.
	if (!hasBaseRotation)
	{
		baseRotation = ecs::GetEntityTransform(&camComp).GetWorldRotation();
		hasBaseRotation = true;
	}

	if (!hasReference)
		return; // No sensor data yet.

	// Compute delta from reference (in radians), handling wrap-around.
	Vec3 delta;
	delta.x = WrapDelta(currentSensorEuler.x - referenceSensorEuler.x); // pitch
	delta.y = WrapDelta(currentSensorEuler.y - referenceSensorEuler.y); // yaw
	delta.z = WrapDelta(currentSensorEuler.z - referenceSensorEuler.z); // roll

	// Convert delta to degrees and add to scene-authored base rotation.
	Vec3 finalRotation = baseRotation + math::ToDegrees(delta);
	ecs::GetEntityTransform(&camComp).SetWorldRotation(finalRotation);
}
