#include "TestRegister.h"

#ifndef GLFW
#include <android/log.h>
#define LOG_TAG "ryEngine"
#endif

#include "Graphics/GraphicsECSMesh.h"
#include "EntityUID.h"
#include "Engine/Audio.h"
#include "Engine/BehaviorTree/BehaviourTree.h"
#include "Engine/EntityLayers.h"
#include "Graphics/CameraComponent.h"
#include "Graphics/LightComponent.h"
#include "Graphics/PostProcessingComponent.h"
#include "Graphics/TextComponent.h"
#include "Graphics/TrailComponent.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Scripting/ScriptComponent.h"
#include "Components/NameComponent.h"
#include "Game/GameCameraController.h"
#include "Game/Character.h"
#include "Game/PlayerCharacter.h"
#include "Game/EnemyCharacter.h"
#include "Game/GrabbableItem.h"
#include "Game/Health.h"

void RegisterShit()
{
	IRegisteredComponent<EntityUIDComponent>::RegisterComponent();
	IRegisteredComponent<AudioSourceComponent>::RegisterComponent();
	IRegisteredComponent<BehaviorTreeComp>::RegisterComponent();
	IRegisteredComponent<EntityLayerComponent>::RegisterComponent();
	IRegisteredComponent<ShakeComponent>::RegisterComponent();
	IRegisteredComponent<CameraComponent>::RegisterComponent();
	IRegisteredComponent<AnchorToCameraComponent>::RegisterComponent();
	IRegisteredComponent<RenderComponent>::RegisterComponent();
	IRegisteredComponent<LightComponent>::RegisterComponent();
	IRegisteredComponent<LightBlinkComponent>::RegisterComponent();
	IRegisteredComponent<PostProcessingComponent>::RegisterComponent();
	IRegisteredComponent<TextComponent>::RegisterComponent();
	IRegisteredComponent<FPSTextComponent>::RegisterComponent();
	IRegisteredComponent<TrailRendererComponent>::RegisterComponent();
	IRegisteredComponent<physics::BoxColliderComp>::RegisterComponent();
	IRegisteredComponent<physics::PhysicsComp>::RegisterComponent();
	IRegisteredComponent<NameComponent>::RegisterComponent();
	IRegisteredComponent<GameCameraControllerComponent>::RegisterComponent();
	IRegisteredComponent<CharacterMovementComponent>::RegisterComponent();
	IRegisteredComponent<PlayerMovementComponent>::RegisterComponent();
	IRegisteredComponent<GrabbableItemComponent>::RegisterComponent();
	IRegisteredComponent<EnemyComponent>::RegisterComponent();
	IRegisteredComponent<HealthComponent>::RegisterComponent();
}
