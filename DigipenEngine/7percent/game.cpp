/******************************************************************************/
/*!
\file   Game.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (80%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (10%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\author Chan Kuan Fu Ryan (10%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  This is the source file for the game scene.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "game.h"
#include "SceneManagement.h"
#include "Editor.h"
#include "FunctionQueue.h" // For button's scene loading
#include "GameSettings.h"

#include "ECSSysLayers.h"
#include "Collision.h"
#include "NameComponent.h"
#include "AnimatorComponent.h"
#include "RenderComponent.h"
#include "CameraController.h"
#include "ScriptComponent.h"
#include "NameComponent.h"
#include "Button.h"

#include "ResourceManager.h"
#include "Performance.h"
#include "TweenManager.h"
#include "AudioManager.h"

// Game-related State data

Game::Game()
    : state{ GAMESTATE::NONE }
    , nextState{ state }
    , Width{ 0 }
    , Height{ 0 }
    , flaggedForReset{ false }
{
}

Game::~Game()
{
}

void Game::Init(unsigned int width, unsigned int height, GAMESTATE firstState)
{
    Width = width, Height = height;

    // load shaders
#ifdef IMGUI_ENABLED
    ST<Engine>::Get()->_vulkan->_renderer->resize(Width, Height);
    ST<CustomViewport>::Get()->Init(Width, Height);
#endif
    ST<CameraController>::Get()->SetCameraData(CameraData{
        .position = Vector2{static_cast<float>(Width / 2), static_cast<float>(Height) / 2 },
        .zoom = 1.0f
    });

    ST<MaterialSystem>::Get()->initialize();
    // Initialize editor systems
    SetState(firstState);
    UpdateState();

    // Attempt to load scenes
    if (firstState != GAMESTATE::EDITOR) // I'm sorry for this check, this is to guard against loading scenes twice when we're in editor mode. Could maybe be avoided with a bool flag somewhere.
        ST<SceneManager>::Get()->ResetAndLoadPrevOpenScenes();

    // TEMPORARY: Listen for window focus callback so we can automatically load the pause menu
    Messaging::Subscribe("OnWindowFocus", OnFocusCallback);
}

void Game::Update()
{
    if (flaggedForReset)
    {
        // Reload current state
        UpdateState(state);
        flaggedForReset = false;
        return;
    }

    UpdateState();

    // Calculate number of realtime iterations (mainly for UI and other systems that don't run off of timescale)
    for (int realtimeIterationsLeft{ GameTime::RealNumFixedFrames() }; realtimeIterationsLeft; --realtimeIterationsLeft)
        ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_START, ECS_LAYER::CUTOFF_REALTIME_INPUT);

    // Calculate how many iterations to run this frame.
    int iterationsLeft{ GameTime::NumFixedFrames() };
    if (iterationsLeft <= 0)
        return; // No need to update this frame...
    else if (iterationsLeft > 1)
        CONSOLE_LOG(LEVEL_INFO) << "Running behind by " << iterationsLeft - 1 << " frames. Catching up...";

    for (; iterationsLeft; --iterationsLeft)
    {
        ProcessInput();

        UpdateSystemsGroup("Pre-Physics", []() -> void {
            ST<TweenManager>::Get()->Update(GameTime::FixedDt());
            ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_INPUT, ECS_LAYER::CUTOFF_PRE_PHYSICS);
        });
        UpdateSystemsGroup("Scripting", []() -> void {
            ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PRE_PHYSICS, ECS_LAYER::CUTOFF_PRE_PHYSICS_SCRIPTS);
        });
        UpdateSystemsGroup("Physics", []() -> void {
            ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PRE_PHYSICS_SCRIPTS, ECS_LAYER::CUTOFF_PHYSICS);
        });
        UpdateSystemsGroup("Post-Physics", []() -> void {
            ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PHYSICS, ECS_LAYER::CUTOFF_POST_PHYSICS);
        });
        UpdateSystemsGroup("Script-Late-Update", []() -> void {
            ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_POST_PHYSICS, ECS_LAYER::CUTOFF_POST_PHYSICS_SCRIPTS);
        });

        FunctionQueue::ExecuteQueuedOperations(); // For button's scene queuing

        Input::NewIteration();
    }
}

void Game::ProcessInput()
{
    // Updates player movement and animation
    ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_REALTIME_INPUT, ECS_LAYER::CUTOFF_INPUT);
}

void Game::Render()
{
    ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_POST_PHYSICS, ECS_LAYER::CUTOFF_RENDER);
}

void Game::Shutdown()
{
    Messaging::Unsubscribe("OnWindowFocus", OnFocusCallback);
    gameStateManager.Exit();
}

GAMESTATE Game::GetState() const
{
    return state;
}

void Game::SetState(GAMESTATE newState)
{
    nextState = newState;
}

void Game::ResetState()
{
    flaggedForReset = true;
}

void Game::TogglePlayMode()
{
    switch (state)
    {
    case GAMESTATE::EDITOR:
        SetState(GAMESTATE::IN_GAME);
        break;
    case GAMESTATE::NONE:
    case GAMESTATE::IN_GAME:
    case GAMESTATE::PAUSE:
    {
        SetState(GAMESTATE::EDITOR);

        // Stop all sounds
        ST<AudioManager>::Get()->StopAllSounds();
        break;
    }
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "class Game has not implemented TogglePlayMode() for state " << static_cast<int>(state) << '!';
    }
}

void Game::TogglePauseMode()
{
    switch (state)
    {
    case GAMESTATE::EDITOR:
        CONSOLE_LOG(LEVEL_ERROR) << "Game state should not toggle pause mode when in editor mode!";
        break;
    case GAMESTATE::IN_GAME:
        SetState(GAMESTATE::PAUSE);
        break;
    case GAMESTATE::PAUSE:
        SetState(GAMESTATE::IN_GAME);
        break;
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "class Game has not implemented TogglePauseMode() for state " << static_cast<int>(state) << '!';
    }
}

void Game::UpdateState()
{
    if (nextState == state)
        return;

    UpdateState(nextState);
}

void Game::UpdateState(GAMESTATE newState)
{
    // Don't set the next state if we're not in the default ecs pool (we could be in the prefab window for example)
    if (ecs::GetCurrentPoolId() != ecs::POOL::DEFAULT)
        return;

    GAMESTATE prevState{ state };
    state = newState;

    switch (newState)
    {
    case GAMESTATE::NONE:
        gameStateManager.Exit();
        break;
    case GAMESTATE::EDITOR:
        gameStateManager.SwitchToState<GameState_Editor>();
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
        gameStateManager.SwitchToState<GameState_Game>();
        break;
    case GAMESTATE::PAUSE:
        gameStateManager.SwitchToState<GameState_Pause>();
        break;
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "Unimplemented game state " << static_cast<int>(prevState) << '!';
        return;
    }
}

void Game::UpdateSystemsGroup(const std::string& profileName, void(*executeSystemsFunc)())
{
    ST<PerformanceProfiler>::Get()->StartProfile(profileName);
    executeSystemsFunc();
    ST<PerformanceProfiler>::Get()->EndProfile(profileName);
}

void Game::OnFocusCallback(bool isFocused)
{
    UNREFERENCED_PARAMETER(isFocused);
    // To use for future purposes...
}
