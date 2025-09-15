/******************************************************************************/
/*!
\file   DescriptorPoolManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
DescriptorPools.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "DescriptorPoolManager.h"

#include "Device.h"
#include "VulkanHelper.h"

DescriptorPoolManager::DescriptorPoolManager() = default;

DescriptorPoolManager::~DescriptorPoolManager() {
  for (auto& pool : m_pools) {
    vkDestroyDescriptorPool(VulkanManager::Get().VkDevice().handle() ,pool, pAllocator);
  }
}

uint32_t DescriptorPoolManager::
createPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags) {
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = maxSets;
  poolInfo.flags = flags;


  VkDescriptorPool pool;
  if (vkCreateDescriptorPool(VulkanManager::Get().VkDevice().handle(), &poolInfo,  pAllocator, &pool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool!");
  }

  m_pools.push_back(pool);
  return index++;
}

VkDescriptorPool DescriptorPoolManager::getPool(uint32_t pool_index) const {
  if (pool_index >= m_pools.size()) {
    throw std::out_of_range("Descriptor pool index out of range");
  }
  return m_pools[pool_index];
}
