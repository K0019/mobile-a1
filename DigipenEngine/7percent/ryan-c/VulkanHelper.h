#pragma once
/******************************************************************************/
/*!
\file   VulkanHelper.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Includes helper functions for Vulkan

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"

namespace Constant // for things that will never be changed.
{
  constexpr unsigned int FRAME_OVERLAP = 2;
  const std::vector validationLayers = { "VK_LAYER_KHRONOS_validation" };
  const std::vector enabledValidationFeatures = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };
  constexpr bool enableValidationFeatures = false;
  constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  const std::vector deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#ifdef IMGUI_ENABLED
  constexpr char name[] = "7Percent Engine";
#else
  constexpr char name[] = "The Last Defender";
#endif
  constexpr uint32_t MAX_TEXTURES = 1024;
#ifdef _NDEBUG
  constexpr bool enableValidationLayers = false;
#else
  constexpr bool enableValidationLayers = true;
#endif
} // namespace Constant

inline VkAllocationCallbacks* pAllocator = nullptr;

class VulkanHelper
{
  public:
  static bool isDeviceSuitable(VkPhysicalDevice_T* device);
  static bool checkValidationLayerSupport();
  static std::vector<const char*> getRequiredExtensions();
  static void DebugMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
  static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pDebugMessenger);
  static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
  static std::optional<uint32_t> findQueueFamily(VkPhysicalDevice _chosenGPU, VkSurfaceKHR _surface);
  static void TransitionImage(VkCommandBuffer cmd, VkImage image,
                              VkImageLayout currentLayout,
                              VkImageLayout newLayout,
                              VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                              VkPipelineStageFlags2 srcStageMask = 0,
                              VkPipelineStageFlags2 dstStageMask = 0);
  static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
};
