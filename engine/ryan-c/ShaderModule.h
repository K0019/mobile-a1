#pragma once
/******************************************************************************/
/*!
\file   ShaderModule.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
contains a class that loads a shader module from a file and creates a VkShaderModule object, remembering to destroy it after going out of scope

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"

#include "VulkanHelper.h"
#include "VulkanManager.h"

class ShaderModule {
public:
    enum class ShaderType {
        Vertex,
        Fragment,
        Compute,
        Geometry,
        TessellationControl,
        TessellationEvaluation
    };

    ShaderModule(const char* filePath, ShaderType type);

    ~ShaderModule();

    // Prevent copying
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    // Allow moving
    ShaderModule(ShaderModule&& other) noexcept;
    ShaderModule& operator=(ShaderModule&& other) noexcept;

    VkShaderModule handle() const;
    ShaderType type() const;

    VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo() const;

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    ShaderType m_type;

    static VkShaderStageFlagBits getShaderStage(ShaderType type);

    void cleanup();

    std::optional<VkShaderModule> loadShaderModule(const char* filePath);
};
