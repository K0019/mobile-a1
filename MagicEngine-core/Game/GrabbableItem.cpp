/******************************************************************************/
/*!
\file   GameCameraController.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	GameCameraController is an ECS component-system pair which takes control of
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the player, map bounds, and any other such effects to be
	implemented now or in the future.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Game/GrabbableItem.h"
#include "Game/Character.h"
#include "Game/GameCameraController.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

GrabbableItemComponent::GrabbableItemComponent()
{
}

void GrabbableItemComponent::Serialize(Serializer& writer) const
{
}

void GrabbableItemComponent::Deserialize(Deserializer& reader)
{
}

void GrabbableItemComponent::EditorDraw()
{
}

GrabbableItemComponentSystem::GrabbableItemComponentSystem()
	: System_Internal{ &GrabbableItemComponentSystem::UpdateGrabbableItemComponent }

{
}

void GrabbableItemComponentSystem::UpdateGrabbableItemComponent(GrabbableItemComponent& comp)
{
}
