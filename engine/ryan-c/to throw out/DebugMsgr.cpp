/******************************************************************************/
/*!
\file   DebugMsgr.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Debug Messenger.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "DebugMsgr.h"

#include "Instance.h"

DebugMessenger::~DebugMessenger()
{
    if(_debugMessenger != VK_NULL_HANDLE) {
        VulkanHelper::DestroyDebugUtilsMessengerEXT(VulkanManager::Get().VkInstance().handle(), _debugMessenger);
    }
}
DebugMessenger::DebugMessenger()
{
    if (VulkanManager::Get().VkInstance().handle() == VK_NULL_HANDLE) throw std::runtime_error("Instance is not initialized.");

    if constexpr(Constant::enableValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        VulkanHelper::DebugMessengerInfo(debugCreateInfo);

        if(VulkanHelper::CreateDebugUtilsMessengerEXT(VulkanManager::Get().VkInstance().handle(), &debugCreateInfo, &_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
    else
    {
        _debugMessenger = VK_NULL_HANDLE;
    }
}

VkDebugUtilsMessengerEXT DebugMessenger::handle() const {return _debugMessenger;}
