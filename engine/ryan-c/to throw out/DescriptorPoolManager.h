#pragma once
/******************************************************************************/
/*!
\file   DescriptorPoolManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
A manager class that handles the creation and destruction of descriptor pools in Vulkan. Mostly used to pass a pool to ImGui but also remember to cleanup the pool when done.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VulkanManager.h"
class DescriptorPoolManager {
    friend class DescriptorSetManager;
public:
    DescriptorPoolManager();

    ~DescriptorPoolManager();

    DescriptorPoolManager(const DescriptorPoolManager&) = delete;
    DescriptorPoolManager& operator=(const DescriptorPoolManager&) = delete;

    DescriptorPoolManager(DescriptorPoolManager&&) noexcept = default;
    DescriptorPoolManager& operator=(DescriptorPoolManager&&) noexcept = default;

    uint32_t createPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets,
                        VkDescriptorPoolCreateFlags flags = 0);

    VkDescriptorPool getPool(uint32_t pool_index) const;

private:
    std::vector<VkDescriptorPool> m_pools;
    uint32_t index = 0;
};

