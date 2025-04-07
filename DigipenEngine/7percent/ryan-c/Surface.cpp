/******************************************************************************/
/*!
\file   Surface.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
VkSurface

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Surface.h"
#include "Engine.h"
#include "Instance.h"
#include "VulkanHelper.h"
Surface::~Surface()
{
    if(m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(VulkanManager::Get().VkInstance().handle(), m_surface,  pAllocator);
    }
}

VkSurfaceKHR Surface::handle() const { return m_surface; }
GLFWwindow* Surface::window() const { return m_window; }

Surface::Surface()
{
    auto window = ST<Engine>::Get()->_window;
    if (window == nullptr) throw std::runtime_error("Window is not initialized.");
    if (!glfwVulkanSupported()) throw std::runtime_error("Vulkan is not supported.");

    if (glfwCreateWindowSurface(VulkanManager::Get().VkInstance().handle(), window, pAllocator, &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    m_window = window;
}

