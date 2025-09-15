#pragma once
/******************************************************************************/
/*!
\file   Instance.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Instance class wrapper

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Vk_Incl.h"
//Wrapper class for VkInstance.
class Instance
{
public:

  Instance();
  ~Instance();

  //Copying is not allowed.
  Instance(const Instance&) = delete;
  Instance& operator=(const Instance&) = delete;

  //Allow moving
  Instance(Instance&&) = delete;
  Instance& operator=(Instance&&) = delete;

  VkInstance handle() const;

private:
  VkInstance m_instance{ VK_NULL_HANDLE };
};

