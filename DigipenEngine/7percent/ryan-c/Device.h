#pragma once
/******************************************************************************/
/*!
\file   Device.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Device class wrapper

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"
#include "VulkanManager.h"
class Device
{
  public:

    Device();
    ~Device();
    std::optional<uint32_t> findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findDepthFormat() const;

    //Copying is not allowed.
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    //Moving is ok.
    Device(Device&&) noexcept;
    Device& operator=(Device&&) noexcept;

    VkDevice handle() const;
    VkPhysicalDevice chosenGPU() const;
    VkQueue graphicsQueue() const;
    uint32_t graphicsQueueFamily() const;
    VkPhysicalDeviceProperties m_gpuProperties;
private:
  VkDevice m_device{ VK_NULL_HANDLE };
  VkPhysicalDevice m_chosenGPU{ VK_NULL_HANDLE };
  VkQueue m_graphicsQueue;
  uint32_t m_graphicsQueueFamily;
};

