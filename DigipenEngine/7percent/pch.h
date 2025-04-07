/******************************************************************************/
/*!
\file   pch.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   02/05/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is a pre-compiled header file to speed up compilation.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

#define NOMINMAX
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <iterator>
#include <ranges>
#include <algorithm>
#include <functional>
#include <bitset>
#include <any>
#include <optional>
#include <random>
#include <utility>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <typeinfo>
#include <exception>
#include <stdexcept>
#include <iterator>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <type_traits>
#include <limits>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <cassert>
#include <cctype>
#include <math.h>

/* LIBRARIES */
#include <windows.h>
#include <dbghelp.h>

#ifdef IMGUI_ENABLED
	#define IMGUI_DEFINE_MATH_OPERATORS
	#include <imgui.h>
	#include "imgui_internal.h"
	#include "imgui_impl_glfw.h"
	#include "imgui_impl_vulkan.h"
#endif
#include "IconsFontAwesome6.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>
#define GLM_ZERO_TO_ONE_RANGE
#include <vulkan/glm/glm.hpp>
#include <vulkan/glm/gtc/matrix_transform.hpp>

#ifndef IMGUI_ENABLED
	#include "volk.h"
#endif

#pragma warning(push, 0)
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"
#pragma warning(pop)

#include <FMOD/fmod.hpp>

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
/* END LIBRARIES */

#include "Singleton.h"
#include "percentmath.h"
#include "Transform.h"
#include "ECS.h"
#include "IRegisteredComponent.h"
#include "IEditorComponent.h"
#include "IHiddenComponent.h"
#include "Messaging.h"
#include "Scheduler.h"
#include "EntityEvents.h"
#include "Input.h"
#include "GameTime.h"
#include "Serializer.h"
#include "TypeID.h"
#include "Utilities.h"
#include "MaskTemplate.h"
#include "MacroTemplates.h"
#include "RandUID.h"

#include "GUICollection.h"
#include "Console.h"

#include "StateMachine.h"
