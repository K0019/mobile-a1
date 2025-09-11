#pragma once

/******************************************************************************/
/*!
\file   CommandManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
A manager class that handles the creation, allocation, and destruction of command pools and command buffers in Vulkan.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VulkanHelper.h"
#include "VulkanManager.h"

class CommandManager
{
public:
    using CommandPoolHandle = uint32_t;
    using CommandBufferHandle = uint32_t;
    
    CommandManager();
    ~CommandManager();


    // Prevent copying
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    // Allow moving
    CommandManager(CommandManager&&) noexcept;
    CommandManager& operator=(CommandManager&&) noexcept;

    // Create a command pool
    std::vector<CommandPoolHandle> createCommandPools(VkCommandPoolCreateInfo info, uint32_t count = 1);

    // Allocate command buffers from a pool
    std::vector<CommandBufferHandle> allocateCommandBuffers(CommandPoolHandle poolHandle, VkCommandBufferAllocateInfo cmdAllocInfo, uint32_t count = 1);

    // Begin a command buffer
    void beginCommandBuffer(CommandBufferHandle bufferHandle, VkCommandBufferBeginInfo info);

    // End a command buffer
    void endCommandBuffer(CommandBufferHandle bufferHandle);

    // Reset a command pool
    void resetCommandPool(CommandPoolHandle poolHandle, VkCommandPoolResetFlags flags = 0);

    void resetCommandBuffer(CommandBufferHandle bufferHandle, VkCommandBufferResetFlags flags = 0);

    // Destroy a command pool
    void destroyCommandPool(CommandPoolHandle poolHandle);

    // Get the underlying VkCommandBuffer (for use with other Vulkan calls)
    VkCommandBuffer getCommandBufferHandle(CommandBufferHandle handle) const;

private:
    struct PoolInfo {
        VkCommandPool pool;
        std::vector<CommandBufferHandle> bufferHandles;
    };

    struct BufferInfo {
        VkCommandBuffer buffer;
        CommandPoolHandle poolHandle;
        bool isValid;
    };

    std::vector<PoolInfo> m_pools;
    std::vector<BufferInfo> m_buffers;
    std::vector<CommandBufferHandle> m_freeBufferHandles;

    CommandPoolHandle m_nextPoolHandle = 0;
    CommandBufferHandle m_nextBufferHandle = 0;

    CommandPoolHandle addPool(VkCommandPool pool);
    CommandBufferHandle addBuffer(CommandPoolHandle poolHandle, VkCommandBuffer buffer);
    void removeBuffer(CommandBufferHandle handle);
};

