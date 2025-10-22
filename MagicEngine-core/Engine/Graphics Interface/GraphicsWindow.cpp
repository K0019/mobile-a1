/******************************************************************************/
/*!
\file   GraphicsWindow.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Abstracts the interface of interacting with the program window.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Graphics Interface/GraphicsWindow.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Engine/Input.h"
#include "GameSettings.h"
#include "Editor/Containers/GUICollection.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

IntVec2 IGraphicsWindow::GetWindowExtent() const
{
	return windowExtent;
}
IntVec2 IGraphicsWindow::GetViewportExtent() const
{
	return viewportExtent;
}
IntVec2 IGraphicsWindow::GetWorldExtent() const
{
	return worldExtent;
}

IGraphicsWindow::IGraphicsWindow()
// TODO: Serialize these
	: windowExtent{ 1600, 900 }
	, viewportExtent{ 1920, 1080 }
	, worldExtent{ 1920, 1080 }
{
}

void GraphicsWindowGeneric::Init()
{
}

void GraphicsWindowGeneric::SetPendingShutdown()
{
}

bool GraphicsWindowGeneric::GetIsPendingShutdown() const
{
	return false;
}

bool GraphicsWindowGeneric::GetIsWindowMinimized() const
{
	return false;
}

bool GraphicsWindowGeneric::SetWindowIcon(const std::string& filepath)
{
	return false;
}

void GraphicsWindowGeneric::SetWindowResolution(int width, int height)
{
}

void GraphicsWindowGeneric::SetFullscreen([[maybe_unused]] bool isFullscreen)
{
#ifdef GLFW
	Core::Display().setFullscreen(isFullscreen);
#endif
}

void GraphicsWindowGeneric::BringWindowToFront()
{
}

//GraphicsWindowGLFW::~GraphicsWindowGLFW()
//{
//	if (!window)
//		return;
//
//	glfwDestroyWindow(window);
//	glfwTerminate();
//}
//
//void GraphicsWindowGLFW::Init()
//{
//	InitGLFW();
//
//	window = glfwCreateWindow(windowExtent.x, windowExtent.y, "Mahou Engine", nullptr, nullptr); // MAGICCCCCCCCCCCCC
//	if (!window)
//	{
//		glfwTerminate();
//		throw std::runtime_error("Window failed to create.");
//	}
//	SetupGLFWSettings();
//	SetupGLFWCallbacks();
//	monitor = glfwGetPrimaryMonitor();
//}
//
//void GraphicsWindowGLFW::SetPendingShutdown()
//{
//	assert(window);
//	glfwSetWindowShouldClose(window, true);
//}
//
//bool GraphicsWindowGLFW::GetIsPendingShutdown() const
//{
//	// If _window isn't initialized, this means we're still initializing the program.
//	return window && glfwWindowShouldClose(window);
//}
//
//bool GraphicsWindowGLFW::GetIsWindowMinimized() const
//{
//	return glfwGetWindowAttrib(window, GLFW_ICONIFIED) ? true : false;
//}
//
//bool GraphicsWindowGLFW::SetWindowIcon(const std::string& filepath)
//{
//	GLFWimage images[1];
//	int channels;
//	images[0].pixels = stbi_load(filepath.c_str(), &images[0].width, &images[0].height, &channels, 4);
//	if (!images[0].pixels)
//	{
//		CONSOLE_LOG(LEVEL_ERROR) << "Failed to load window icon file: " << filepath;
//		return false;
//	}
//
//	glfwSetWindowIcon(window, 1, images);
//	stbi_image_free(images[0].pixels);
//	return true;
//}
//
//void GraphicsWindowGLFW::SetWindowResolution(int width, int height)
//{
//	assert(window);
//
//	glfwSetWindowMonitor(window, nullptr, 100, 100, width, height, 0);
//	SetWindowIcon(ST<Filepaths>::Get()->graphicsWindowIcon);
//	Callback_FramebufferResize(width, height);
//}
//
//void GraphicsWindowGLFW::SetFullscreen(bool isFullscreen)
//{
//	if (isFullscreen)
//	{
//		const GLFWvidmode* mode{ glfwGetVideoMode(monitor) };
//		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
//		Callback_FramebufferResize(mode->width, mode->height);
//	}
//	else
//		SetWindowResolution(windowExtent.x, windowExtent.y);
//}
//
//void GraphicsWindowGLFW::BringWindowToFront()
//{
//	glfwRestoreWindow(window);
//}
//
//void GraphicsWindowGLFW::InitGLFW()
//{
//	if (!glfwInit()) {
//		throw std::runtime_error("GLFW failed to initialise");
//	}
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
//	glfwWindowHint(GLFW_ICONIFIED, GLFW_TRUE);
//	//uint32_t glfwExtCount = 0;
//	//glfwGetRequiredInstanceExtensions(&glfwExtCount);
//}
//
//void GraphicsWindowGLFW::SetupGLFWSettings()
//{
//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//	SetWindowIcon(ST<Filepaths>::Get()->graphicsWindowIcon);
//}
//
//void GraphicsWindowGLFW::SetupGLFWCallbacks()
//{
//	glfwSetFramebufferSizeCallback(window, GLFW_Callback_FramebufferResize);
//	glfwSetKeyCallback(window, KeyboardMouseInput::GLFW_Callback_OnKeyboardClick);
//	glfwSetMouseButtonCallback(window, KeyboardMouseInput::GLFW_Callback_OnMouseClick);
//	glfwSetCursorPosCallback(window, KeyboardMouseInput::GLFW_Callback_OnMouseMove);
//	glfwSetScrollCallback(window, KeyboardMouseInput::GLFW_Callback_OnMouseScroll);
//	//glfwSetJoystickCallback(joystick_cb);
//	glfwSetWindowFocusCallback(window, GLFW_Callback_WindowFocusChanged);
//	glfwSetCursorEnterCallback(window, GLFW_Callback_CursorEnteredWindow);
//	glfwSetWindowIconifyCallback(window, GLFW_Callback_IconifyStateChanged);
//}
//
//void GraphicsWindowGLFW::GLFW_Callback_FramebufferResize(GLFWwindow*, int width, int height)
//{
//	ST<GraphicsWindow>::Get()->Callback_FramebufferResize(width, height);
//}
//void GraphicsWindowGLFW::Callback_FramebufferResize(int width, int height)
//{
//	windowExtent.x = width;
//	windowExtent.y = height;
//#ifndef IMGUI_ENABLED
//	viewportExtent = windowExtent;
//#endif
//
//	ST<GraphicsMain>::Get()->INTERNAL_OnWindowResized(width, height);
//}
//
//void GraphicsWindowGLFW::GLFW_Callback_WindowFocusChanged(GLFWwindow*, int isFocused)
//{
//	Messaging::BroadcastAll("OnWindowFocus", static_cast<bool>(isFocused));
//}
//
//void GraphicsWindowGLFW::GLFW_Callback_IconifyStateChanged(GLFWwindow* window, int isIconified)
//{
//	GLFW_Callback_WindowFocusChanged(window, isIconified == 0);
//}
//
//void GraphicsWindowGLFW::GLFW_Callback_CursorEnteredWindow(GLFWwindow* window, int isEntered)
//{
//#ifdef IMGUI_ENABLED
//	ImGuiIO& io = ImGui::GetIO();
//	if (isEntered)
//	{
//		// Only hide cursor if ImGui isn't using it
//		if (!io.WantCaptureMouse)
//			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
//	}
//	else
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//#else
//	if (isEntered)
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//	else
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//#endif
//}
//
//GLFWwindow* GraphicsWindowGLFW::INTERNAL_GetWindow()
//{
//	return window;
//}
