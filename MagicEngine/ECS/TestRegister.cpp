#include "TestRegister.h"

#ifndef GLFW
#include <android/log.h>
#define LOG_TAG "ryEngine"
#endif

#include "Graphics/RenderComponent.h"
#include "Graphics/AnimationComponent.h"
#include "EntityUID.h"
#include "Engine/SceneTransition.h"
#include "Engine/Audio.h"
#include "Engine/BehaviorTree/BehaviourTree.h"
#include "Engine/Platform/Android/AndroidInputManager.h"
#include "Engine/EntityLayers.h"
#include "Engine/NavMesh.h"
#include "Engine/NavMeshAgent.h"
#include "Graphics/CameraComponent.h"
#include "Graphics/LightComponent.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Scripting/ScriptComponent.h"
#include "Components/NameComponent.h"
#include "Components/EntityReferenceHolder.h"
#include "Game/GameCameraController.h"
#include "Game/Character.h"
#include "Game/PlayerCharacter.h"
#include "Game/EnemyCharacter.h"
#include "Game/GrabbableItem.h"
#include "Game/Health.h"
#include "UI/SpriteComponent.h"
#include "UI/ButtonComponent.h"
#include "UI/SliderComponent.h"
#include "UI/TextComponent.h"
#include "UI/BarComponent.h"
#include "UI/HealthBarComponent.h"
#include "UI/DelusionBarComponent.h"
#include "UI/DelusionRankComponent.h"
#include "Engine/VideoPlayer.h"
#include "Engine/Platform/Android/AndroidUIDeleter.h"
#include "Engine/Platform/Android/AndroidMaterialSwapper.h"
#include "Engine/Platform/Desktop/DesktopUIDeleter.h"
#include "Game/FlashComponent.h"
#include "Game/MaterialSwapper.h"
#include "UI/RectTransform.h"
#include "3DUI/BillboardComponent.h"
#include "Game/Delusion.h"
#include "Graphics/BoneAttachment.h"
#include "Graphics/AnimatorComponent.h"

void RegisterShit()
{
	IRegisteredComponent<EntityUIDComponent>::RegisterComponent();
	IRegisteredComponent<SceneTransitionComponent>::RegisterComponent();
	IRegisteredComponent<AudioSourceComponent>::RegisterComponent();
	IRegisteredComponent<BehaviorTreeComp>::RegisterComponent();
	IRegisteredComponent<AndroidInputComp>::RegisterComponent();
	IRegisteredComponent<EntityLayerComponent>::RegisterComponent();
	IRegisteredComponent<ShakeComponent>::RegisterComponent();
	IRegisteredComponent<CameraComponent>::RegisterComponent();
	IRegisteredComponent<AnchorToCameraComponent>::RegisterComponent();
	IRegisteredComponent<RenderComponent>::RegisterComponent();
	IRegisteredComponent<AnimationComponent>::RegisterComponent();
	IRegisteredComponent<LightComponent>::RegisterComponent();
	IRegisteredComponent<LightBlinkComponent>::RegisterComponent();
	IRegisteredComponent<physics::BoxColliderComp>::RegisterComponent();
	IRegisteredComponent<physics::PhysicsComp>::RegisterComponent();
	IRegisteredComponent<NameComponent>::RegisterComponent();
	IRegisteredComponent<EntityReferenceHolderComponent>::RegisterComponent();
	IRegisteredComponent<GameCameraControllerComponent>::RegisterComponent();
	IRegisteredComponent<CharacterMovementComponent>::RegisterComponent();
	IRegisteredComponent<PlayerMovementComponent>::RegisterComponent();
	IRegisteredComponent<GrabbableItemComponent>::RegisterComponent();
	IRegisteredComponent<EnemyComponent>::RegisterComponent();
	IRegisteredComponent<HealthComponent>::RegisterComponent();
	IRegisteredComponent<ScriptComponent>::RegisterComponent();
	IRegisteredComponent<navmesh::NavMeshSurfaceComp>::RegisterComponent();
	IRegisteredComponent<navmesh::NavMeshAgentComp>::RegisterComponent();
	IRegisteredComponent<AudioListenerComponent>::RegisterComponent();
	IRegisteredComponent<SpriteComponent>::RegisterComponent();
	IRegisteredComponent<ButtonComponent>::RegisterComponent();
	IRegisteredComponent<SliderComponent>::RegisterComponent();
	IRegisteredComponent<TextComponent>::RegisterComponent();
	IRegisteredComponent<BarComponent>::RegisterComponent();
	IRegisteredComponent<VideoPlayerComponent>::RegisterComponent();
	IRegisteredComponent<HealthBarComponent>::RegisterComponent();
	IRegisteredComponent<DelusionBarComponent>::RegisterComponent();
	IRegisteredComponent<DelusionRankComponent>::RegisterComponent();
	IRegisteredComponent<AndroidUIDeleterComp>::RegisterComponent();
	IRegisteredComponent<DesktopUIDeleterComp>::RegisterComponent();
	IRegisteredComponent<FlashComponent>::RegisterComponent();
	IRegisteredComponent<MaterialSwapperComponent>::RegisterComponent();
	IRegisteredComponent<RectTransformComponent>::RegisterComponent();
	IRegisteredComponent<BillboardComponent>::RegisterComponent();
	IRegisteredComponent<DelusionComponent>::RegisterComponent();
	IRegisteredComponent<BoneAttachment>::RegisterComponent();
	IRegisteredComponent<AnimatorComponent>::RegisterComponent();
}

void PreloadShit()
{
	// Need WeaponInfo because characters will read from it immediately on first frame
	ST<AssetManager>::Get()->GetContainer<WeaponInfo>().RequestLoadAll();
}
