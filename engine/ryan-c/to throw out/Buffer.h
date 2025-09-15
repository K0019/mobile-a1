#pragma once
/******************************************************************************/
/*!
\file   Buffer.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
A buffer struct that utilises Vulkan Memory Allocator (VMA) to allocate memory for buffers. 

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/



#include "Vk_Incl.h"
#include "VulkanManager.h"
#include "VulkanHelper.h"

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
    void cleanup();
};//

