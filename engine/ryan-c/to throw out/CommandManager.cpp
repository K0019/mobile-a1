/******************************************************************************/
/*!
\file   CommandManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
CommandManager.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "CommandManager.h"

#include "Device.h"

CommandManager::CommandManager() = default;

CommandManager::CommandManager(CommandManager&&) noexcept = default;

CommandManager& CommandManager::operator=(CommandManager&&) noexcept = default;

CommandManager::~CommandManager()
{
	for (auto& poolinfo : m_pools)
	{
		vkDestroyCommandPool(VulkanManager::Get().VkDevice().handle(), poolinfo.pool, pAllocator);
	}
}

std::vector<CommandManager::CommandPoolHandle> CommandManager::createCommandPools(VkCommandPoolCreateInfo info, uint32_t count)
{
    std::vector<CommandPoolHandle> handles;
    handles.reserve(count);

    for (uint32_t i = 0; i < count; i++)
    {
        VkCommandPool pool;
        if (vkCreateCommandPool(VulkanManager::Get().VkDevice().handle(), &info, pAllocator, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool");
        }
        handles.push_back(addPool(pool));
    }

    return handles;
}

std::vector<CommandManager::CommandBufferHandle> CommandManager::allocateCommandBuffers(CommandPoolHandle poolHandle, VkCommandBufferAllocateInfo cmdAllocInfo, uint32_t count)
{
    cmdAllocInfo.commandPool = m_pools[poolHandle].pool;
    cmdAllocInfo.commandBufferCount = count;

    std::vector<VkCommandBuffer> buffers(count);
    if (vkAllocateCommandBuffers(VulkanManager::Get().VkDevice().handle(), &cmdAllocInfo, buffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    std::vector<CommandBufferHandle> handles;
    handles.reserve(count);
    for (auto buffer : buffers)
    {
        handles.push_back(addBuffer(poolHandle, buffer));
    }

    return handles;
}

void CommandManager::beginCommandBuffer(CommandBufferHandle bufferHandle, VkCommandBufferBeginInfo info)
{
    vkBeginCommandBuffer(getCommandBufferHandle(bufferHandle), &info);
}

void CommandManager::endCommandBuffer(CommandBufferHandle bufferHandle)
{
	vkEndCommandBuffer(getCommandBufferHandle(bufferHandle));
}

void CommandManager::resetCommandPool(CommandPoolHandle poolHandle, VkCommandPoolResetFlags flags)
{
    vkResetCommandPool(VulkanManager::Get().VkDevice().handle(), m_pools[poolHandle].pool, flags);

    // Invalidate all buffers associated with this pool
    for (auto bufferHandle : m_pools[poolHandle].bufferHandles)
    {
        m_buffers[bufferHandle].isValid = false;
        m_freeBufferHandles.push_back(bufferHandle);
    }
    m_pools[poolHandle].bufferHandles.clear();
}

void CommandManager::resetCommandBuffer(CommandBufferHandle bufferHandle, VkCommandBufferResetFlags flags)
{
	vkResetCommandBuffer(getCommandBufferHandle(bufferHandle), flags);
}

void CommandManager::destroyCommandPool(CommandPoolHandle poolHandle)
{
    // Invalidate all buffers associated with this pool
    for (auto bufferHandle : m_pools[poolHandle].bufferHandles)
    {
        m_buffers[bufferHandle].isValid = false;
        m_freeBufferHandles.push_back(bufferHandle);
    }

    vkDestroyCommandPool(VulkanManager::Get().VkDevice().handle(), m_pools[poolHandle].pool, pAllocator);
    m_pools[poolHandle] = PoolInfo{};  // Clear the pool info
}

VkCommandBuffer CommandManager::getCommandBufferHandle(CommandBufferHandle handle) const
{
    if (handle >= m_buffers.size() || !m_buffers[handle].isValid)
    {
        throw std::runtime_error("Invalid command buffer handle");
    }
    return m_buffers[handle].buffer;
}

CommandManager::CommandPoolHandle CommandManager::addPool(VkCommandPool pool)
{
    CommandPoolHandle handle = m_nextPoolHandle++;
    m_pools.push_back(PoolInfo{ pool, {} });
    return handle;
}

CommandManager::CommandBufferHandle CommandManager::addBuffer(CommandPoolHandle poolHandle, VkCommandBuffer buffer)
{
    CommandBufferHandle handle;
    if (!m_freeBufferHandles.empty()) {
        handle = m_freeBufferHandles.back();
        m_freeBufferHandles.pop_back();
        m_buffers[handle] = BufferInfo{ buffer, poolHandle, true };
    }
    else {
        handle = m_nextBufferHandle++;
        m_buffers.push_back(BufferInfo{ buffer, poolHandle, true });
    }
    m_pools[poolHandle].bufferHandles.push_back(handle);
    return handle;
}

void CommandManager::removeBuffer(CommandBufferHandle handle)
{
	m_buffers[handle].isValid = false;
	m_freeBufferHandles.push_back(handle);
}
