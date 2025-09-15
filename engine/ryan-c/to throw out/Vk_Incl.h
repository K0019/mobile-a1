#pragma once
/******************************************************************************/
/*!
\file   Vk_Incl.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Small file to ensure that Vulkan headers are included correctly.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/


#include "windows.h"

#ifdef IMGUI_ENABLED
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#endif

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>
#ifdef IMGUI_ENABLED
#else
#include "volk.h"
#endif

#include "vma_impl.h"
