#include "BillboardComponent.h"
#include "Graphics/CameraComponent.h"

BillboardSystem::BillboardSystem()
	: System_Internal{ &BillboardSystem::UpdateComp }
{
}

bool BillboardSystem::PreRun()
{
	// TODO: Account for which camera is the active one
	auto camIter{ ecs::GetCompsActiveBeginConst<CameraComponent>() };
	if (camIter == ecs::GetCompsEndConst<CameraComponent>())
		return false;

	camPos = camIter.GetEntity()->GetTransform().GetWorldPosition();
	return true;
}

void BillboardSystem::UpdateComp(BillboardComponent& comp)
{
	Transform& transform{ ecs::GetEntityTransform(&comp) };
	Vec3 toCam{ camPos - transform.GetWorldPosition() };

	Vec3 worldRot{ transform.GetWorldRotation() };
	worldRot.y = -math::ToDegrees(std::atan2f(toCam.z, toCam.x));
	transform.SetWorldRotation(worldRot);
}
