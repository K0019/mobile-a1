/******************************************************************************/
/*!
\file   Swapchain.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
VkSwapchain.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Swapchain.h"

#include "Device.h"
#include "Engine.h"
#include "Surface.h"
#include "VkInit.h"

Swapchain::Swapchain()
{
    m_swapchain = VK_NULL_HANDLE;
    m_depthResources.view = VK_NULL_HANDLE;
    m_depthResources.image._image = VK_NULL_HANDLE;
    m_depthResources.image._allocation = VK_NULL_HANDLE;
    
    create();
}

Swapchain::~Swapchain() {
    destroy();
}


VkSwapchainKHR Swapchain::handle() const { return m_swapchain; }

VkFormat Swapchain::imageFormat() const { return m_swapchainImageFormat; }

VkExtent2D Swapchain::extent() const { return m_swapchainExtent; }

const std::vector<VkImage>& Swapchain::images() const { return m_swapchainImages; }

const std::vector<VkImageView>& Swapchain::imageViews() const { return m_swapchainImageViews; }

const Swapchain::DepthResources& Swapchain::depthResources() const { return m_depthResources;}

void Swapchain::create(VkSwapchainKHR old_swapchain) {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    auto& m_device = VulkanManager::Get().VkDevice();
    auto m_surface = VulkanManager::Get().VkSurface().handle();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.chosenGPU(), m_surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.chosenGPU(),m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.chosenGPU(), m_surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.chosenGPU(),m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.chosenGPU(), m_surface, &presentModeCount, presentModes.data());
    }

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
    VkExtent2D extent = chooseSwapExtent(capabilities);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = VulkanManager::Get().VkSurface().handle();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = old_swapchain;

    if (vkCreateSwapchainKHR(m_device.handle(), &createInfo, pAllocator, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_device.handle(), m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device.handle(), m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;

    createImageViews();

    m_depthResources.format = VulkanManager::Get().VkDevice().findDepthFormat(); 

    extent = ST<Engine>::Get()->_windowExtent;

    auto depth_extent = ST<Engine>::Get()->_viewportExtent;

    VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(
        m_depthResources.format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VkExtent3D{depth_extent.width, depth_extent.height, 1}
    );

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (m_depthResources.image._image != VK_NULL_HANDLE) {
        vmaDestroyImage(VulkanManager::Get().VkAllocator(), m_depthResources.image._image, m_depthResources.image._allocation);
        m_depthResources.image._image = VK_NULL_HANDLE;  // Add this line
        m_depthResources.image._allocation = VK_NULL_HANDLE;  // Add this line
    }

    if (m_depthResources.view != VK_NULL_HANDLE) {
        vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), m_depthResources.view, pAllocator);
        m_depthResources.view = VK_NULL_HANDLE;  // Add this line
    }

    if (vmaCreateImage(VulkanManager::Get().VkAllocator(), &imageInfo, &allocInfo,
                       &m_depthResources.image._image,
                       &m_depthResources.image._allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create depth image!");
    }

    VkImageViewCreateInfo viewInfo = VkInit::ImageViewCreateInfo(
        m_depthResources.format,
        m_depthResources.image._image,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    if (vkCreateImageView(VulkanManager::Get().VkDevice().handle(),
                          &viewInfo, nullptr, &m_depthResources.view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create depth image view!");
    }
}

void Swapchain::destroy() {
    if (m_depthResources.view != VK_NULL_HANDLE) {
        vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), m_depthResources.view, pAllocator);
        m_depthResources.view = VK_NULL_HANDLE;
    }

    if (m_depthResources.image._image != VK_NULL_HANDLE) {
        vmaDestroyImage(VulkanManager::Get().VkAllocator(), m_depthResources.image._image, m_depthResources.image._allocation);
        m_depthResources.image._image = VK_NULL_HANDLE;
        m_depthResources.image._allocation = VK_NULL_HANDLE;
    }

    for (auto imageView : m_swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), imageView, pAllocator);
        }
    }
    m_swapchainImageViews.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(VulkanManager::Get().VkDevice().handle(), m_swapchain, pAllocator);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void Swapchain::recreate() {
    // Ensure all GPU operations are complete
    vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());

    VkSwapchainKHR old_swapchain = m_swapchain;
    std::vector<VkImageView> old_image_views = std::move(m_swapchainImageViews);

    // Save old depth resources to destroy later
    VkImageView old_depth_view = m_depthResources.view;
    VkImage old_depth_image = m_depthResources.image._image;
    VmaAllocation old_depth_allocation = m_depthResources.image._allocation;

    // Reset to NULL handles to avoid double-free
    m_depthResources.view = VK_NULL_HANDLE;
    m_depthResources.image._image = VK_NULL_HANDLE;
    m_depthResources.image._allocation = VK_NULL_HANDLE;

    // Create new swapchain
    create(old_swapchain);

    // Now initialise the new images BEFORE destroying old resources
    initialiseImages();

    // Wait for all initialization commands to complete
    vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());

    // Clean up old swapchain resources
    for (auto imageView : old_image_views) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), imageView, pAllocator);
        }
    }

    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(VulkanManager::Get().VkDevice().handle(), old_swapchain, pAllocator);
    }

    // Clean up old depth resources after initializing new ones
    if (old_depth_view != VK_NULL_HANDLE) {
        vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), old_depth_view, pAllocator);
    }

    if (old_depth_image != VK_NULL_HANDLE) {
        vmaDestroyImage(VulkanManager::Get().VkAllocator(), old_depth_image, old_depth_allocation);
    }
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == Constant::presentMode) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(ST<Engine>::Get()->_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Swapchain::createImageViews() {
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo = VkInit::ImageViewCreateInfo(m_swapchainImageFormat, m_swapchainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);

        if (vkCreateImageView(VulkanManager::Get().VkDevice().handle(), &viewInfo, pAllocator, &m_swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void Swapchain::initialiseImages()
{
    ST<Engine>::Get()->_vulkan->immediateSubmit([&](VkCommandBuffer cmd)
    {
        for (auto& image : m_swapchainImages) {
            VulkanHelper::TransitionImage(
                cmd,
                image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
        }

        VulkanHelper::TransitionImage(
            cmd,
            m_depthResources.image._image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );
    });
}
