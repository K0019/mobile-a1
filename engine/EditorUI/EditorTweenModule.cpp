/******************************************************************************/
/*!
\file   EditorTweenModule.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   02/11/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  Handles the interpolation of the Editor camera.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "EditorTweenModule.h"
#include "TweenManager.h"
#include "CameraController.h"
#include "Editor.h"
#include "Engine.h"

EditorTweenModule::EditorTweenModule()
{
	Messaging::Subscribe("OnEntitySelected", EditorTweenModule::TweenCameraToEntity);
}

void EditorTweenModule::TweenCameraToEntity(ecs::EntityHandle entity)
{
	// Move camera to entity
	ST<TweenManager>::Get()->StartTween(
		*ST<CameraController>::Get(),
		&CameraController::SetPosition,
		ST<CameraController>::Get()->GetPosition(),
		entity->GetTransform().GetWorldPosition(),
		tweenDuration,
		TT::EASE_BOTH
	);

	// Calculate biggest dimension of entity
	float x = entity->GetTransform().GetWorldScale().x;
	float y = entity->GetTransform().GetWorldScale().y;
	float a = x > y ? x : y; // Get the biggest length

	float width = static_cast<float>(ST<Engine>::Get()->_worldExtent.width);
	float height = static_cast<float>(ST<Engine>::Get()->_worldExtent.height);
	float b = width < height ? width : height; // Get the smallest length


	float currentZoom = ST<CameraController>::Get()->GetZoom();
	float targetZoom = (b / a) * zoomAmount;

	// Start tween to zoom
	ST<TweenManager>::Get()->StartTween(
		*ST<CameraController>::Get(),
		&CameraController::SetZoom,
		currentZoom,
		targetZoom,
		tweenDuration,
		TT::EASE_BOTH
	);

	// ST<CameraController>::Get()->SetPosition(entity->GetTransform().GetWorldPosition());
}

EditorTweenModule::~EditorTweenModule()
{
	Messaging::Unsubscribe("OnEntitySelected", EditorTweenModule::TweenCameraToEntity);
}