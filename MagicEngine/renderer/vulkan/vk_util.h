// renderer/vulkan/vk_util.h
// Minimal header for Vulkan includes compatibility
// The old vulkan backend was removed, this provides just the includes needed by other code
#pragma once

// Ensure Windows types are available for Vulkan headers
#if defined(_WIN32) || defined(WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Include windef.h directly for basic types, then Windows.h
#include <windef.h>
#include <winbase.h>
#include <Windows.h>
// Tell Vulkan we're on Win32
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

#include <vulkan/vulkan.h>
