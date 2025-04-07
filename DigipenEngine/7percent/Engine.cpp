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

#include "SettingsWindow.h"
#include "LayersMatrix.h"
#include "EntityLayers.h"

#include "ryan-c/Renderer.h"
#include "ryan-c/VulkanHelper.h"
#include "CSScripting.h"
#include "HotReloader.h"

#include "fa.h"

#include <stb/stb_image.h>

#include "Import.h"
#include "Filesystem.h"
#include "FunctionQueue.h"

namespace {

	void key_cb(GLFWwindow* window, int key, int scancode, int action, int mode)
	{
		UNREFERENCED_PARAMETER(mode);
		UNREFERENCED_PARAMETER(scancode);
		UNREFERENCED_PARAMETER(window);

		switch(action)
		{
			case GLFW_PRESS:
				//CONSOLE_LOG(LEVEL_DEBUG) << "Key: " << key << " Pressed";
				Input::OnKeyDown(static_cast<short>(key));
				break;
			case GLFW_RELEASE:
				Input::OnKeyUp(static_cast<short>(key));
				break;
		}
	}

	void fbsize_cb(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
		app->onWindowResized(width, height);
	}

	void cursor_enter_cb(GLFWwindow* window, int entered)
	{
#ifdef IMGUI_ENABLED
		ImGuiIO& io = ImGui::GetIO();
		if(entered) {
			// Only hide cursor if ImGui isn't using it
			if(!io.WantCaptureMouse) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			}
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
#else
		if(entered)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif

	}

	void mousebutton_cb(GLFWwindow* window, int button, int action, int mode)
	{
		UNREFERENCED_PARAMETER(window);
		UNREFERENCED_PARAMETER(mode);
		switch(action)
		{
			case GLFW_PRESS:
				//CONSOLE_LOG(LEVEL_DEBUG) << "MB: " << button << " Pressed";
				Input::OnKeyDown(static_cast<short>(button));
				break;
			case GLFW_RELEASE:
				Input::OnKeyUp(static_cast<short>(button));
				break;
		}
	}

	void mousepos_cb(GLFWwindow* window, double xpos, double ypos)
	{
		UNREFERENCED_PARAMETER(window);
#ifdef IMGUI_ENABLED
		UNREFERENCED_PARAMETER(xpos);
		UNREFERENCED_PARAMETER(ypos);
		auto pos = ImGui::GetMousePos();
		Input::OnMouseMove(pos.x, pos.y);
#else
		// Clamp mouse position to window bounds
		double clampedXpos = math::Clamp(xpos, 0.0, static_cast<double>(ST<Engine>::Get()->_windowExtent.width));
		double clampedYpos = math::Clamp(ypos, 0.0, static_cast<double>(ST<Engine>::Get()->_windowExtent.height));
		if (clampedXpos != xpos || clampedYpos != ypos)
			glfwSetCursorPos(window, clampedXpos, clampedYpos);

		Input::OnMouseMove(clampedXpos, clampedYpos);
#endif
	}

	void mousescroll_cb(GLFWwindow* window, double xoffset, double yoffset)
	{
		UNREFERENCED_PARAMETER(window);
		UNREFERENCED_PARAMETER(xoffset);
		Input::OnScroll(static_cast<float>(yoffset));

	}

	void joystick_cb([[maybe_unused]] int id, [[maybe_unused]] int event)
	{
	}

	void setup_event_callbacks(GLFWwindow* window)
	{
		glfwSetFramebufferSizeCallback(window, fbsize_cb);
		glfwSetKeyCallback(window, key_cb);
		glfwSetMouseButtonCallback(window, mousebutton_cb);
		glfwSetCursorPosCallback(window, mousepos_cb);
		glfwSetScrollCallback(window, mousescroll_cb);
		glfwSetJoystickCallback(joystick_cb);
		glfwSetWindowFocusCallback(window, Engine::OnFocusChanged);
		glfwSetCursorEnterCallback(window, cursor_enter_cb);
		glfwSetWindowIconifyCallback(window, [](GLFWwindow* window, int iconified)
		{
			Engine::OnFocusChanged(window, !iconified);
		});
	}

	void setWindowIcon(GLFWwindow* window) {
		GLFWimage images[1];
		int channels;
		images[0].pixels = stbi_load("Assets/Icon_game.png", &images[0].width, &images[0].height, &channels, 4);
		if(images[0].pixels) {
			glfwSetWindowIcon(window, 1, images);
			stbi_image_free(images[0].pixels);
		}
	}
}

void Engine::onWindowResized(int width, int height)
{
	_windowExtent.width = width;
	_windowExtent.height = height;
#ifdef IMGUI_ENABLED
#else
	_viewportExtent = _windowExtent;
#endif
	if(_vulkan)
		_vulkan->resized = true;
}
void Engine::onResolutionChanged(int width, int height)
{
	if(_window)
	{
		glfwSetWindowMonitor(_window, nullptr, 100, 100, width, height, 0);
		setWindowIcon(_window);
		onWindowResized(width, height);
	}
}
void Engine::OnFocusChanged([[maybe_unused]] GLFWwindow* window, int isFocused)
{
	CONSOLE_LOG(LEVEL_DEBUG) << "Focused changed: " << isFocused;

#ifdef IMGUI_ENABLED
	// TODO: Clean this call up
	HotReloader::FocusCallBackReload(window, isFocused);
#endif

	Messaging::BroadcastAll("OnWindowFocus", static_cast<bool>(isFocused));
}
void Engine::onFullscreen()
{
	const GLFWvidmode* mode = glfwGetVideoMode(_monitor);
	glfwSetWindowMonitor(_window, _monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	onWindowResized(mode->width, mode->height);
}

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
	glfwSetWindowShouldClose(_window, true);
}

bool Engine::IsShuttingDown() const
{
	// If _window isn't initialized, this means we're still initializing the program.
	return _window && glfwWindowShouldClose(_window);
}

void Engine::init()
{
	ST<GameSettings>::Get()->Load(); // Only load settings from file first so we have the correct filepaths.

	ST<Console>::Get()->SetupCrashHandler(); // DO NOT REMOVE THIS LINE EVER

	// Scripting Engine Initialisation
	CSharpScripts::CSScripting::Init();

	// FMOD Initialisation
	ST<AudioManager>::Get()->Initialise();

	constexpr unsigned int SCREEN_WIDTH = 1600;
	// The height of the screen
	constexpr unsigned int SCREEN_HEIGHT = 900;

	constexpr unsigned int VIEWPORT_WIDTH = 1920;

	constexpr unsigned int VIEWPORT_HEIGHT = 1080;
	// the width of the visible world
	constexpr unsigned int WORLD_WIDTH = 1920;
	// the height of the visible world
	constexpr unsigned int WORLD_HEIGHT = 1080;
	// TO BE READ IN DATA LATER

	_windowExtent = { SCREEN_WIDTH, SCREEN_HEIGHT };
	_viewportExtent = { VIEWPORT_WIDTH, VIEWPORT_HEIGHT };
	_worldExtent = { WORLD_WIDTH, WORLD_HEIGHT };

	if(!glfwInit()) {
		throw std::runtime_error("GLFW failed to initialise");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_ICONIFIED, GLFW_TRUE);
	uint32_t glfwExtCount = 0;
	glfwGetRequiredInstanceExtensions(&glfwExtCount);

	_window = glfwCreateWindow(_windowExtent.width, _windowExtent.height, Constant::name, nullptr, nullptr);
	auto windowCreate = std::chrono::high_resolution_clock::now();
	glfwSetDropCallback(_window, import::DropCallback); //To catch files 


	if(!_window) {
		glfwTerminate();
		throw std::runtime_error("Window failed to create.");
	}
	glfwSetWindowUserPointer(_window, this);


	CONSOLE_LOG(LEVEL_DEBUG) << "This is a Demo for Debugging";
	CONSOLE_LOG(LEVEL_INFO) << "This is a Demo for Information";
	CONSOLE_LOG(LEVEL_WARNING) << "This is a Demo for Warnings";
	CONSOLE_LOG(LEVEL_ERROR) << "This is a Demo for Errors";
	CONSOLE_LOG(LEVEL_FATAL) << "This is a Demo for Fatal issues";

	setup_event_callbacks(_window);

	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	_monitor = glfwGetPrimaryMonitor();

	ST<GameSettings>::Get()->Apply(); // Apply the loaded settings here

	setWindowIcon(_window);
	// Vulkan configuration
	// --------------------

	if(volkInitialize() != VK_SUCCESS)
	{
		throw std::runtime_error("failed to initialize Volk!");
	}

	_vulkan = std::make_unique<VulkanContext>();
	auto startTime = std::chrono::high_resolution_clock::now();
	_vulkan->init();
	auto currentTime = std::chrono::high_resolution_clock::now();
	CONSOLE_LOG(LEVEL_INFO) << "Vulkan context initialization: " << std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() << "ms";

	// NOTE THAT IMGUI IS SETUP IN VULKAN DUE TO THE CALLBACKS NEEDED FOR IT TO WORK

#ifdef _DEBUG
	CONSOLE_LOG(LEVEL_INFO) << "Current working directory: " << std::filesystem::canonical(ST<Filepaths>::Get()->workingDir);
	// identify file path for loading asset files
#endif

	// load resources
	ST<AssetBrowser>::Get()->file_system.Initialize(ST<Filepaths>::Get()->workingDir);
	ResourceManager::LoadAssetsFromFile(ST<Filepaths>::Get()->workingDir + "/Assets/assets.json");
	// Load fonts manually for now
	const std::array<std::string, 3> fontsToLoad{
		ST<Filepaths>::Get()->fontsSave + "/Arial.ttf",
		ST<Filepaths>::Get()->fontsSave + "/Lato-Regular.ttf",
		ST<Filepaths>::Get()->fontsSave + "/slkscre.ttf"
	};
	std::for_each(fontsToLoad.begin(), fontsToLoad.end(), ResourceManager::LoadFont);

	// initialize game
	// ---------------
	ecs::Initialize();
	ST<SceneManager>::Get(); // Initialize scene manager

	ST<EntitySpawnEvents>::Get(); // Initialize systems that listen for entity created events

#ifdef IMGUI_ENABLED
	ST<Game>::Get()->Init(WORLD_WIDTH, WORLD_HEIGHT, GAMESTATE::EDITOR);
	imgui_styling();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.Fonts->AddFontDefault();
	float baseFontSize = 15.0f; // 13.0f is the size of the default font. Change to the font size you use.
	float iconFontSize = baseFontSize * 2.5f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true; // Merge icon font to the previous font if you want to have both icons and text
	icons_config.PixelSnapH = true;
	icons_config.GlyphMinAdvanceX = iconFontSize;
	io.Fonts->AddFontFromFileTTF(fontsToLoad[1].c_str(), baseFontSize);
	io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, iconFontSize, &icons_config, icons_ranges);

	//If you want change between icons size you will need to create a new font
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 12.0f, &icons_config, icons_ranges);
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 20.0f, &icons_config, icons_ranges);

	io.Fonts->Build();
	ImGui_ImplVulkan_CreateFontsTexture();
#else
	ST<Game>::Get()->Init(WORLD_WIDTH, WORLD_HEIGHT, GAMESTATE::IN_GAME);
#endif
	auto timeafterwindow = std::chrono::high_resolution_clock::now();
	CONSOLE_LOG(LEVEL_INFO) << "Initialization: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeafterwindow - windowCreate).count() << "ms";
}
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
		serializer.Serialize("show_editor", ST<Editor>::Get()->GetIsOpen());
		serializer.Serialize("show_settings", ST<SettingsWindow>::Get()->GetIsOpen());
		serializer.Serialize("show_browser", show_browser);
		serializer.Serialize("show_hierarchy", ST<Hierarchy>::Get()->isOpen);
	}
	void loadState(const char* filename) {
		Deserializer deserializer{ filename };
		if(!deserializer.IsValid())
			return;

		bool b{};
		deserializer.DeserializeVar("show_demo_window", &show_demo_window);
		deserializer.DeserializeVar("show_console", &b), ST<Console>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_performance", &b), ST<PerformanceProfiler>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_editor", &b), ST<Editor>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_settings", &b), ST<SettingsWindow>::Get()->SetIsOpen(b);
		deserializer.DeserializeVar("show_browser", &show_browser);
		deserializer.DeserializeVar("show_hierarchy", &ST<Hierarchy>::Get()->isOpen);
	}
}
#endif

void Engine::run() {
	glfwRestoreWindow(_window);
#ifdef IMGUI_ENABLED
	ImGuiIO& io = ImGui::GetIO();
	loadState("imgui.json");
#endif
	bool bQuit = false;
	while(!bQuit)
	{
		wait();

#ifdef IMGUI_ENABLED
		GameTime::SetFps(io.Framerate);
#else
		GameTime::SetFps(ST<PerformanceProfiler>::Get()->GetFPS());
#endif
		ST<PerformanceProfiler>::Get()->StartFrame();
		GameTime::NewFrame(ST<PerformanceProfiler>::Get()->GetDeltaTime());

		// Only reset key states when systems are updating so we don't skip inputs.
		if(GameTime::RealNumFixedFrames())
		{
			Input::NewFrame();
			glfwPollEvents();
			GamepadInput::PollInput();
		}

		if(glfwWindowShouldClose(_window)) {
			bQuit = true;
		}

		// Start the Dear ImGui frame
		_vulkan->beginFrame();

		// Enable docking
#ifdef IMGUI_ENABLED
		if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("##DockSpace ", nullptr, window_flags);
			ImGui::PopStyleVar(3);

			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}

		if(show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

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
		if(ST<Editor>::Get()->GetIsOpen())
		{
			ST<Editor>::Get()->Draw();
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
		ST<SettingsWindow>::Get()->Draw();
		ST<LayersMatrix>::Get()->Draw();
		ST<Editor>::Get()->RenderGrid();
		ecs::FlushChanges(); // For if any of the above systems deleted an entity.

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
					ST<SettingsWindow>::Get()->SetIsOpen(true);
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
					ST<Editor>::Get()->SetIsOpen(true);
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

		static bool startup = true;
		ST<CustomViewport>::Get()->DrawImGuiWindow();
#endif


		// manage user input
		// -----------------
		ST<PerformanceProfiler>::Get()->StartProfile("Process Input");
		if(GameTime::RealNumFixedFrames())
		{
#ifdef IMGUI_ENABLED
			ST<Editor>::Get()->ProcessInput();
			if(Input::GetKeyPressed(KEY::GRAVE))
				ST<Console>::Get()->SetIsOpen(!ST<Console>::Get()->GetIsOpen());
			if(Input::GetKeyPressed(KEY::F1))
				show_demo_window = true;
#endif

			if(Input::GetKeyPressed(KEY::F11))
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
		FunctionQueue::ExecuteQueuedOperations();
		ST<Scheduler>::Get()->Update(GameTime::FixedDt() * static_cast<float>(GameTime::NumFixedFrames()));

		// render
		// ------
#ifdef IMGUI_ENABLED
		if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGui::End();
		}
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
#else

		bool main_is_minimized = false;
		// Handle window events
		if(glfwGetWindowAttrib(_window, GLFW_ICONIFIED)) {
			main_is_minimized = true;
		}
#endif

		// Game window draw
		if(!main_is_minimized)
		{
			ST<PerformanceProfiler>::Get()->StartProfile("Render");
			ST<Game>::Get()->Render();

#ifdef IMGUI_ENABLED
			ST<Editor>::Get()->DrawSelectedEntityBorder();
#endif

			_vulkan->_renderer->drawFrame();
			ST<PerformanceProfiler>::Get()->EndProfile("Render");
		}

		// Update and Render additional Platform Windows
#ifdef IMGUI_ENABLED
		if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
#endif

		//
		// Present Main Platform Window
		if(!main_is_minimized)
		{
			_vulkan->endFrame();
		}
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
	ST<Editor>::Destroy();
#endif
	ST<HiddenComponentsStore>::Destroy();
	ST<RegisteredComponents>::Destroy();
	ST<PrefabManager>::Destroy();
	ST<PrefabWindow>::Destroy();
#ifdef IMGUI_ENABLED
	ST<Hierarchy>::Destroy();
#endif
	CSharpScripts::CSScripting::Exit();

	ecs::Shutdown();

	ST<GameSettings>::Destroy();
	//ST<Filepaths>::Destroy(); // Filepaths kinda needs to live for other threads to reference filepaths... smart pointers will free this later. sry about this
	ST<ecs::RegisteredSystemsOperatingByLayer>::Destroy();

	// In case any systems send logs to the console while destructing.
	ST<Console>::Destroy();

	_vulkan->shutdown();
	glfwDestroyWindow(_window);
	glfwTerminate();
}
#ifdef IMGUI_ENABLED
void Engine::imgui_styling()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.300000011920929f;
	style.WindowPadding = ImVec2(10.10000038146973f, 10.10000038146973f);
	style.WindowRounding = 10.30000019073486f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(20.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.ChildRounding = 8.199999809265137f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 10.69999980926514f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(20.0f, 1.5f);
	style.FrameRounding = 4.800000190734863f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(9.699999809265137f, 5.300000190734863f);
	style.ItemInnerSpacing = ImVec2(5.400000095367432f, 9.300000190734863f);
	style.CellPadding = ImVec2(7.900000095367432f, 2.0f);
	style.IndentSpacing = 10.69999980926514f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 12.10000038146973f;
	style.ScrollbarRounding = 20.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 4.599999904632568f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
}
#endif