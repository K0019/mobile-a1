#pragma once
/******************************************************************************/
/*!
\file   Surface.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Class that wraps the VkSurfaceKHR and GLFWwindow.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"
#include "VulkanManager.h"
class Surface
{
  public:
  Surface();
  ~Surface();

  //Copying is not allowed.
  Surface(const Surface&) = delete;
  Surface& operator=(const Surface&) = delete;

  //Moving is not allowed.
  Surface(Surface&&) = delete;
  Surface(const Surface&&) = delete;

  VkSurfaceKHR handle() const;
  GLFWwindow* window() const;

private:
  VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
  GLFWwindow* m_window;
};

