#pragma once
/******************************************************************************/
/*!
\file   DebugMsgr.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
A class that handles the creation and destruction of a Vulkan debug messenger.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "VulkanHelper.h"
#include "VulkanManager.h"

class DebugMessenger
{
  public:
  DebugMessenger();
  ~DebugMessenger();

  //Copying is not allowed.
  DebugMessenger(const DebugMessenger&) = delete;
  DebugMessenger& operator=(const DebugMessenger&) = delete;

  //Moving is ok
  DebugMessenger(DebugMessenger&&) noexcept;
  DebugMessenger& operator=(DebugMessenger&&) noexcept;

  VkDebugUtilsMessengerEXT handle() const;

private:
  VkDebugUtilsMessengerEXT _debugMessenger{ VK_NULL_HANDLE };
};

