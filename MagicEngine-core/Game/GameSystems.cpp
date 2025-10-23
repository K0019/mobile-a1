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

#include "Game/GameSystems.h"
#include "ECS/ECSSysLayers.h"
#include "Utilities/Messaging.h"
#include "Engine/SceneManagement.h"
#include "Managers/AudioManager.h"

#include "Graphics/GraphicsECSMesh.h"
#include "Graphics/PostProcessingComponent.h"
#include "Graphics/TextSystem.h"
#include "Scripting/ScriptComponent.h"
#include "Graphics/CustomViewport.h"

#include "Game/CheatCodes.h"
#include "Graphics/CameraSystem.h"
#include "Tween/TweenECS.h"
#include "Game/AudioListener.h"
#include "Graphics/LightingSystem.h"
#include "Game/GameCameraController.h"
#include "Game/PlayerCharacter.h"
#include "Engine/Audio.h"
#include "Graphics/TrailSystem.h"
#include "Game/PrefabSpawner.h"
#include "Physics/Physics.h"
#include "Engine/BehaviorTree/BehaviourTree.h"

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
    ecs::AddSystem(ECS_LAYER::POST_PHYSICS_0, PlayerMovementComponentSystem{});
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

GameSystemsManager::GameSystemsManager()
    : state{ GAMESTATE::NONE }
    , nextState{ state }
    , flaggedForReset{ false }
{
}

void GameSystemsManager::Init(GAMESTATE initalState)
{
    nextState = initalState;
    UpdateState();

    Messaging::Subscribe("EngineTogglePlayMode", OnTogglePlayMode);
    Messaging::Subscribe("EngineTogglePauseMode", OnTogglePauseMode);
}

GAMESTATE GameSystemsManager::GetState() const
{
    return state;
}

void GameSystemsManager::UpdateState()
{
    // Don't update state if there are no changes
    if (nextState == state && !flaggedForReset)
        return;
    // Don't set the next state if we're not in the default ecs pool (we could be in the prefab window for example)
    if (ecs::GetCurrentPoolId() != ecs::POOL::DEFAULT)
        return;

    GAMESTATE prevState{ state };
    state = nextState;
    flaggedForReset = false;

    switch (state)
    {
    case GAMESTATE::NONE:
        Exit();
        break;
    case GAMESTATE::EDITOR:
        SwitchToState<GameState_Editor>();
        ST<SceneManager>::Get()->ResetAndLoadPrevOpenScenes();
        break;
    case GAMESTATE::IN_GAME:
        // Don't save scenes if we're resuming from pause mode.
        if (prevState == GAMESTATE::EDITOR)
        {
            ST<SceneManager>::Get()->SaveAllScenes();
            ST<SceneManager>::Get()->SaveWhichScenesOpened();
            Messaging::BroadcastAll("OnEngineSimulationStart");
        }
        SwitchToState<GameState_Game>();
        break;
    case GAMESTATE::PAUSE:
        SwitchToState<GameState_Pause>();
        break;
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "Unimplemented game state " << +state << '!';
        return;
    }
}

void GameSystemsManager::ResetState()
{
    flaggedForReset = true;
}

void GameSystemsManager::Exit()
{
    Messaging::Unsubscribe("EngineTogglePlayMode", OnTogglePlayMode);
    Messaging::Unsubscribe("EngineTogglePauseMode", OnTogglePauseMode);

    SimpleStateMachine::SwitchToState(nullptr);
}

void GameSystemsManager::OnTogglePlayMode()
{
    auto* gameStateManager{ ST<GameSystemsManager>::Get() };
    switch (gameStateManager->GetState())
    {
    case GAMESTATE::EDITOR:
        gameStateManager->nextState = GAMESTATE::IN_GAME;
        break;
    case GAMESTATE::NONE:
    case GAMESTATE::IN_GAME:
    case GAMESTATE::PAUSE:
    {
        gameStateManager->nextState = GAMESTATE::EDITOR;

        // Stop all sounds
        // TODO: Use event system for this
        ST<AudioManager>::Get()->StopAllSounds();
        break;
    }
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "class Game has not implemented TogglePlayMode() for state " << +gameStateManager->GetState() << '!';
    }
}
void GameSystemsManager::OnTogglePauseMode()
{
    auto* gameStateManager{ ST<GameSystemsManager>::Get() };
    switch (gameStateManager->GetState())
    {
    case GAMESTATE::EDITOR:
        CONSOLE_LOG(LEVEL_ERROR) << "Game state should not toggle pause mode when in editor mode!";
        break;
    case GAMESTATE::IN_GAME:
        gameStateManager->nextState = GAMESTATE::PAUSE;
        break;
    case GAMESTATE::PAUSE:
        gameStateManager->nextState = GAMESTATE::IN_GAME;
        break;
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "class Game has not implemented TogglePauseMode() for state " << +gameStateManager->GetState() << '!';
    }
}
