#pragma once
/******************************************************************************/
/*!
\file   PipelineManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
A class for managing pipelines in Vulkan.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VulkanHelper.h"

#include "Device.h"
#include "ShaderModule.h"
#include "VulkanManager.h"
struct Pipeline
{
    VkPipeline _pipeline{};
    VkPipelineLayout _pipelineLayout{};
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;

};

class PipelineBuilder
{
    friend class PipelineManager;
    public:
    PipelineBuilder();

    void addShaderModule(const ShaderModule& shaderModule);

    void setSpecializationInfo(uint32_t shaderIndex, VkSpecializationInfo* specInfo);

    void clearShaderModules();

    void addDescriptorSetLayout(VkDescriptorSetLayout layout);

    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineRenderingCreateInfo _renderingInfo;
    VkPipelineDynamicStateCreateInfo _dynamicState;
    std::vector<VkDescriptorSetLayoutCreateInfo> _descriptorSetLayoutsInfo;
    VkPipelineLayoutCreateInfo _pipelineLayoutInfo;

    VkPipelineShaderStageCreateInfo _computeStage{};
    bool _isComputePipeline{ false };
    private:
    bool prebuiltLayouts = false;
    std::vector<VkSpecializationInfo> _specializationInfo;
    std::vector<const ShaderModule*> _shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    std::vector<VkDescriptorSetLayout> _prebuiltDescriptorSetLayouts;

    void updateShaderStages();

    static int getShaderStageOrder(VkShaderStageFlagBits stage);

    Pipeline buildPipeline();

    Pipeline buildComputePipeline();
};

class PipelineManager {
    public:
    PipelineManager();

    ~PipelineManager();

    // Prevent copying
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;

    // Allow moving
    PipelineManager(PipelineManager&&) noexcept;
    PipelineManager& operator=(PipelineManager&&) noexcept;

    void createPipeline(const std::string& name, PipelineBuilder parameters);

    void createComputePipeline(
        const std::string& name,
        PipelineBuilder& builder);

    Pipeline& getPipeline(const std::string& name);

    void destroyPipeline(const std::string& name);

    void destroyAllPipelines();

    private:
    std::unordered_map<std::string, Pipeline> m_pipelines;
};
