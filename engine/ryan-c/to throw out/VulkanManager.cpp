/******************************************************************************/
/*!
\file   VulkanManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Holds all of the vulkan objects and handles init and cleanup.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VulkanManager.h"

#include "CommandManager.h"
#include "DebugMsgr.h"
#include "DescriptorPoolManager.h"
#include "DescriptorSetManager.h"
#include "Device.h"
#include "TextureManager.h"
#include "Instance.h"
#include "PipelineManager.h"
#include "Surface.h"
#include "Swapchain.h"

VulkanManager& VulkanManager::Get() {
  static VulkanManager instance;
  return instance;
}
void VulkanManager::Initialize() {
  // Create objects in the correct order
  m_instance = std::make_unique<Instance>();
  m_debugMessenger = std::make_unique<DebugMessenger>();
  m_surface = std::make_unique<Surface>();
  m_device = std::make_unique<Device>();

  VmaVulkanFunctions vulkanFunctions {};
  vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = m_device->chosenGPU();
  allocatorInfo.device = m_device->handle();
  allocatorInfo.instance = m_instance->handle();
  allocatorInfo.pVulkanFunctions = &vulkanFunctions;

  if(vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
    throw std::runtime_error("failed to create VMA allocator!");
  }
  m_swapchain = std::make_unique<Swapchain>();
  m_pipelineManager = std::make_unique<PipelineManager>();
  m_imageManager = std::make_unique<TextureManager>();
  m_commandManager = std::make_unique<CommandManager>();
  m_descriptorPoolManager = std::make_unique<DescriptorPoolManager>();
  m_descriptorSetManager = std::make_unique<DescriptorSetManager>();
  m_queryManager = std::make_unique<QueryManager>();
}
void VulkanManager::Cleanup() {
  m_queryManager.reset();
  m_descriptorSetManager.reset();
  m_descriptorPoolManager.reset();
  m_commandManager.reset();
  m_imageManager.reset();
  m_pipelineManager.reset();
  m_swapchain.reset();
  if(m_allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(m_allocator);
    m_allocator = VK_NULL_HANDLE;
  }
  m_device.reset();
  m_surface.reset();
  m_debugMessenger.reset();
  m_instance.reset();
}
Instance& VulkanManager::VkInstance() const {
  return *m_instance;
}
Surface& VulkanManager::VkSurface() const {
  return *m_surface;
}
Device& VulkanManager::VkDevice() const {
  return *m_device;
}
Swapchain& VulkanManager::VkSwapchain() const {
  return *m_swapchain;
}
PipelineManager& VulkanManager::VkPipelineManager() const {
  return *m_pipelineManager;
}
TextureManager& VulkanManager::VkTextureManager() const {
  return *m_imageManager;
}
CommandManager& VulkanManager::VkCommandManager() const {
  return *m_commandManager;
}
DescriptorPoolManager& VulkanManager::VkDescriptorPoolManager() const {
  return *m_descriptorPoolManager;
}
DescriptorSetManager& VulkanManager::VkDescriptorSetManager() const {
  return *m_descriptorSetManager;
}

QueryManager& VulkanManager::VkQueryManager() const {
  return *m_queryManager;
}

VmaAllocator& VulkanManager::VkAllocator() {
  return m_allocator;
}
VulkanManager::VulkanManager() = default;
VulkanManager::~VulkanManager() = default;
