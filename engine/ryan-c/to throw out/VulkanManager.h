/******************************************************************************/
/*!
\file   VulkanManager.h
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

#pragma once
#include "QueryManager.h"
#include "vma_impl.h"

class DebugMessenger;
class DescriptorSetManager;
class DescriptorPoolManager;
class CommandManager;
class TextureManager;
class PipelineManager;
class Swapchain;
class Device;
class Surface;
class Instance;

class VulkanManager {
    public:

    //Singleton pattern

    static VulkanManager& Get();

    void Initialize();
    // Cleanup method
    void Cleanup();

    // Accessor methods
    Instance& VkInstance() const;

    Surface& VkSurface() const;

    Device& VkDevice() const;

    Swapchain& VkSwapchain() const;

    PipelineManager& VkPipelineManager() const;

    TextureManager& VkTextureManager() const;

    CommandManager& VkCommandManager() const;

    DescriptorPoolManager& VkDescriptorPoolManager() const;

    DescriptorSetManager& VkDescriptorSetManager() const;

    QueryManager& VkQueryManager() const;

    VmaAllocator& VkAllocator();

    private:
    VulkanManager();
    ~VulkanManager();

    VmaAllocator m_allocator{};
    std::unique_ptr<Instance> m_instance;
    std::unique_ptr<DebugMessenger> m_debugMessenger;
    std::unique_ptr<Surface> m_surface;
    std::unique_ptr<Device> m_device;
    std::unique_ptr<Swapchain> m_swapchain;
    std::unique_ptr<PipelineManager> m_pipelineManager;
    std::unique_ptr<TextureManager> m_imageManager;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<DescriptorPoolManager> m_descriptorPoolManager;
    std::unique_ptr<DescriptorSetManager> m_descriptorSetManager;
    std::unique_ptr<QueryManager> m_queryManager;
};

