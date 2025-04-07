/******************************************************************************/
/*!
\file   CameraSystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of CameraSystem.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "CameraSystem.h"
#include "CameraController.h"

CameraSystem::CameraSystem() : System_Internal(&CameraSystem::UpdateCameraComp) {}

void CameraSystem::UpdateCameraComp(CameraComponent& cameraComp)
{
  if(cameraComp.isActive()) // && (cameraComp.priority == CameraComponent::globalPriority || cameraComp.priority > CameraComponent::globalPriority))
  {
    const auto entity = ecs::GetEntity(&cameraComp);
    const auto& transform = entity->GetTransform();
    ST<CameraController>::Get()->SetPosition(transform.GetWorldPosition());
    ST<CameraController>::Get()->SetRotation(transform.GetWorldRotation());
    if (cameraComp.priority > CameraComponent::globalPriority)
    {
      CameraComponent::globalPriority = cameraComp.priority;
    }
  }
  else
  {
    cameraComp.active = false;
  }
  cameraComp.zoom = ST<CameraController>::Get()->GetZoom();
}

AnchorToCameraSystem::AnchorToCameraSystem()
    : System_Internal{ &AnchorToCameraSystem::UpdateComp }
{
}

bool AnchorToCameraSystem::PreRun()
{
    targetPosition = ST<CameraController>::Get()->GetPosition();
    return true;
}

void AnchorToCameraSystem::UpdateComp(AnchorToCameraComponent& comp)
{
    ecs::GetEntityTransform(&comp).SetWorldPosition(targetPosition);
}

ShakeSystem::ShakeSystem()
    : System_Internal{ &ShakeSystem::UpdateComp }
{
}

void ShakeSystem::UpdateComp(ShakeComponent& comp)
{
    comp.UpdateTime(GameTime::FixedDt());

    const auto& offsets{ comp.CalcOffsets() };
    Transform& transform{ ecs::GetEntityTransform(&comp) };
    transform.AddLocalPosition(offsets.pos);
    // ROTATION IS CURRENTLY VERY BROKEN LOL
    //transform.AddLocalRotation(offsets.rot);
}

void ShakeSystem::OnAdded()
{
    Messaging::Subscribe("DoCameraShake", ShakeSystem::ApplyShakeToCamera);
}

void ShakeSystem::OnRemoved()
{
    Messaging::Unsubscribe("DoCameraShake", ShakeSystem::ApplyShakeToCamera);
}

void ShakeSystem::ApplyShakeToCamera(float strength, float cap)
{
    // Iterate camera components since there's likely to be fewer of those than shake components.
    for (auto camIter{ ecs::GetCompsBegin<CameraComponent>() }, endIter{ ecs::GetCompsEnd<CameraComponent>() }; camIter != endIter; ++camIter)
        if (auto shakeComp{ camIter.GetEntity()->GetComp<ShakeComponent>() })
            shakeComp->InduceStress(strength, cap);
}

UndoShakeSystem::UndoShakeSystem()
    : System_Internal{ &UndoShakeSystem::UpdateComp }
{
}

void UndoShakeSystem::UpdateComp(ShakeComponent& comp)
{
    const auto& offsets{ comp.GetOffsets() };
    Transform& transform{ ecs::GetEntityTransform(&comp) };
    transform.AddLocalPosition(-offsets.pos);
    //transform.AddLocalRotation(-offsets.rot);
}
