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
#include "Engine.h"

#include "SceneManagement.h"
#include "EntitySpawnEvents.h"
#include "IGameComponentCallbacks.h"
#include "TweenManager.h"
#include "PrefabManager.h"
#include "GameSettings.h"
#include "JoltPhysics.h"

#include "SettingsWindow.h"
#include "LayersMatrix.h"
#include "EntityLayers.h"

#include "CSScripting.h"
#include "HotReloader.h"

#include "GraphicsAPI.h"
#include "camera.h"
#include "Import.h"
#include "Filesystem.h"

#ifdef IMGUI_ENABLED
namespace
{
	// ImGui windows
	// dumb way to do it but sure i guess.
	bool show_demo_window = false;
	bool show_browser = false;

	void saveState(const char* filename) {
		Serializer serializer{ filename };
		serializer.Serialize("show_demo_window", show_demo_window);
		serializer.Serialize("show_console", ST<Console>::Get()->GetIsOpen());
		serializer.Serialize("show_performance", ST<PerformanceProfiler>::Get()->GetIsOpen());
		serializer.Serialize("show_editor", ST<Inspector>::Get()->GetIsOpen());
		//serializer.Serialize("show_settings", ST<SettingsWindow>::Get()->GetIsOpen());
		serializer.Serialize("show_browser", show_browser);
		serializer.Serialize("show_hierarchy", ST<Hierarchy>::Get()->isOpen);
	}
	void loadState(const char* filename) {
		Deserializer deserializer{ filename };
		if (!deserializer.IsValid())
			return;

		bool b{};
		deserializer.DeserializeVar("show_demo_window", &show_demo_window);
		deserializer.DeserializeVar("show_console", &b), ST<Console>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_performance", &b), ST<PerformanceProfiler>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_editor", &b), ST<Inspector>::Get()->SetIsOpen(b);
		//deserializer.DeserializeVar("show_settings", &b), ST<SettingsWindow>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_browser", &show_browser);
		deserializer.DeserializeVar("show_hierarchy", &ST<Hierarchy>::Get()->isOpen);
	}
}
#endif

Engine::Engine() = default;

Engine::~Engine() = default;

void Engine::setFPS(double _fps)
{
	this->fps = _fps;
	if(_fps > 0.0) {
		const double frameTimeNs = 1e9 / _fps;
		m_targetFrameTime = duration(static_cast<int64_t>(frameTimeNs));
	}
	else {
		m_targetFrameTime = duration::zero();
	}
	m_lastFrameTime = clock::now();
}

void Engine::wait()
{
	// Skip timing if no FPS limit is set (m_targetFrameTime will be zero)
	if(m_targetFrameTime == duration::zero()) {
		return;
	}

	// Get current time - using steady_clock prevents issues with system time changes
	const time_point now = clock::now();

	// Calculate when this frame should end based on the perfect frame sequence
	// Instead of measuring from 'now', we measure from the last frame time
	// This prevents error accumulation that would cause FPS drift
	const time_point targetTime = m_lastFrameTime + m_targetFrameTime;

	// Only wait if we're ahead of schedule
	if(now < targetTime) {
		// Calculate how long we need to wait
		const auto remainingTime = targetTime - now;

		// For longer waits (>500µs by default), use sleep_for first
		// This saves CPU compared to pure spin-waiting
		// We stop sleeping SPIN_THRESHOLD before the target to account for
		// sleep_for's inaccuracy (OS might wake us up a few ms late)
		// Thread keeps running, actively checking time
		// Like watching the clock tick instead of using an alarm
		// This is more CPU-intensive but more accurate than sleep_for

		if(remainingTime > SPIN_THRESHOLD) {
			std::this_thread::sleep_for(remainingTime - SPIN_THRESHOLD);
		}

		// Fine-tune the remaining time with spin-waiting
		// yield() allows other threads to run during the spin-wait
		while(clock::now() < targetTime) {
			std::this_thread::yield();
		}
	}

	// Update our frame time tracking
	// We use targetTime instead of now to maintain perfect frame pacing
	// If we're running behind (now > targetTime), this sets up the next frame
	// to be relative to where this frame SHOULD have been, not where it actually was
	// This helps maintain consistent frame pacing even if some frames take too long
	m_lastFrameTime = targetTime;


	// JUST SET TO UNLIMITED FOR 99% OF THE TIME, BUT THIS WORKS
}

void Engine::MarkToShutdown()
{
	ST<GraphicsMain>::Get()->SetPendingShutdown();
}

bool Engine::IsShuttingDown() const
{
	return ST<GraphicsMain>::Get()->GetIsPendingShutdown();
}

void Engine::init()
{
	ST<GameSettings>::Get()->Load(); // Only load settings from file first so we have the correct filepaths.

	ST<Console>::Get()->SetupCrashHandler(); // DO NOT REMOVE THIS LINE EVER

	// Scripting Engine Initialisation
	CSharpScripts::CSScripting::Init();

	// FMOD Initialisation
	ST<AudioManager>::Get()->Initialise();

	// Jolt Physics Initialisation
	physics::JoltRegister();
	ST<physics::JoltPhysics>::Get()->Initialize();

	auto windowCreate = std::chrono::high_resolution_clock::now();

	// Graphics initialization
	auto graphicsMain{ ST<GraphicsMain>::Get() };
	graphicsMain->Init();
	graphicsMain->SetCallback_DragDrop(import::DropCallback);

	ST<GameSettings>::Get()->Apply(); // Apply the loaded settings here

#ifdef _DEBUG
	// identify file path for loading asset files
	CONSOLE_LOG(LEVEL_INFO) << "Current working directory: " << std::filesystem::canonical(ST<Filepaths>::Get()->workingDir);
	CONSOLE_LOG(LEVEL_INFO) << "Actual working directory: " << std::filesystem::current_path();
#endif

	// load resources
	//ST<AssetBrowser>::Get()->file_system.Initialize(ST<Filepaths>::Get()->workingDir);
	ResourceManager::LoadAssetsFromFile(ST<Filepaths>::Get()->workingDir + "/Assets/assets.json");
	// Load fonts manually for now
	const std::array<std::string, 3> fontsToLoad{
		ST<Filepaths>::Get()->fontsSave + "/Arial.ttf",
		ST<Filepaths>::Get()->fontsSave + "/Lato-Regular.ttf",
		ST<Filepaths>::Get()->fontsSave + "/slkscre.ttf"
	};
	//std::for_each(fontsToLoad.begin(), fontsToLoad.end(), ResourceManager::LoadFont);
	
	// initialize game
	// ---------------
	ecs::Initialize();
	ST<SceneManager>::Get(); // Initialize scene manager
	ST<EntitySpawnEvents>::Get(); // Initialize systems that listen for entity created events

	auto worldExtents{ graphicsMain->GetWorldExtent() };
#ifdef IMGUI_ENABLED
	ST<Game>::Get()->Init(worldExtents.width, worldExtents.height, GAMESTATE::EDITOR);
#else
	ST<Game>::Get()->Init(worldExtents.width, worldExtents.height, GAMESTATE::IN_GAME);
#endif

	auto timeafterwindow = std::chrono::high_resolution_clock::now();
	CONSOLE_LOG(LEVEL_INFO) << "Initialization: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeafterwindow - windowCreate).count() << "ms";

	// get ready for running
	// ---------------------
	graphicsMain->BringWindowToFront();
	loadState("imgui.json");

	// TODO: Use a proper scene
	graphicsMain->LoadSampleScene();
}

void Engine::run()
{
	while (!ST<GraphicsMain>::Get()->GetIsPendingShutdown())
	{
		// Wait till the start of this frame
		wait();

		// Update tracking of framerate and frametime
#ifdef IMGUI_ENABLED
		ImGuiIO& io = ImGui::GetIO();
		GameTime::SetFps(io.Framerate);
#else
		GameTime::SetFps(ST<PerformanceProfiler>::Get()->GetFPS());
#endif
		ST<PerformanceProfiler>::Get()->StartFrame();
		GameTime::NewFrame(ST<PerformanceProfiler>::Get()->GetDeltaTime());

		// Only reset key states when systems are updating so we don't skip inputs.
		if(GameTime::RealNumFixedFrames())
		{
			ST<Input>::Get()->NewFrame();
			glfwPollEvents();
			//GamepadInput::PollInput();
		}

		ST<GraphicsMain>::Get()->BeginFrame();
#ifdef IMGUI_ENABLED
		ST<GraphicsMain>::Get()->BeginImGuiFrame();

		if(show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// TODO: Convert all of these window singletons into the ecs versions so we can support multiple instances of a single window.
		if(ST<Console>::Get()->GetIsOpen())
		{
			ST<PerformanceProfiler>::Get()->StartProfile("Console");
			ST<Console>::Get()->Draw();
			ST<PerformanceProfiler>::Get()->EndProfile("Console");
		}
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
					editor::CreateWindow<editor::SettingsWindow>();
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
					ST<Console>::Get()->SetIsOpen(true);
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

			ImGui::EndMainMenuBar();  // End the main menu bar
		}

		ST<CustomViewport>::Get()->DrawImGuiWindow();

		ST<GraphicsMain>::Get()->EndImGuiFrame();
#endif


		// manage user input
		// -----------------
		ST<PerformanceProfiler>::Get()->StartProfile("Process Input");
		if(GameTime::RealNumFixedFrames())
		{
#ifdef IMGUI_ENABLED
			ST<Inspector>::Get()->ProcessInput();
			if(ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::GRAVE))
				ST<Console>::Get()->SetIsOpen(!ST<Console>::Get()->GetIsOpen());
			if(ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F1))
				show_demo_window = true;
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
		}
		ST<PerformanceProfiler>::Get()->EndProfile("Process Input");

		// update game state
		// -----------------
#ifdef IMGUI_ENABLED
		CSharpScripts::CSScripting::CheckCompileUserAssemblyAsyncCompletion();
#endif
		ST<Game>::Get()->Update();
		ST<Scheduler>::Get()->Update(GameTime::FixedDt() * static_cast<float>(GameTime::NumFixedFrames()));

		// render
		// ------

		// Game window draw
		ST<PerformanceProfiler>::Get()->StartProfile("Render");
		if (!ST<GraphicsMain>::Get()->GetIsWindowMinimized())
		{
			ST<Game>::Get()->Render();
			ST<Inspector>::Get()->DrawSelectedEntityBorder();
			ST<GraphicsMain>::Get()->RenderSampleScene();
		}
		ST<GraphicsMain>::Get()->EndFrame();
		ST<PerformanceProfiler>::Get()->EndProfile("Render");


		ST<PerformanceProfiler>::Get()->EndFrame();
	}

#ifdef IMGUI_ENABLED
	saveState("imgui.json");
#endif

	ST<GameSettings>::Get()->Save();
	ResourceManager::SaveAssetsToFile(ST<Filepaths>::Get()->assetsJson);
}

void Engine::shutdown() {

	// Clean up your subsystems
	ST<Game>::Get()->Shutdown();
	ST<Game>::Destroy();
	ST<GameComponentCallbacksHandler>::Destroy();
	ST<EntitySpawnEvents>::Destroy();
	ST<SceneManager>::Destroy();
	ResourceManager::Clear();
	// Singletons
	ST<AudioManager>::Destroy();
	ST<TweenManager>::Destroy();
	ST<PerformanceProfiler>::Destroy();
	ST<AssetBrowser>::Destroy();
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
	CSharpScripts::CSScripting::Exit();

	ST<GameSettings>::Destroy();
	//ST<Filepaths>::Destroy(); // Filepaths kinda needs to live for other threads to reference filepaths... smart pointers will free this later. sry about this
	ST<ecs::RegisteredSystemsOperatingByLayer>::Destroy();

	ST<GraphicsMain>::Destroy();
	// In case any systems send logs to the console while destructing.
	ST<Console>::Destroy();
}
