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
#if PLATFORM_DESKTOP
	Core::Lifecycle().RequestExit();
#endif
}

bool GraphicsWindowGeneric::GetIsPendingShutdown() const
{
#if PLATFORM_DESKTOP
	return Core::Lifecycle().ShouldQuit();
#else
	return false;
#endif
}

bool GraphicsWindowGeneric::GetIsWindowMinimized() const
{
	return false;
}

bool GraphicsWindowGeneric::SetWindowIcon([[maybe_unused]] const std::string& filepath)
{
	return false;
}

void GraphicsWindowGeneric::SetWindowResolution([[maybe_unused]] int width, [[maybe_unused]] int height)
{
}

void GraphicsWindowGeneric::SetFullscreen([[maybe_unused]] bool isFullscreen)
{
#if PLATFORM_DESKTOP
	Core::Display().setFullscreen(isFullscreen);
#endif
}

void GraphicsWindowGeneric::BringWindowToFront()
{
}
