/******************************************************************************/
/*!
\file   Engine.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (95%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Matthew Chan Shao Jie (5%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
This file contains the declaration of the Engine class.
The Engine class is responsible for initializing the game engine, running the game loop, and shutting down the engine.
It includes methods for initializing the engine, running the game loop, and shutting down the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Engine/Engine.h"
#include "renderer/frame_data.h"
#include "Game/GameSystems.h"
#include "Engine/Resources/ResourceManager.h"

// Permanent systems (engine-side components)
#include "Editor/Gizmo.h"
#include "Managers/AudioManager.h"

#include "Utilities/CrashHandler.h"
#include "VFS/VFS.h"

#include "Engine/Events/EventsQueue.h"
#include "Engine/Input.h"

#include "Engine/SceneManagement.h"
#include "Engine/SceneTransition.h"
#include "Engine/EntitySpawnEvents.h"
#include "Game/IGameComponentCallbacks.h"
#include "Tween/TweenManager.h"
#include "Engine/PrefabManager.h"
#include "GameSettings.h"
#include "Physics/JoltPhysics.h"

#include "Editor/Containers/GUIAsECS.h"
#include "Engine/BehaviorTree/BehaviourTreeFactory.h"
#include "Engine/EntityLayers.h"

#include "Scripting/LuaScripting.h"
#include "Scripting/HotReloader.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Graphics/CameraController.h"
#include "math/camera.h"

#include "Engine/Platform/Android/AndroidInputBridge.h"
#include"BehaviorTree/BehaviourNode.h"

#ifndef GLFW
#include <android/log.h>
#define LOG_TAG "ryEngine"
#endif

#include "ECS/TestRegister.h"

void MagicEngine::Init(Context& context, bool startInGameMode)
{
	CrashHandler::SetupCrashHandler(); // DO NOT REMOVE THIS LINE EVER

	RegisterShit();
#ifdef GLFW
	// The ifdef is to prevent double loading on android's side.
	// A temporary thing while I decide where android and windows directory adding goes.
	// Right now, android's is inside android_main.cpp.
	//VFS::MountDirectory("assets/", Filepaths::assets);
	VFS::MountDirectory("", Filepaths::assets);
#endif

	ST<GameSettings>::Get()->Load(); // Only load settings from file first so we have the correct filepaths.
#ifdef _DEBUG
	// identify file path for loading asset files
	CONSOLE_LOG(LEVEL_INFO) << "Current working directory: " << std::filesystem::canonical(Filepaths::workingDir);
	CONSOLE_LOG(LEVEL_INFO) << "Actual working directory: " << std::filesystem::current_path();
#endif

	// Scripting MagicEngine Initialisation
	ST<LuaScripting>::Get()->Init();

	// FMOD Initialisation
	ST<AudioManager>::Get()->Initialise();

	// Jolt Physics Initialisation
	physics::JoltRegister();
	ST<physics::JoltPhysics>::Get()->Initialize();

	auto windowCreate = std::chrono::high_resolution_clock::now();

	// Graphics initialization
	ST<GraphicsMain>::Get()->Init(context);
	//graphicsMain->SetCallback_DragDrop(import::DropCallback);

	ST<GameSettings>::Get()->Apply(); // Apply the loaded settings here


	ST<BTFactory>::Get()->SetAllFilePath();

	// load resources
	ST<MagicResourceManager>::Get()->Init();
	ST<MagicResourceManager>::Get()->LoadFromFile();
	// Load fonts manually for now
	const std::array<std::string, 3> fontsToLoad{
		Filepaths::fontsSave + "/Arial.ttf",
		Filepaths::fontsSave + "/Lato-Regular.ttf",
		Filepaths::fontsSave + "/slkscre.ttf"
	};
	//std::for_each(fontsToLoad.begin(), fontsToLoad.end(), ResourceManagerOld::LoadFont);
	
	// initialize game
	// ---------------
	ecs::Initialize();
	ST<SceneManager>::Get(); // Initialize scene manager
	ST<EntitySpawnEvents>::Get(); // Initialize systems that listen for entity created events

	//auto worldExtents{ ST<GraphicsWindow>::Get()->GetWorldExtent()};
	auto worldExtents{ IntVec2{ 1920, 1080 } };
	ST<CameraController>::Get()->SetCameraData(CameraData{
		.position = Vec3{static_cast<float>(worldExtents.x) / 2, static_cast<float>(worldExtents.y) / 2, 0.0f },
		.zoom = 1.0f
	});

	// Load ecs systems
	LoadPermanentSystems();

	// Initialize game state - application controls whether to start in game mode
	if (startInGameMode) {
		ST<GameSystemsManager>::Get()->Init(GAMESTATE::IN_GAME);
		ST<SceneManager>::Get()->ResetAndLoadPrevOpenScenes();
	} else {
		ST<GameSystemsManager>::Get()->Init(GAMESTATE::EDITOR);
	}

	auto timeafterwindow = std::chrono::high_resolution_clock::now();
	CONSOLE_LOG(LEVEL_INFO) << "Initialization: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeafterwindow - windowCreate).count() << "ms";

	// get ready for running
	// ---------------------
	//ST<GraphicsWindow>::Get()->BringWindowToFront();

#if defined(__ANDROID__)
	AndroidInputBridge::Initialize();
#endif

}

void MagicEngine::ExecuteFrame(RenderFrameData& frameData)
{
	// Update tracking of framerate and frametime
	GameTime::WaitUntilNextFrame();

	// Clear the events of the previous frame
	ST<EventsQueue>::Get()->NewFrame();

	ST<MagicInput>::Get()->NewFrame();
	//GamepadInput::PollInput();

	ST<GraphicsMain>::Get()->BeginFrame();

	if (ST<GameSettings>::Get()->m_physicsDebugDraw)
		ST<physics::JoltPhysics>::Get()->DebugDraw();

	ST<GraphicsMain>::Get()->BeginImGuiFrame();

	// Run permanent editor systems (not windows)
	ecs::RunSystems(ECS_LAYER::PERMANENT_EDITOR);

	// Draw editor windows
	ecs::SwitchToPool(ecs::POOL::EDITOR_GUI);
	ecs::RunSystems(ECS_LAYER::PRE_PHYSICS_0); // Editor window systems are placed in this layer.
	ecs::FlushChanges(); // This deletes entities whose windows were closed (see WindowBase::OnOpenStateChanged())
	ecs::SwitchToPool(ecs::POOL::DEFAULT);
	ecs::FlushChanges(); // For if any of the editor windows deleted an entity.

	ST<GraphicsMain>::Get()->EndImGuiFrame();


	// manage user input
	// -----------------
	ecs::RunSystems(ECS_LAYER::PERMANENT_INPUT);

	if(ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F11))
	{
		if(ST<GameSettings>::Get()->m_fullscreenMode == 0)
		{
			ST<GameSettings>::Get()->m_fullscreenMode = 1;
		}
		else
		{
			ST<GameSettings>::Get()->m_fullscreenMode = 0;
		}
		ST<GameSettings>::Get()->Apply();
	}

	// update game state
	// -----------------
	ST<GameSystemsManager>::Get()->UpdateState(); // Update which ecs systems are active
	ExecuteUpdateSystems(); // Run ecs systems that update the world
	ecs::FlushChanges();
	ST<Scheduler>::Get()->Update(GameTime::FixedDt() * static_cast<float>(GameTime::NumFixedFrames()));
	ecs::FlushChanges();

	// render
	// ------

	// Game window draw
	if (!ST<GraphicsWindow>::Get()->GetIsWindowMinimized())
		ExecuteRenderSystems(); // Run ecs systems that render the world to the graphics pipeline

	// Ensure primary view exists and has basic frame info
	FrameData& primaryView = EnsureView(frameData, 0);
	primaryView.deltaTime = GameTime::FixedDt();
	primaryView.screenWidth = static_cast<uint32_t>(Core::Display().GetWidth());
	primaryView.screenHeight = static_cast<uint32_t>(Core::Display().GetHeight());
	primaryView.viewportWidth = static_cast<float>(primaryView.screenWidth);
	primaryView.viewportHeight = static_cast<float>(primaryView.screenHeight);

	// EndFrame populates camera matrices for ALL views from GraphicsMain::frameData (set by SetViewCamera)
	ST<GraphicsMain>::Get()->EndFrame(&frameData);

#if defined(__ANDROID__)
	// read State() anywhere you need during the frame
	AndroidInputBridge::ClearEdges();   // once per frame after you've consumed it
#endif

}

void MagicEngine::shutdown()
{
	ST<GameSettings>::Get()->Save();
	ST<MagicResourceManager>::Get()->SaveToFile();

	// Clean up your subsystems
	ST<GameSystemsManager>::Get()->Exit();
	ST<GameSystemsManager>::Destroy();
	ST<GameComponentCallbacksHandler>::Destroy();
	ST<EntitySpawnEvents>::Destroy();
	ST<SceneManager>::Destroy();
	ST<MagicResourceManager>::Destroy();
	//ResourceManagerOld::Clear();
	// Singletons
	ST<TweenManager>::Destroy();

	ST<BTFactory>::Destroy();

	ST<HiddenComponentsStore>::Destroy();
	ST<RegisteredComponents>::Destroy();
	ST<PrefabManager>::Destroy();
	ST<History>::Destroy();

	ecs::Shutdown();

	ST<LuaScripting>::Destroy();
	ST<AudioManager>::Destroy();
	ST<physics::JoltPhysics>::Destroy();

	ST<GameSettings>::Destroy();
	ST<ecs::RegisteredSystemsOperatingByLayer>::Destroy();

	ST<MagicResourceManager>::Get()->Shutdown();
	ST<MagicResourceManager>::Destroy();

	ST<GraphicsMain>::Destroy();
	ST<EventsQueue>::Destroy();
	// In case any systems send logs to the console while destructing.
	ST<internal::LoggedMessagesBuffer>::Destroy();
}

void MagicEngine::LoadPermanentSystems()
{
	ecs::AddSystem(ECS_LAYER::PERMANENT_UPDATE, SceneTransitionSystem{});

	// Editor systems are now registered by the editor executable (see editor/application.cpp)
}

void MagicEngine::ExecuteUpdateSystems()
{
	ecs::RunSystems(ECS_LAYER::PERMANENT_UPDATE);

	// Input
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PERMANENT, ECS_LAYER::CUTOFF_INPUT);

	// Pre-Physics
	ST<TweenManager>::Get()->Update(GameTime::FixedDt());
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_INPUT, ECS_LAYER::CUTOFF_PRE_PHYSICS);

	// Scripting
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PRE_PHYSICS, ECS_LAYER::CUTOFF_PRE_PHYSICS_SCRIPTS);

	// Run physics on real time frames only
	for (int iterationsLeft{ GameTime::NumFixedFrames() }; iterationsLeft; --iterationsLeft)
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PRE_PHYSICS_SCRIPTS, ECS_LAYER::CUTOFF_PHYSICS);

	// Post-Physics
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PHYSICS, ECS_LAYER::CUTOFF_POST_PHYSICS);

	// Script-Late-Update
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_POST_PHYSICS, ECS_LAYER::CUTOFF_POST_PHYSICS_SCRIPTS);
}

void MagicEngine::ExecuteRenderSystems()
{
	// 3D world
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_POST_PHYSICS_SCRIPTS, ECS_LAYER::CUTOFF_RENDER);

	ecs::RunSystems(ECS_LAYER::PERMANENT_RENDER);
}
