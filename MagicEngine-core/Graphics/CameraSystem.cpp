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
#include "Graphics/CameraSystem.h"
#include "Graphics/CameraController.h"
#include "Engine/Graphics Interface/GraphicsScene.h"

CameraCompUploadSystem::CameraCompUploadSystem() : System_Internal(&CameraCompUploadSystem::UpdateCameraComp) {}

void CameraCompUploadSystem::UpdateCameraComp(CameraComponent& cameraComp)
{
    if (!cameraComp.isActive())
        return;

    if (cameraComp.priority < CameraComponent::globalPriority)
        return;
    CameraComponent::globalPriority = cameraComp.priority;

    const auto& transform{ ecs::GetEntityTransform(&cameraComp) };
    Vec3 position{ transform.GetWorldPosition() }, rotation{ transform.GetWorldRotation() };
    static CameraPositioner_MoveTo positioner{ position, rotation };
    positioner.setPosition(position);
    positioner.setDesiredPosition(position);
    positioner.setAngles(rotation);
    positioner.setDesiredAngles(rotation);
    positioner.update(0.0f);
    ST<GraphicsScene>::Get()->SetViewCamera(Camera{ positioner });
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
