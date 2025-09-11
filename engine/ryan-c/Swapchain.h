#pragma once
/******************************************************************************/
/*!
\file   Swapchain.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Wrapper class for VkSwapchain.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Vk_Incl.h"

#include "TextureManager.h"
#include "VulkanManager.h"
class Swapchain {
public:
    Swapchain();
    ~Swapchain();

private:
    struct DepthResources {
        AllocatedImage image;
        VkImageView view;
        VkFormat format;
    };
public:
    // Prevent copying
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // Allow moving
    Swapchain(Swapchain&&) noexcept;
    Swapchain& operator=(Swapchain&&) noexcept;

    void create(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
    void destroy();
    void recreate();
    void initialiseImages();

    VkSwapchainKHR handle() const;
    VkFormat imageFormat() const;
    VkExtent2D extent() const;
    const std::vector<VkImage>& images() const;
    const std::vector<VkImageView>& imageViews() const;
    const DepthResources& depthResources() const;

private:
    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    DepthResources m_depthResources;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent{};
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createImageViews();
};

