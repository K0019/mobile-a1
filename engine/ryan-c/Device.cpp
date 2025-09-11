/******************************************************************************/
/*!
\file   Device.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Device.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Device.h"

#include "Instance.h"
#include "Surface.h"
#include "VulkanHelper.h"


Device::~Device()
{
    vkDestroyDevice(m_device, pAllocator);
}

std::optional<uint32_t> Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_chosenGPU, &memProperties);
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return std::nullopt;
}

VkFormat Device::findDepthFormat() const
{
    const std::vector candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_chosenGPU, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported depth format");
}

VkDevice Device::handle() const { return m_device;}
VkPhysicalDevice Device::chosenGPU() const { return m_chosenGPU;}
VkQueue Device::graphicsQueue() const { return m_graphicsQueue;}
uint32_t Device::graphicsQueueFamily() const { return m_graphicsQueueFamily;}

Device::Device()
{
    auto m_instance = VulkanManager::Get().VkInstance().handle();
    auto m_surface = VulkanManager::Get().VkSurface().handle();
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    if(deviceCount == 0) {
        MessageBoxA(nullptr, 
            "No GPUs with Vulkan support were found. Please update your graphics drivers.", 
            "Graphics Driver Update Recommended", 
            MB_OK | MB_ICONWARNING);
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    m_chosenGPU = VK_NULL_HANDLE;
    
    for(const auto& device : devices) {
        if(VulkanHelper::isDeviceSuitable(device)) {
            m_chosenGPU = device;
            break;
        }
    }
    
    if(m_chosenGPU == VK_NULL_HANDLE) {
        // Get the first device's properties for more detailed error message
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(devices[0], &deviceProps);
        
        std::string message = "No suitable GPU found. Your graphics card (" + 
            std::string(deviceProps.deviceName) + 
            ") may need updated drivers to support required Vulkan features.";
            
        MessageBoxA(nullptr, 
            message.c_str(),
            "Graphics Driver Update Recommended", 
            MB_OK | MB_ICONWARNING);
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    auto queueFamily = VulkanHelper::findQueueFamily(m_chosenGPU, m_surface);
    if(!queueFamily.has_value()) {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(m_chosenGPU, &deviceProps);
        
        std::string message = "Graphics card (" + 
            std::string(deviceProps.deviceName) + 
            ") doesn't support required queue operations. Please update your graphics drivers.";
            
        MessageBoxA(nullptr, 
            message.c_str(),
            "Graphics Driver Update Recommended", 
            MB_OK | MB_ICONWARNING);
        throw std::runtime_error("failed to find a suitable queue family!");
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily.value();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features12.timelineSemaphore = VK_TRUE;
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features.pipelineStatisticsQuery = VK_TRUE;
    deviceFeatures2.features.wideLines = VK_TRUE;
    deviceFeatures2.pNext = &features12;

    std::vector<const char*> deviceExtensions = Constant::deviceExtensions;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &deviceFeatures2;
    createInfo.pEnabledFeatures = nullptr;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if(Constant::enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(Constant::validationLayers.size());
        createInfo.ppEnabledLayerNames = Constant::validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(m_chosenGPU, &createInfo, pAllocator, &m_device) != VK_SUCCESS) {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(m_chosenGPU, &deviceProps);
        
        std::string message = "Failed to initialize " + 
            std::string(deviceProps.deviceName) + 
            ". Please ensure you have the latest graphics drivers installed.";
            
        MessageBoxA(nullptr, 
            message.c_str(),
            "Graphics Driver Update Recommended", 
            MB_OK | MB_ICONWARNING);
        throw std::runtime_error("Failed to create logical device");
    }

    volkLoadDevice(m_device);
    vkGetDeviceQueue(m_device, queueFamily.value(), 0, &m_graphicsQueue);
    m_graphicsQueueFamily = queueFamily.value();
    vkGetPhysicalDeviceProperties(m_chosenGPU, &m_gpuProperties);
}
