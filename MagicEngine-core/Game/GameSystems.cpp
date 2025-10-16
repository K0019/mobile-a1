/******************************************************************************/
/*!
\file   GameSystems.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing state classes that manage which systems are
  loaded into ECS.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "GameSystems.h"
#include "ECSSysLayers.h"

#include "GraphicsECSMesh.h"
#include "PostProcessingComponent.h"
#include "TextSystem.h"
#include "ScriptComponent.h"
#include "CustomViewport.h"

#include "CheatCodes.h"
#include "CameraSystem.h"
#include "TweenECS.h"
#include "AudioListener.h"
#include "LightingSystem.h"
#include "GameCameraController.h"
#include "Audio.h"
#include "TrailSystem.h"
#include "PrefabSpawner.h"
#include "Physics.h"
#include "BehaviourTree.h"

#include "Demo.h"

void GameStateBase::OnExit()
{
	ecs::RemoveAllSystems();
}

void GameState_Common::OnEnter()
{
    ecs::AddSystem(ECS_LAYER::RENDER_0, RenderSystem{});
    ecs::AddSystem(ECS_LAYER::RENDER_0, TrailRendererDrawingSystem{});
    ecs::AddSystem(ECS_LAYER::RENDER_0, LightingSystem{});
    ecs::AddSystem(ECS_LAYER::RENDER_0, PostProcessingSystem{});
    ecs::AddSystem(ECS_LAYER::RENDER_UI_0, TextSystem{});
    
    // Camera (will be overriden by the game camera during simulation, see GameState_Game)
    ecs::AddSystem(ECS_LAYER::RENDER_0, CustomViewportCameraUploadSystem{});

    ecs::AddSystem(ECS_LAYER::PRE_PHYSICS_0, FPSTextSystem{});
    ecs::AddSystem(ECS_LAYER::AUDIO, AudioSystem{});
    ecs::AddSystem(ECS_LAYER::AUDIO, AudioListenerSystem{});
}

void GameState_Editor::OnEnter()
{
    GameState_Common::OnEnter();

    ecs::AddSystem(ECS_LAYER::PRE_PHYSICS_0, TrailRendererSystem{});
    ecs::AddSystem(ECS_LAYER::PRE_UPDATE_0, UndoShakeSystem{});
    ecs::AddSystem(ECS_LAYER::POST_PHYSICS_1, ShakeSystem{});

    //ecs::AddSystem(ECS_LAYER::POST_PHYSICS_0, ArmPivotIKSystem{}); // CHANGE LATER
}

void GameState_Game::OnEnter()
{
    GameState_Common::OnEnter();

    ecs::AddSystem(ECS_LAYER::PRE_UPDATE_0, UndoShakeSystem{});

    ecs::AddSystem(ECS_LAYER::PRE_PHYSICS_0, TrailRendererSystem{});
    ecs::AddSystem(ECS_LAYER::INPUT_0, CheatCodes{});
    ecs::AddSystem(ECS_LAYER::POST_PHYSICS_0, GameCameraControllerSystem{});
    ecs::AddSystem(ECS_LAYER::TWEENING, TweenSystem{});

    ecs::AddSystem(ECS_LAYER::SCRIPT_PREAWAKE, ScriptPreAwakeSystem{});
    ecs::AddSystem(ECS_LAYER::SCRIPT_AWAKE, ScriptAwakeSystem{});
    ecs::AddSystem(ECS_LAYER::SCRIPT_START, ScriptStartSystem{});
    ecs::AddSystem(ECS_LAYER::SCRIPT_UPDATE, ScriptSystem{});
    ecs::AddSystem(ECS_LAYER::SCRIPT_LATE_UPDATE, ScriptLateUpdateSystem{});

    ecs::AddSystem(ECS_LAYER::SCRIPT_UPDATE, BehaviorTreeSystem{});

    ecs::AddSystem(ECS_LAYER::POST_PHYSICS_0, AnchorToCameraSystem{});
    ecs::AddSystem(ECS_LAYER::POST_PHYSICS_1, ShakeSystem{});

    ecs::AddSystem(ECS_LAYER::POST_PHYSICS_0, PrefabSpawnSystem{});

    ecs::AddSystem(ECS_LAYER::PRE_PHYSICS_0, ExampleSystem{});

    ecs::AddSystem(ECS_LAYER::PHYSICS, physics::PhysicsSystem{});

    // Override custom viewport camera
    ecs::AddSystem(ECS_LAYER::RENDER_1, CameraCompUploadSystem{});
    
}

void GameState_Pause::OnEnter()
{
    GameState_Common::OnEnter();
}

void GameStateManager::Exit()
{
    SimpleStateMachine::SwitchToState(nullptr);
}
