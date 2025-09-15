#pragma once
/******************************************************************************/
/*!
\file   QueryManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Class for interfacing with the performance profiler to get frame time and other statistics.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"
#include "VulkanHelper.h"

class QueryManager
{
    uint32_t QUERY_COUNT = 2;

    std::array<VkQueryPool, Constant::FRAME_OVERLAP> timeQueryPools;
    std::array<VkQueryPool, Constant::FRAME_OVERLAP> pipelineStatsQueryPools;
    uint32_t currentFrame = 0;
public:
    QueryManager();
    ~QueryManager();

    QueryManager(const QueryManager&) = delete;
    QueryManager& operator=(const QueryManager&) = delete;

    QueryManager(QueryManager&&) noexcept;
    QueryManager& operator=(QueryManager&&) noexcept;

    void InitializeQueryPools();
    void CleanupQueryPools();
    void StartFrame(VkCommandBuffer cmd);
    void EndFrame(VkCommandBuffer cmd);

    struct FrameStats {
        float frameTime;          // in milliseconds
        uint64_t vertexCount;
        uint64_t primitiveCount;
    };

    FrameStats GetFrameStats();
};

