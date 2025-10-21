#pragma once

#ifdef IMGUI_ENABLED
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include "imgui_internal.h"

#ifdef GLFW
#include "imgui_impl_glfw.h"
#endif

#endif
#include "IconsFontAwesome6.h"
