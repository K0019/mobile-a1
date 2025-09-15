/******************************************************************************/
/*!
\file   Instance.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
VkInstance.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Instance.h"
#include "VulkanHelper.h"

Instance::~Instance()
{
    if(m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, pAllocator);
    }
}

VkInstance Instance::handle() const { return m_instance; }

Instance::Instance()
{  
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = Constant::name;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions

    //quick hack to minimise load times.
    std::vector<const char*> extensions = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };
    if (Constant::enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation layer setup (if enabled)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    VkValidationFeaturesEXT validationFeatures{};
    if (Constant::enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(Constant::validationLayers.size());
        createInfo.ppEnabledLayerNames = Constant::validationLayers.data();

        VulkanHelper::DebugMessengerInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, pAllocator, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    volkLoadInstanceOnly(m_instance);
}