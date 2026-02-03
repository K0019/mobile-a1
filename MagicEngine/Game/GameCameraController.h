/******************************************************************************/
/*!
\file   GameCameraController.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	GameCameraController is an ECS component-system pair which takes control of 
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the player, map bounds, and any other such effects to be
	implemented now or in the future.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "ECS/IEditorComponent.h"
#include "Assets/AssetHandle.h"
#include "Assets/Types/AssetTypes.h"

/*****************************************************************//*!
\class GameCameraControllerComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class GameCameraControllerComponent
	: public IRegisteredComponent<GameCameraControllerComponent>
	, public IEditorComponent<GameCameraControllerComponent>
{
public:
	EntityReference cameraEntity;
	EntityReference playerEntity;

	float cameraPitch;
	float cameraYaw;
	float cameraAutoZoomSpeed = 5.0f;
	float minPitch;
	float maxPitch;

	float targetCameraDistance;
	float currentCameraDistance;
	Vec3 offsetPosition = Vec3(0.5f, 1.2f, 0);

	float cameraSensitivity;

	// Serialized
	AssetHandle<ResourceMaterial> translucentMaterial;

	std::vector<ecs::EntityHandle> currentColliders;


	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	GameCameraControllerComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	//~GameCameraControllerComponent();

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

public:
	// ===== Lua helpers =====
	float GetCameraAutoZoomSpeed() const { return cameraAutoZoomSpeed; }
	void  SetCameraAutoZoomSpeed(float v) { cameraAutoZoomSpeed = v; }

	float GetTargetCameraDistance() const { return targetCameraDistance; }
	void  SetTargetCameraDistance(float v) { targetCameraDistance = v; }

	float GetCurrentCameraDistance() const { return currentCameraDistance; }
	void  SetCurrentCameraDistance(float v) { currentCameraDistance = v; }

	Vec3  GetOffsetPosition() const { return offsetPosition; }
	void  SetOffsetPosition(const Vec3& v) { offsetPosition = v; }

	float GetCameraSensitivity() const { return cameraSensitivity; }
	void  SetCameraSensitivity(float v) { cameraSensitivity = v; }

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

	property_vtable()
};
property_begin(GameCameraControllerComponent)
{
	//property_var(cameraEntity),
	//property_var(playerEntity),
	//property_var(cameraPitch),
	//property_var(cameraYaw),
	property_var(cameraAutoZoomSpeed),
	property_var(targetCameraDistance),
	property_var(currentCameraDistance),
	property_var(offsetPosition),
	property_var(minPitch),
	property_var(maxPitch),
	property_var(cameraSensitivity),
	//property_var(lookAction),
}
property_vend_h(GameCameraControllerComponent)

/*****************************************************************//*!
\class GameCameraControllerSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class GameCameraControllerSystem : public ecs::System<GameCameraControllerSystem, GameCameraControllerComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	GameCameraControllerSystem();

	/*****************************************************************//*!
	\brief
		Subscribes to relevant messages.
	*//******************************************************************/
	//void OnAdded() override;

	/*****************************************************************//*!
	\brief
		Unsubscribes from relevant messages.
	*//******************************************************************/
	//void OnRemoved() override;

	/*****************************************************************//*!
	\brief
		Static callback function.
	*//******************************************************************/
	static void OnWaveStarted();

private:
	Vec2 prevPos = Vec2(-1, -1);
	/*****************************************************************//*!
	\brief
		Updates GameCameraControllerComponent.
	\param comp
		The GameCameraControllerComponent to update.
	*//******************************************************************/
	void UpdateGameCameraController(GameCameraControllerComponent& comp);
};