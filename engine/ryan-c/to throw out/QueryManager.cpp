/******************************************************************************/
/*!
\file   QueryManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Query

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "QueryManager.h"

#include "ryan-c/Device.h"
#include "ryan-c/VulkanManager.h"

QueryManager::QueryManager()
{
    InitializeQueryPools();
}

QueryManager::~QueryManager()
{
    CleanupQueryPools();
}

void QueryManager::InitializeQueryPools()
{
    // Timestamp query pool
    VkQueryPoolCreateInfo timestampQueryPoolInfo{};
    timestampQueryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    timestampQueryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    timestampQueryPoolInfo.queryCount = QUERY_COUNT;

    for (auto& queryPool : timeQueryPools) {
        vkCreateQueryPool(VulkanManager::Get().VkDevice().handle(), &timestampQueryPoolInfo, nullptr, &queryPool);
    }

    // Pipeline statistics query pool
    VkQueryPoolCreateInfo pipelineStatsQueryPoolInfo{};
    pipelineStatsQueryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    pipelineStatsQueryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    pipelineStatsQueryPoolInfo.queryCount = 1;
    pipelineStatsQueryPoolInfo.pipelineStatistics = 
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;

    for (auto& queryPool : pipelineStatsQueryPools) {
        vkCreateQueryPool(VulkanManager::Get().VkDevice().handle(), &pipelineStatsQueryPoolInfo, nullptr, &queryPool);
    }
}

void QueryManager::CleanupQueryPools()
{
    for (auto& queryPool : timeQueryPools) {
        vkDestroyQueryPool(VulkanManager::Get().VkDevice().handle(), queryPool, nullptr);
    }
    for (auto& queryPool : pipelineStatsQueryPools) {
        vkDestroyQueryPool(VulkanManager::Get().VkDevice().handle(), queryPool, nullptr);
    }
}

void QueryManager::StartFrame(VkCommandBuffer cmd) {
    vkCmdResetQueryPool(cmd, timeQueryPools[currentFrame], 0, QUERY_COUNT);
    vkCmdResetQueryPool(cmd, pipelineStatsQueryPools[currentFrame], 0, 1);
    
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                        timeQueryPools[currentFrame], 0);
    
    vkCmdBeginQuery(cmd, pipelineStatsQueryPools[currentFrame], 0, 0);
}

void QueryManager::EndFrame(VkCommandBuffer cmd) {
    vkCmdEndQuery(cmd, pipelineStatsQueryPools[currentFrame], 0);
    
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                        timeQueryPools[currentFrame], 1);
}

QueryManager::FrameStats QueryManager::GetFrameStats() {
    FrameStats stats;

    // Get timestamp results
    uint64_t timestampResults[2];
    VkResult result = vkGetQueryPoolResults(VulkanManager::Get().VkDevice().handle(), 
                                            timeQueryPools[currentFrame], 0, 2,
                                            sizeof(timestampResults), timestampResults,
                                            sizeof(uint64_t), 
                                            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to get timestamp query results");
    }

    float timestampPeriod = VulkanManager::Get().VkDevice().m_gpuProperties.limits.timestampPeriod;
    stats.frameTime = (timestampResults[1] - timestampResults[0]) * timestampPeriod * 1e-6f;  // Convert to milliseconds

    // Get pipeline statistics results
    uint64_t pipelineStatsResults[2];
    result = vkGetQueryPoolResults(VulkanManager::Get().VkDevice().handle(), 
                                   pipelineStatsQueryPools[currentFrame], 0, 1,
                                   sizeof(pipelineStatsResults), pipelineStatsResults,
                                   sizeof(uint64_t), 
                                   VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to get pipeline statistics query results");
    }

    stats.vertexCount = pipelineStatsResults[0];
    stats.primitiveCount = pipelineStatsResults[1];

    currentFrame = (currentFrame + 1) % Constant::FRAME_OVERLAP;
    return stats;
}

