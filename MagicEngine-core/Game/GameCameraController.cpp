/******************************************************************************/
/*!
\file   GameCameraController.cpp
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
#include "Game/GameCameraController.h"
#include "Game/FlashComponent.h"
#include "Game/MaterialSwapper.h"
#include "Tween/TweenManager.h"
#include "Utilities/Messaging.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Input.h"
#include "Physics/Physics.h"

#if defined(__ANDROID__)
#include "Engine/Platform/Android/AndroidInputBridge.h"
#endif


GameCameraControllerComponent::GameCameraControllerComponent()
	: cameraEntity{ nullptr }
	, playerEntity{ nullptr }
	, cameraPitch{ 0.0f }
	, cameraYaw{ 0.0f }
	, minPitch{ 0.0f }
	, maxPitch{ 0.0f }
	, cameraSensitivity{ 1.0f }
	, currentCameraDistance{ 5.0f }
    , currentColliders{}
{
}

//GameCameraControllerComponent::~GameCameraControllerComponent()
//{
//}

void GameCameraControllerComponent::Serialize(Serializer& writer) const
{
    IRegisteredComponent::Serialize(writer);
	writer.Serialize("cameraEntity", cameraEntity);
	writer.Serialize("playerEntity", playerEntity);

	writer.Serialize("maxPitch", maxPitch);
	writer.Serialize("minPitch", minPitch);
		   
	writer.Serialize("cameraSensitivity", cameraSensitivity);

    writer.Serialize("translucentMaterial", translucentMaterial);
}

void GameCameraControllerComponent::Deserialize(Deserializer& reader)
{
    IRegisteredComponent::Deserialize(reader);
	reader.Deserialize("cameraEntity", &cameraEntity);
	reader.Deserialize("playerEntity", &playerEntity);

	reader.DeserializeVar("maxPitch", &maxPitch);
	reader.DeserializeVar("minPitch", &minPitch);

	reader.DeserializeVar("cameraSensitivity", &cameraSensitivity);

	reader.DeserializeVar("translucentMaterial", &translucentMaterial);


	//cameraEntity.Deserialize(reader);
	//playerEntity.Deserialize(reader);
}

void GameCameraControllerComponent::EditorDraw()
{
	cameraEntity.EditorDraw("Camera");
	playerEntity.EditorDraw("Player");
	gui::TextBoxReadOnly("Yaw", std::to_string(cameraYaw));
	gui::TextBoxReadOnly("Pitch", std::to_string(cameraPitch));

	gui::VarInput("Max Pitch", &maxPitch);
	gui::VarInput("Min Pitch", &minPitch);

	gui::VarDrag("Sensitivity", &cameraSensitivity, 0.05f, 0.05f, 1.0f);

    const std::string* materialText{ ST<MagicResourceManager>::Get()->Editor_GetName(translucentMaterial.GetHash()) };
    gui::TextUnformatted("Material");
    gui::TextBoxReadOnly("##", materialText ? materialText->c_str() : "");
    gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
        translucentMaterial = hash;
        });


}

GameCameraControllerSystem::GameCameraControllerSystem()
	: System_Internal{ &GameCameraControllerSystem::UpdateGameCameraController }
{
}

void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
    // Mouse look
    float yaw = comp.cameraYaw;
    float pitch = comp.cameraPitch;

    Vec2 currPos = ST<KeyboardMouseInput>::Get()->GetMousePos();

    // --- ANDROID: only allow camera look if touch isn't owned by joystick/buttons ---
#if defined(__ANDROID__)
    const auto owner = AndroidInputBridge::Owner();
    const bool cameraAllowed = (owner == TouchOwner::NoOwner || owner == TouchOwner::Camera);
#else
    const bool cameraAllowed = true; // desktop unaffected
#endif
    // -------------------------------------------------------------------------------

    if (prevPos.x > 0)
    {
        if (cameraAllowed)
        {
            Vec2 mouseDelta = currPos - prevPos;

            yaw -= mouseDelta.x * comp.cameraSensitivity;
            pitch -= mouseDelta.y * comp.cameraSensitivity;

            // Wrap yaw
            if (yaw < 0.0f)   yaw += 360.0f;
            if (yaw > 360.0f) yaw -= 360.0f;

            // Clamp pitch
            if (pitch < comp.minPitch) pitch = comp.minPitch;
            if (pitch > comp.maxPitch) pitch = comp.maxPitch;

            comp.cameraPitch = pitch;
            comp.cameraYaw = yaw;
        }
    }
    prevPos = currPos;

    // Apply camera transform
    Vec3 eulerAngles{ pitch, yaw, 0.0f };
    ecs::GetEntityTransform(&comp).SetWorldRotation(eulerAngles);

    // If no player, skip following logic
    if (!comp.playerEntity) return;

    // Follow player at current distance
    Vec3 forward = comp.currentCameraDistance * math::EulerAnglesToVector(eulerAngles.x, eulerAngles.y);
    Vec3 playerPos = comp.playerEntity->GetTransform().GetWorldPosition();
    Vec3 cameraPos = playerPos - forward;

    Vec3 direction(sin(math::ToRadians(yaw + 90)), 0, cos(math::ToRadians(yaw + 90)));

    ecs::GetEntityTransform(&comp).SetWorldPosition(cameraPos);

    // Fake fading effect on environment objects
    // I don't see a raycast option, so I'm doing a boxcast instead...
    std::vector<ecs::EntityHandle> currentColliders;
    //physics::OverlapSphere(currentColliders, playerPos - (forward * 0.5f), Vec3{ 1.0f,1.0f,comp.currentCameraDistance }, Vec3{ pitch, yaw, 0.0f });
    
    physics::RaycastHit hit;


    if (physics::Raycast(playerPos - forward * 0.1f, -forward, hit, comp.currentCameraDistance))
    {
        currentColliders.push_back(hit.entityHit);
    }

    // We only want to swap materials that are in either vector. Not both
    std::vector<ecs::EntityHandle> onlyInPrevious;
    for (auto prevColl : comp.currentColliders)
    {
        bool matchInBoth = false;
        for (auto coll : currentColliders)
        {
            if (coll == prevColl)
            {
                matchInBoth = true;
                break;
            }
        }
        if (!matchInBoth)
            onlyInPrevious.push_back(prevColl);
    }
    std::vector<ecs::EntityHandle> onlyInNext;
    for (auto nextColl : currentColliders)
    {
        bool matchInBoth = false;
        for (auto coll : comp.currentColliders)
        {
            if (coll == nextColl)
            {
                matchInBoth = true;
                break;
            }
        }
        if (!matchInBoth)
            onlyInNext.push_back(nextColl);
    }

    // Assign the materials
    for (auto colliderEntity : onlyInPrevious)
    {
        if(ecs::IsEntityHandleValid(colliderEntity))
        {
            ecs::CompHandle<MaterialSwapperComponent> matSwapComp{ colliderEntity->GetComp<MaterialSwapperComponent>() };
            if (matSwapComp && !colliderEntity->GetComp<FlashComponent>())
            {
                matSwapComp->ToggleMaterialSwap(false);
            }
        }
    }
    for (auto colliderEntity : onlyInNext)
    {
        if (ecs::IsEntityHandleValid(colliderEntity))
        {
            ecs::CompHandle<MaterialSwapperComponent> matSwapComp{ colliderEntity->GetComp<MaterialSwapperComponent>() };

            if (!matSwapComp && !colliderEntity->GetComp<FlashComponent>())
            {
                if (colliderEntity->GetComp<EntityLayerComponent>()->GetLayer() == ENTITY_LAYER::ENVIRONMENT)
                {
                    matSwapComp = colliderEntity->AddComp(MaterialSwapperComponent{});
                    matSwapComp->swapMaterial = comp.translucentMaterial;
                }
            }


            if (matSwapComp && !colliderEntity->GetComp<FlashComponent>())
            {
                matSwapComp->ToggleMaterialSwap(true);
            }
        }
    }


    comp.currentColliders = currentColliders;
}
