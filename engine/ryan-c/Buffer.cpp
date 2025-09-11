/******************************************************************************/
/*!
\file   Buffer.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Buffer.cpp contains the implementation of the Buffer class

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Buffer.h"

void AllocatedBuffer::cleanup() {
  vmaDestroyBuffer(VulkanManager::Get().VkAllocator(), _buffer, _allocation);
}
