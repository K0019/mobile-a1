#pragma once
/******************************************************************************/
/*!
\file   DescriptorSet.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
A class that handles the creation and destruction of a Vulkan debug messenger.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"

#include "VulkanHelper.h"
#include "VulkanManager.h"

class DescriptorSetManager {
public:
    enum class SetType {
        Static,
        Dynamic
    };

    struct DescriptorSetHandle {
        uint32_t id = 0;
        SetType type = SetType::Static;
    };

    DescriptorSetManager();
    ~DescriptorSetManager();

    DescriptorSetManager(const DescriptorSetManager&) = delete;
    DescriptorSetManager& operator=(const DescriptorSetManager&) = delete;

    DescriptorSetManager(DescriptorSetManager&&) noexcept;
    DescriptorSetManager& operator=(DescriptorSetManager&&) noexcept;

    DescriptorSetHandle createDescriptorSet(const VkDescriptorSetLayout& createInfo, SetType type = SetType::Static);
    void updateDescriptorSet(DescriptorSetHandle handle, std::vector<VkWriteDescriptorSet>& writes);
    VkDescriptorSet getDescriptorSet(DescriptorSetHandle handle, uint32_t frameIndex = 0) const;

    void freeDescriptorSet(DescriptorSetHandle handle);

private:
    VkDescriptorPool m_defaultPool;
    struct DescriptorSetData {
        std::vector<VkDescriptorSet> sets;
    };
    std::unordered_map<uint32_t, DescriptorSetData> m_descriptorSets;
    uint32_t m_nextId = 1;  // Start from 1, use 0 as invalid handle
};
