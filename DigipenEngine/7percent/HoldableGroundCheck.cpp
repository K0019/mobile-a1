#include "pch.h"
#include "HoldableGroundCheck.h"

HealthpackGroundCheckSystem::HealthpackGroundCheckSystem()
	: System_Internal{ &HealthpackGroundCheckSystem::UpdateComp }

{
}

void HealthpackGroundCheckSystem::UpdateComp(HealthpackComponent& healthpackComp)
{
	if (!healthpackComp.thisEntity)
		healthpackComp.thisEntity = ecs::GetEntity(&healthpackComp);
	healthpackComp.PerformGroundedCheck();
}

