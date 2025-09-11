/******************************************************************************/
/*!
\file   DescriptorSetManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
DescriptorSets.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "DescriptorSetManager.h"

#include "DescriptorPoolManager.h"
#include "Device.h"

DescriptorSetManager::DescriptorSetManager() {
  // Create a default pool
  const std::vector<VkDescriptorPoolSize> poolSizes = {
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Constant::MAX_TEXTURES }
  };

  m_defaultPool = VulkanManager::Get().VkDescriptorPoolManager().getPool(VulkanManager::Get().VkDescriptorPoolManager().createPool(poolSizes, 1000, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
}

DescriptorSetManager::~DescriptorSetManager()= default;

DescriptorSetManager::DescriptorSetHandle DescriptorSetManager::createDescriptorSet(
    const VkDescriptorSetLayout& createInfo, SetType type)
{
    DescriptorSetData data{};

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_defaultPool;
    allocInfo.descriptorSetCount = type == SetType::Dynamic ? Constant::FRAME_OVERLAP : 1;

    // Create an array of layouts
    std::vector layouts(allocInfo.descriptorSetCount, createInfo);
    allocInfo.pSetLayouts = layouts.data();

    data.sets.resize(allocInfo.descriptorSetCount);
    if (vkAllocateDescriptorSets(VulkanManager::Get().VkDevice().handle(), &allocInfo, data.sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    const uint32_t id = m_nextId++;
    m_descriptorSets[id] = data;
    return DescriptorSetHandle{ id, type };
}

void DescriptorSetManager::updateDescriptorSet(DescriptorSetHandle handle,
                                               std::vector<VkWriteDescriptorSet>& writes)
{
  const auto it = m_descriptorSets.find(handle.id);
  if(it != m_descriptorSets.end()) {
    if(handle.type == SetType::Static) {
      vkUpdateDescriptorSets(VulkanManager::Get().VkDevice().handle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
      return;
    }
    for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; ++i) {
      writes[i].dstSet = it->second.sets[i];
      vkUpdateDescriptorSets(VulkanManager::Get().VkDevice().handle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
  }
}

VkDescriptorSet DescriptorSetManager::getDescriptorSet(DescriptorSetHandle handle, uint32_t frameIndex) const
{
    auto it = m_descriptorSets.find(handle.id);
    if (it != m_descriptorSets.end()) {
        if (handle.type == SetType::Static) {
            return it->second.sets[0];
        }
        return it->second.sets[frameIndex % Constant::FRAME_OVERLAP];
    }
    return VK_NULL_HANDLE;
}

// Add this method to free a specific descriptor set
void DescriptorSetManager::freeDescriptorSet(DescriptorSetHandle handle) {
    auto it = m_descriptorSets.find(handle.id);
    if (it != m_descriptorSets.end()) {
        // Free the descriptor sets
        vkFreeDescriptorSets(
            VulkanManager::Get().VkDevice().handle(),
            m_defaultPool,
            static_cast<uint32_t>(it->second.sets.size()),
            it->second.sets.data()
        );
        // Remove from map
        m_descriptorSets.erase(it);
    }
}
