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
		const glm::quat& rot = gyroEvent->rotation;

		// Skip zero-initialized events — Engine.cpp dispatches GyroRotation
		// every frame, but the first frame(s) may fire before the sensor has
		// produced any real data, leaving gyroRotation at default (0,0,0).
		// Capturing (0,0,0) as the reference would make every subsequent
		// delta equal the phone's absolute orientation instead of relative.
		if (rot.x == 0.0f && rot.y == 0.0f && rot.z == 0.0f && rot.w == 0.0f)
			continue;

		currentSensorQuat = rot; // radians from android_main.cpp

		// Capture first real reading as reference zero-point.
		if (!hasReference)
		{
			referenceSensorQuat = currentSensorQuat;
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

	// Calculate the rotation difference between quaternions
	glm::quat diffQuat{ currentSensorQuat * glm::conjugate(referenceSensorQuat) };

	// Compute delta from reference (in radians).
	float qx{ diffQuat.x }, qy{ diffQuat.y }, qz{ diffQuat.z }, qw{ diffQuat.w };
	Vec3 delta;
	delta.x = asinf(2.0f * (qw * qy - qz * qx)); // Pitch
	delta.y = atan2f(2.0f * (qw * qz + qx * qy), 1.0f - 2.0f * (qy * qy + qz * qz)); // Yaw
	delta.z = atan2f(2.0f * (qw * qx + qy * qz), 1.0f - 2.0f * (qx * qx + qy * qy)); // Roll

	// Convert delta to degrees and add to scene-authored base rotation.
	Vec3 finalRotation = baseRotation + math::ToDegrees(-delta); // Everything's flipped it seems
	ecs::GetEntityTransform(&camComp).SetWorldRotation(finalRotation);

	static int frame = 0;
	if (++frame % 100 == 0)
		CONSOLE_LOG(LEVEL_INFO) << ecs::GetEntityTransform(&camComp).GetWorldRotation();
}
