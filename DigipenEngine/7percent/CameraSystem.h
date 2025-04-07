#pragma once
/******************************************************************************/
/*!
\file   CameraSystem.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (5%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Camera system for managing camera components.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "CameraComponent.h"

class CameraSystem : public ecs::System<CameraSystem, CameraComponent>
{
public:
    explicit CameraSystem();
private:
    void UpdateCameraComp(CameraComponent& cameraComp);
};

/*****************************************************************//*!
\class AnchorToCameraSystem
\brief
    Anchors the entity with AnchorToCameraComponent to the camera's location.
*//******************************************************************/
class AnchorToCameraSystem : public ecs::System<AnchorToCameraSystem, AnchorToCameraComponent>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    AnchorToCameraSystem();

    /*****************************************************************//*!
    \brief
        Updates the target position.
    \return
        True. This system will always run.
    *//******************************************************************/
    bool PreRun() override;

private:
    /*****************************************************************//*!
    \brief
        Moves entities with AnchorToCameraComponent to the camera.
    *//******************************************************************/
    void UpdateComp(AnchorToCameraComponent& comp);

    //! The target location to move entities to.
    Vector2 targetPosition;

};

/*****************************************************************//*!
\class ShakeSystem
\brief
    Shakes entities with ShakeComponent attached.
*//******************************************************************/
class ShakeSystem : public ecs::System<ShakeSystem, ShakeComponent>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    ShakeSystem();

    /*****************************************************************//*!
    \brief
        Subscribes to "DoCameraShake" event. Only shakes entities that have
        both shake and camera components.
    *//******************************************************************/
    void OnAdded() override;

    /*****************************************************************//*!
    \brief
        Unsubscribes from "DoCameraShake" event.
    *//******************************************************************/
    void OnRemoved() override;

private:
    /*****************************************************************//*!
    \brief
        Shakes entities with ShakeComponent attached.
    *//******************************************************************/
    void UpdateComp(ShakeComponent& comp);

    /*****************************************************************//*!
    \brief
        Looks for all entities with shake and camera components and applies
        a shake.
    \param strength
        The amount of shake to apply.
    \param cap
        The maximum amount of shake that the camera can have as a result of
        this shake.
    *//******************************************************************/
    static void ApplyShakeToCamera(float strength, float cap);

};

/*****************************************************************//*!
\class UndoShakeSystem
\brief
    Undoes the offsets that entities with ShakeComponent attached
    received in the previous update.
*//******************************************************************/
class UndoShakeSystem : public ecs::System<UndoShakeSystem, ShakeComponent>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    UndoShakeSystem();

private:
    /*****************************************************************//*!
    \brief
        Undoes the shake that entities with ShakeComponent attached received
        in the previous update.
    *//******************************************************************/
    void UpdateComp(ShakeComponent& comp);

};
