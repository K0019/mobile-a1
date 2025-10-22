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
#include "Game/GameCameraController.h"
#include "Tween/TweenManager.h"
#include "Utilities/Messaging.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Input.h"

GameCameraControllerComponent::GameCameraControllerComponent()
	: cameraEntity{ nullptr }
	, playerEntity{ nullptr }
	, cameraPitch{ 0.0f }
	, cameraYaw{ 0.0f }
	, cameraSensitivity{ 1.0f }
{
}

//GameCameraControllerComponent::~GameCameraControllerComponent()
//{
//}

void GameCameraControllerComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("cameraEntity", cameraEntity);
	writer.Serialize("playerEntity", playerEntity);
	//.Serialize(writer);
	//playerEntity.Serialize(writer);
}

void GameCameraControllerComponent::Deserialize(Deserializer& reader)
{
	reader.Deserialize("cameraEntity", &cameraEntity);
	reader.Deserialize("playerEntity", &playerEntity);
	//cameraEntity.Deserialize(reader);
	//playerEntity.Deserialize(reader);
}

void GameCameraControllerComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
	cameraEntity.EditorDraw("Camera");
	playerEntity.EditorDraw("Player");
	gui::TextBoxReadOnly("Yaw", std::to_string(cameraYaw));
	gui::TextBoxReadOnly("Pitch", std::to_string(cameraPitch));
#endif
}

GameCameraControllerSystem::GameCameraControllerSystem()
	: System_Internal{ &GameCameraControllerSystem::UpdateGameCameraController }
{
}
glm::mat3x3 GetRotationMatrix(Vec3 rotation)
{
	Vec3 rot = rotation;
	float cx = cos(rot.x); // pitch
	float sx = sin(rot.x);
	float cy = cos(rot.y); // yaw
	float sy = sin(rot.y);
	float cz = cos(rot.z); // roll
	float sz = sin(rot.z);

	// Rotation order: Y * X * Z
	return  glm::mat3x3(
		cy * cz + sy * sx * sz, cz * sy * sx - cy * sz, sy * cx,
		cx * sz, cx * cz, -sx,
		cy * sx * sz - cz * sy, cy * cz * sx + sy * sz, cy * cx
	);
}
Vec3 OrientationFromForward(Vec3 forward)
{
	Vec3 worldUp(0, 1, 0);

	Vec3 f = forward.Normalized();
	Vec3 r = (worldUp* f).Normalized();
	Vec3 u = (f* r); // re-orthogonalize up

	// Build rotation matrix
	glm::mat3x3 R(
		r.x, u.x, f.x,
		r.y, u.y, f.y,
		r.z, u.z, f.z
	);

	Vec3 rot;
	// Euler angles
	rot.x =  math::ToDegrees(asin(-R[2][1]));				// Pitch                     
	rot.y =  math::ToDegrees(atan2(R[2][0], R[2][2]));		// Yaw          
	rot.z =  math::ToDegrees(atan2(R[0][1], R[1][1]));		// Roll

	// Force set rotation
	return rot;
}
glm::mat3x3 PitchYawRollToRotationMatrix(float pitchRadians, float yawRadians, float rollRadians)
{
	// Precompute to save one extra sin/cos per matrix
	float cosA = cos(pitchRadians);
	float sinA = sin(pitchRadians);

	float cosB = cos(yawRadians);
	float sinB = sin(yawRadians);

	float cosC = cos(rollRadians);
	float sinC = sin(rollRadians);

	return glm::mat3x3(
		cosA * cosB,		cosA * sinB * sinC - sinA * cosB,		cosA * sinB * cosC + sinA * sinC,
		sinA * cosB,		sinA * sinB * sinC + cosA * cosC,		sinA * sinB * cosC + cosA * sinC,
		-sinB,				cosB * sinC,							cosB * cosC
	);
		//
}
void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
	// If no player or camera reference, just return.
	if (!comp.playerEntity || !comp.cameraEntity)
		return;

	// Mouse look
	float yaw = comp.cameraYaw;
	float pitch = comp.cameraPitch;
	Vec2 currPos = ST<KeyboardMouseInput>::Get()->GetMousePos();
	if (prevPos.x > 0)
	{
		Vec2 mouseDelta = currPos - prevPos;

		yaw -= mouseDelta.x * comp.cameraSensitivity;
		pitch += mouseDelta.y * comp.cameraSensitivity;

		// Wrap yaw
		if (yaw < 0.0f)
			yaw += 360.0f;
		if (yaw > 360.0f)
			yaw -= 360.0f;

		// Clamp pitch
		if (pitch < -5.0f)
			pitch = -5.0f;
		if (pitch > 85.0f)
			pitch = 85.0f;

		comp.cameraPitch = pitch;
		comp.cameraYaw = yaw;
	}
	prevPos = currPos;

	// Get the camera's position
	float verticalFactor = sin(math::ToRadians(-pitch));
	float horizontalFactor = cos(math::ToRadians(-pitch));
	Vec3 calculatedCameraDirection = Vec3(
		horizontalFactor * cos(math::ToRadians(-yaw + 90)),
		verticalFactor,
		horizontalFactor * sin(math::ToRadians(-yaw + 90))
	);


	ecs::EntityHandle compEntity = ecs::GetEntity(&comp);


	glm::mat3x3 rotMatrix = PitchYawRollToRotationMatrix(math::ToRadians(pitch), math::ToRadians(yaw), 0.0f);
	Vec3 rot = Vec3(1, 1, 1) * rotMatrix;
	rot.x = (rotMatrix[2][1] - rotMatrix[1][2]) / sqrt(pow(rotMatrix[2][1] - rotMatrix[1][2], 2) + pow(rotMatrix[0][2] - rotMatrix[2][0], 2) + pow(rotMatrix[1][0] - rotMatrix[0][1], 2));
	rot.y = (rotMatrix[0][2] - rotMatrix[2][0]) / sqrt(pow(rotMatrix[2][1] - rotMatrix[1][2], 2) + pow(rotMatrix[0][2] - rotMatrix[2][0], 2) + pow(rotMatrix[1][0] - rotMatrix[0][1], 2));
	rot.z = (rotMatrix[1][0] - rotMatrix[0][1]) / sqrt(pow(rotMatrix[2][1] - rotMatrix[1][2], 2) + pow(rotMatrix[0][2] - rotMatrix[2][0], 2) + pow(rotMatrix[1][0] - rotMatrix[0][1], 2));

	//compEntity->GetTransform().SetWorldRotation(rot*math::ToDegrees(1));
	//comp.cameraEntity->GetTransform().SetWorldRotation(Vec3(0, yaw, 0));
	//Vec3 childRot = Vec3(
	//	pitch * sin(math::ToRadians(yaw)),
	//	yaw + 90,
	//	pitch * sin(math::ToRadians(yaw)));
	//compEntity->GetTransform().SetLocalRotation(childRot);
	Vec3 currRot = compEntity->GetTransform().GetWorldRotation();
	currRot.y = yaw+90;

	compEntity->GetTransform().SetWorldRotation(currRot);
	//compEntity->GetTransform().SetLocalRotation({ 0 });
	//compEntity->GetTransform().AddWorldRotation({ 0,0,pitch });
	//compEntity->GetTransform().AddWorldRotation({ 0,yaw,0 });

	comp.cameraEntity->GetTransform().SetWorldPosition(comp.playerEntity->GetTransform().GetWorldPosition() - calculatedCameraDirection * 10.0f);
}
