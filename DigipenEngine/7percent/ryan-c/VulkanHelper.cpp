/******************************************************************************/
/*!
\file   VulkanHelper.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Vulkan helper functions.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VulkanHelper.h"

bool VulkanHelper::isDeviceSuitable(VkPhysicalDevice_T* device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if(deviceProperties.apiVersion < VK_API_VERSION_1_3) {
        std::cerr << "Device does not support Vulkan 1.3" << '\n';
        return false;
    }

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

    bool issuitable = true;

    if(!issuitable) {
        std::cerr << "Device is not suitable" << '\n';
    }

    return issuitable;
}

bool VulkanHelper::checkValidationLayerSupport(){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layerName : Constant::validationLayers) {
        bool layerFound = std::any_of(availableLayers.begin(), availableLayers.end(),
            [&layerName](const VkLayerProperties& layerProperties) {
                return std::string_view(layerName) == layerProperties.layerName;
            });

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanHelper::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(Constant::enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

void VulkanHelper::DebugMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

VkResult VulkanHelper::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    if(const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanHelper::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
    if(const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")); func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkBool32 VulkanHelper::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                     VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                     void* pUserData)
{
    UNREFERENCED_PARAMETER(pUserData); // for logging to file in future

    std::cerr << "|====================================BEGIN MESSAGE=========================================|\n";
    // Convert message severity to string
    const char* severityStr = "UNKNOWN";
    switch(messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severityStr = "VERBOSE"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    severityStr = "INFO";    break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severityStr = "WARNING"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   severityStr = "ERROR";   break;
        default:;
    }

    // Convert message type to string
    std::string typeStr;
    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)     typeStr += "GENERAL";
    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)  typeStr += "VALIDATION";
    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) typeStr += "PERFORMANCE";

    if(!pCallbackData)
    {
        std::cerr << "No message data\n";
        return false;
    }

    if(pCallbackData->pMessageIdName == nullptr)
    {
        std::cerr << "No message data\n";
        return false;
    }
    // Format and output the debug message
    std::cerr << "[VULKAN][" << severityStr << "][" << typeStr << "]\n"
        << "Message ID Name: " << pCallbackData->pMessageIdName << "\n"
        << "Message ID Number: " << pCallbackData->messageIdNumber << "\n"
        << "Message: " << pCallbackData->pMessage << "\n";

    // Output any labeled objects
    if(pCallbackData->objectCount > 0) {
        std::cerr << "Objects:\n";
        for(uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
            std::cerr << "\tObject " << i << "\n"
                << "\t\tType: " << pCallbackData->pObjects[i].objectType << "\n"
                << "\t\tHandle: " << pCallbackData->pObjects[i].objectHandle << "\n"
                << "\t\tName: " << (pCallbackData->pObjects[i].pObjectName ? pCallbackData->pObjects[i].pObjectName : "unnamed") << "\n";
        }
    }

    std::cerr << "|=====================================END MESSAGE==========================================|" << '\n';  // Ensure output is flushed
    // The return value of VK_FALSE indicates that the Vulkan call should not be aborted
    return VK_FALSE;
}

std::optional<uint32_t> VulkanHelper::findQueueFamily(VkPhysicalDevice _chosenGPU, VkSurfaceKHR _surface)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_chosenGPU, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_chosenGPU, &queueFamilyCount, queueFamilies.data());

    for(uint32_t i = 0; i < queueFamilyCount; i++) {
        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) // VK_QUEUE_COMPUTE_BIT IS SUPPORTED IMPLICITLY
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_chosenGPU, i, _surface, &presentSupport);
            if(presentSupport) {
                return i;
            }
        }
    }
    return std::nullopt;
}

void VulkanHelper::TransitionImage(VkCommandBuffer cmd, VkImage image, 
                                  VkImageLayout currentLayout,
                                  VkImageLayout newLayout, 
                                  VkImageAspectFlags aspectMask,
                                  VkPipelineStageFlags2 srcStageMask,
                                  VkPipelineStageFlags2 dstStageMask) {
    if (currentLayout == newLayout) {
        return;
    }

    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.image = image;
    
    // Base configuration - use default stages if none provided
    VkPipelineStageFlags2 defaultSrcStages = 0;
    VkPipelineStageFlags2 defaultDstStages = 0;
    VkAccessFlags2 srcAccessMask = 0;
    VkAccessFlags2 dstAccessMask = 0;

    // ========== SOURCE ACCESS CONFIGURATION ==========
    switch(currentLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            defaultSrcStages = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Include ALL shader stages for maximum safety
            defaultSrcStages = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | 
                               VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            defaultSrcStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | 
                            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            defaultSrcStages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | 
                               VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | 
                            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            defaultSrcStages = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            defaultSrcStages = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_GENERAL:
            // General layout could be used for many purposes - be conservative
            defaultSrcStages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT |
                           VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            defaultSrcStages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            srcAccessMask = 0;
            break;

        default:
            // Conservative default for safety
            defaultSrcStages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
    }

    // ========== DESTINATION ACCESS CONFIGURATION ==========
    switch(newLayout) {
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Wait for ALL possible shader stages
            defaultDstStages = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | 
                               VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            defaultDstStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | 
                            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            defaultDstStages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | 
                               VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | 
                            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            defaultDstStages = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            defaultDstStages = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_GENERAL:
            defaultDstStages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT |
                           VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            defaultDstStages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            dstAccessMask = 0;
            break;

        default:
            defaultDstStages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
    }

    // Use provided stage masks or defaults
    imageBarrier.srcStageMask = srcStageMask ? srcStageMask : defaultSrcStages;
    imageBarrier.dstStageMask = dstStageMask ? dstStageMask : defaultDstStages;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstAccessMask = dstAccessMask;

    // Configure subresource range
    imageBarrier.subresourceRange.aspectMask = aspectMask;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    // Add a full memory dependency
    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;
    depInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void VulkanHelper::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    
    // Use the minimum of source and destination sizes to avoid overrun
    uint32_t effectiveWidth = (std::min)(srcSize.width, dstSize.width);
    uint32_t effectiveHeight = (std::min)(srcSize.height, dstSize.height);
    
    // Source coordinates
    blitRegion.srcOffsets[0] = {0, 0, 0};
    blitRegion.srcOffsets[1] = {
        static_cast<int32_t>(effectiveWidth),
        static_cast<int32_t>(effectiveHeight),
        1
    };
    
    // If you want to center the viewport in a larger window:
    int32_t xOffset = (dstSize.width - effectiveWidth) / 2;
    int32_t yOffset = (dstSize.height - effectiveHeight) / 2;
    
    blitRegion.dstOffsets[0] = {xOffset, yOffset, 0};
    blitRegion.dstOffsets[1] = {
        static_cast<int32_t>(xOffset + effectiveWidth),
        static_cast<int32_t>(yOffset + effectiveHeight),
        1
    };
    
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.dstImage = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_NEAREST;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}
    

