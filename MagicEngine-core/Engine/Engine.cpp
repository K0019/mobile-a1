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
#include "Game/GameSystems.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Console.h"
#include "Editor/Performance.h"
#include "Graphics/CustomViewport.h"
#include "Managers/AudioManager.h"
#include "Editor/AssetBrowser.h"
#include "Editor/PrefabWindow.h"
#include "Editor/Hierarchy.h"
#include "Editor/Popup.h"
#include "Editor/Editor.h"
#include "Utilities/CrashHandler.h"
#include "VFS/VFS.h"

#include "Engine/Events/EventsQueue.h"
#include "Engine/Input.h"

#include "Engine/SceneManagement.h"
#include "Engine/EntitySpawnEvents.h"
#include "Game/IGameComponentCallbacks.h"
#include "Tween/TweenManager.h"
#include "Engine/PrefabManager.h"
#include "GameSettings.h"
#include "Physics/JoltPhysics.h"

#include "Editor/Containers/GUIAsECS.h"
#include "Editor/SettingsWindow.h"
#include "Engine/BehaviorTree/BehaviourTreeFactory.h"
#include "Editor/BehaviourTreeWindow.h"
#include "Editor/LayersMatrix.h"
#include "Engine/EntityLayers.h"

#include "Scripting/CSScripting.h"
#include "Scripting/HotReloader.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Graphics/CameraController.h"
#include "Editor/Import.h"
#include "math/camera.h"

#include "Engine/Platform/Android/AndroidInputBridge.h"
#include"BehaviorTree/BehaviourNode.h"

#ifdef IMGUI_ENABLED
namespace
{
	// ImGui windows
	// dumb way to do it but sure i guess.
	bool show_browser = false;

	void saveState(const char* filename) {
		// TODO: Do proper state saving
		ecs::SwitchToPool(ecs::POOL::EDITOR_GUI);
		Serializer serializer{ filename };
		serializer.Serialize("show_console", ecs::GetCompsBegin<editor::Console>() != ecs::GetCompsEnd<editor::Console>());
		serializer.Serialize("show_performance", ST<PerformanceProfiler>::Get()->GetIsOpen());
		serializer.Serialize("show_editor", ST<Inspector>::Get()->GetIsOpen());
		//serializer.Serialize("show_settings", ST<SettingsWindow>::Get()->GetIsOpen());
		serializer.Serialize("show_browser", show_browser);
		serializer.Serialize("show_hierarchy", ST<Hierarchy>::Get()->isOpen);
		ecs::SwitchToPool(ecs::POOL::DEFAULT);
	}
	void loadState(const char* filename) {

		std::ifstream t(filename); //Should be safe, only used on windows
		std::stringstream buffer;
		buffer << t.rdbuf();
		Deserializer deserializer{ buffer.str()};

		//Deserializer deserializer{ filename };
		if (!deserializer.IsValid())
			return;

		bool b{};
		deserializer.DeserializeVar("show_console", &b);
		if (b)
			editor::CreateGuiWindow<editor::Console>();

		deserializer.DeserializeVar("show_performance", &b), ST<PerformanceProfiler>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_editor", &b), ST<Inspector>::Get()->SetIsOpen(b);
		//deserializer.DeserializeVar("show_settings", &b), ST<SettingsWindow>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_browser", &show_browser);
		deserializer.DeserializeVar("show_hierarchy", &ST<Hierarchy>::Get()->isOpen);
	}
}
#endif

#ifndef GLFW
#include <android/log.h>
#define LOG_TAG "ryEngine"
#endif

#include "ECS/TestRegister.h"

MagicEngine::MagicEngine() = default;

MagicEngine::~MagicEngine() = default;

void MagicEngine::MarkToShutdown()
{
	ST<GraphicsMain>::Get()->SetPendingShutdown();
}

bool MagicEngine::IsShuttingDown() const
{
	return ST<GraphicsMain>::Get()->GetIsPendingShutdown();
}


void MagicEngine::Init(Context& context)
{
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
	CrashHandler::SetupCrashHandler(); // DO NOT REMOVE THIS LINE EVER

	// Scripting MagicEngine Initialisation
#ifdef GLFW
	CSharpScripts::CSScripting::Init();
#endif

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
	//ST<AssetBrowser>::Get()->file_system.Initialize(Filepaths::workingDir);
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
#ifdef IMGUI_ENABLED
	ST<GameSystemsManager>::Get()->Init(GAMESTATE::EDITOR);
#else
	ST<GameSystemsManager>::Get()->Init(GAMESTATE::IN_GAME);
	//ST<SceneManager>::Get()->LoadScene("scenes/defaultscene.scene");
	ST<SceneManager>::Get()->ResetAndLoadPrevOpenScenes();
#endif

	auto timeafterwindow = std::chrono::high_resolution_clock::now();
	CONSOLE_LOG(LEVEL_INFO) << "Initialization: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeafterwindow - windowCreate).count() << "ms";

	// get ready for running
	// ---------------------
	//ST<GraphicsWindow>::Get()->BringWindowToFront();
#ifdef IMGUI_ENABLED
	loadState("imgui.json");
	editor::CreateGuiWindow<CustomViewport>(static_cast<unsigned int>(worldExtents.x), static_cast<unsigned int>(worldExtents.y));
#endif


#if defined(__ANDROID__)
	AndroidInputBridge::Initialize();
#endif

}

void MagicEngine::ExecuteFrame(FrameData& frameData)
{

	// Update tracking of framerate and frametime
	GameTime::WaitUntilNextFrame();
	ST<PerformanceProfiler>::Get()->StartFrame();

	// Clear the events of the previous frame
	ST<EventsQueue>::Get()->NewFrame();


	
	ST<MagicInput>::Get()->NewFrame();
	//GamepadInput::PollInput();

	ST<GraphicsMain>::Get()->BeginFrame();

	ST<physics::JoltPhysics>::Get()->DebugDraw();

#ifdef IMGUI_ENABLED
	ST<GraphicsMain>::Get()->BeginImGuiFrame();

	// TODO: Convert all of these window singletons into the ecs versions so we can support multiple instances of a single window.
	if(ST<PerformanceProfiler>::Get()->GetIsOpen())
	{
		ST<PerformanceProfiler>::Get()->Draw();
	}
	if(ST<Inspector>::Get()->GetIsOpen())
	{
		ST<Inspector>::Get()->Draw();
	}
	if(show_browser)
	{
		ST<AssetBrowser>::Get()->Draw(&show_browser);
	}
	if(ST<PrefabWindow>::Get()->IsOpen())
	{
		ST<PrefabWindow>::Get()->DrawSaveLoadPrompt(&ST<PrefabWindow>::Get()->IsOpen());
	}
	if(ST<Hierarchy>::Get()->isOpen)
	{
		ST<Hierarchy>::Get()->Draw();
	}
	ST<Popup>::Get()->Draw();
	//ST<Inspector>::Get()->RenderGrid();

	// Draw editor windows
	ecs::SwitchToPool(ecs::POOL::EDITOR_GUI);
	ecs::RunSystems(ECS_LAYER::PRE_PHYSICS_0); // Editor window systems are placed in this layer.
	ecs::FlushChanges(); // This deletes entities whose windows were closed (see WindowBase::OnOpenStateChanged())
	ecs::SwitchToPool(ecs::POOL::DEFAULT);
	ecs::FlushChanges(); // For if any of the editor windows deleted an entity.


	if(ImGui::BeginMainMenuBar())
	{
		// Add a "File" menu
		if(ImGui::BeginMenu("File"))
		{
			if(ImGui::MenuItem("New"))
			{
				// Handle "New" action
			}
			if(ImGui::MenuItem("Save"))
			{
				ST<SceneManager>::Get()->SaveAllScenes();
				ST<SceneManager>::Get()->SaveWhichScenesOpened();
			}
			if(ImGui::MenuItem("Settings"))
			{
				editor::CreateGuiWindow<editor::SettingsWindow>();
			}
			if(ImGui::MenuItem("Exit"))
			{
				MarkToShutdown();
			}
			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Tools"))
		{
			if(ImGui::MenuItem("Console"))
			{
				editor::CreateGuiWindow<editor::Console>();
				ImGui::SetWindowFocus(ICON_FA_TERMINAL"Console"); // Save the name of the windows somewhere else so i dont have to copy paste = ryan cheong
			}
			if(ImGui::MenuItem("Performance"))
			{
				ST<PerformanceProfiler>::Get()->SetIsOpen(true);
				ImGui::SetWindowFocus(ICON_FA_GAUGE_HIGH" Performance");
			}
			if(ImGui::MenuItem("Inspector"))
			{
				ST<Inspector>::Get()->SetIsOpen(true);
				ImGui::SetWindowFocus(ICON_FA_MAGNIFYING_GLASS" Inspector");
			}
			if(ImGui::MenuItem("Browser"))
			{
				show_browser = true;
				ImGui::SetWindowFocus(ICON_FA_FOLDER" Browser");
			}
			if(ImGui::MenuItem("Hierarchy", 0, false))
			{
				ST<Hierarchy>::Get()->isOpen = true;
				ImGui::SetWindowFocus(ICON_FA_SITEMAP" Hierarchy");
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Behaviour Tree"))
		{
			editor::CreateGuiWindow<editor::BehaviourTreeWindow>();
		}

		ImGui::EndMainMenuBar();  // End the main menu bar
	}

	ST<GraphicsMain>::Get()->EndImGuiFrame();
#endif


	// manage user input
	// -----------------
	ST<PerformanceProfiler>::Get()->StartProfile("Process Input");
#ifdef IMGUI_ENABLED
	ST<Inspector>::Get()->ProcessInput();
	// TODO: Put this in some editor windows manager class. In fact, all of this imgui stuff needs to be put in that class or subclasses.
	/*if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::GRAVE))
		ST<Console>::Get()->SetIsOpen(!ST<Console>::Get()->GetIsOpen());*/

#endif

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
	ST<PerformanceProfiler>::Get()->EndProfile("Process Input");

	// update game state
	// -----------------
#if defined(IMGUI_ENABLED) && defined(GLFW)
	CSharpScripts::CSScripting::CheckCompileUserAssemblyAsyncCompletion();
#endif
	ST<GameSystemsManager>::Get()->UpdateState(); // Update which ecs systems are active
	ExecuteUpdateSystems(); // Run ecs systems that update the world
	ST<Scheduler>::Get()->Update(GameTime::FixedDt() * static_cast<float>(GameTime::NumFixedFrames()));

	// render
	// ------

	// Game window draw
	ST<PerformanceProfiler>::Get()->StartProfile("Render");
	if (!ST<GraphicsWindow>::Get()->GetIsWindowMinimized())
	{
		ExecuteRenderSystems(); // Run ecs systems that render the world to the graphics pipeline
#ifdef IMGUI_ENABLED
		ST<Inspector>::Get()->DrawSelectedEntityBorder();
#endif
	}
	ST<GraphicsMain>::Get()->EndFrame(&frameData);
	ST<PerformanceProfiler>::Get()->EndProfile("Render");


	ST<PerformanceProfiler>::Get()->EndFrame();

#if defined(__ANDROID__)
	// read State() anywhere you need during the frame
	AndroidInputBridge::ClearEdges();   // once per frame after you've consumed it
#endif

}

void MagicEngine::shutdown()
{
#ifdef IMGUI_ENABLED
	saveState("imgui.json");
#endif

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
	ST<AudioManager>::Destroy();
	ST<TweenManager>::Destroy();
	ST<PerformanceProfiler>::Destroy();
	ST<AssetBrowser>::Destroy();
	
	ST<BTFactory>::Destroy();

#ifdef IMGUI_ENABLED
	ST<Inspector>::Destroy();
#endif
	ST<HiddenComponentsStore>::Destroy();
	ST<RegisteredComponents>::Destroy();
	ST<PrefabManager>::Destroy();
	ST<PrefabWindow>::Destroy();
#ifdef IMGUI_ENABLED
	ST<Hierarchy>::Destroy();
#endif

	ecs::Shutdown();

	ST<physics::JoltPhysics>::Destroy();
#ifdef GLFW
	CSharpScripts::CSScripting::Exit();
#endif

	ST<GameSettings>::Destroy();
	ST<ecs::RegisteredSystemsOperatingByLayer>::Destroy();

	ST<MagicResourceManager>::Get()->Shutdown();
	ST<MagicResourceManager>::Destroy();

	ST<GraphicsMain>::Destroy();
	ST<EventsQueue>::Destroy();
	// In case any systems send logs to the console while destructing.
	ST<internal::LoggedMessagesBuffer>::Destroy();
}

void MagicEngine::ExecuteUpdateSystems()
{
	auto UpdateSystemsGroup{ [](const std::string& profileName, void(*executeSystemsFunc)()) -> void {
#ifdef IMGUI_ENABLED
		ST<PerformanceProfiler>::Get()->StartProfile(profileName);
#endif
		executeSystemsFunc();
#ifdef IMGUI_ENABLED
		ST<PerformanceProfiler>::Get()->EndProfile(profileName);
#endif
	} };

	UpdateSystemsGroup("Input", []() -> void {
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_START, ECS_LAYER::CUTOFF_INPUT);
	});
	UpdateSystemsGroup("Pre-Physics", []() -> void {
		ST<TweenManager>::Get()->Update(GameTime::FixedDt());
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_INPUT, ECS_LAYER::CUTOFF_PRE_PHYSICS);
	});
	UpdateSystemsGroup("Scripting", []() -> void {
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PRE_PHYSICS, ECS_LAYER::CUTOFF_PRE_PHYSICS_SCRIPTS);
	});

	// Run physics on real time frames only
	UpdateSystemsGroup("Physics", []() -> void {
		for (int iterationsLeft{ GameTime::NumFixedFrames() }; iterationsLeft; --iterationsLeft)
			ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PRE_PHYSICS_SCRIPTS, ECS_LAYER::CUTOFF_PHYSICS);
	});

	UpdateSystemsGroup("Post-Physics", []() -> void {
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_PHYSICS, ECS_LAYER::CUTOFF_POST_PHYSICS);
	});
	UpdateSystemsGroup("Script-Late-Update", []() -> void {
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_POST_PHYSICS, ECS_LAYER::CUTOFF_POST_PHYSICS_SCRIPTS);
	});
}

void MagicEngine::ExecuteRenderSystems()
{
	ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_POST_PHYSICS_SCRIPTS, ECS_LAYER::CUTOFF_RENDER);
}
