/******************************************************************************/
/*!
\file   PipelineManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Pipelines

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "PipelineManager.h"

#include "Device.h"
#include "VkInit.h"

PipelineBuilder::PipelineBuilder()
{
	_pipelineLayoutInfo = VkInit::PipelineLayoutCreateInfo();
}

void PipelineBuilder::addShaderModule(const ShaderModule& shaderModule) {
	_shaderModules.push_back(&shaderModule);
	updateShaderStages();
}

void PipelineBuilder::setSpecializationInfo(uint32_t shaderIndex, VkSpecializationInfo* specInfo) {
	if(shaderIndex >= _specializationInfo.size()) {
		_specializationInfo.resize(shaderIndex + 1);
	}
	_specializationInfo[shaderIndex] = *specInfo;
}

void PipelineBuilder::clearShaderModules() {
	_shaderModules.clear();
	_shaderStages.clear();
}

void PipelineBuilder::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
	_prebuiltDescriptorSetLayouts.push_back(layout);
	prebuiltLayouts = true;
}

void PipelineBuilder::updateShaderStages() {
	_shaderStages.clear();
	std::unordered_set<VkShaderStageFlagBits> usedStages;

	for(const auto* shaderModule : _shaderModules) {
		auto stageInfo = shaderModule->getPipelineShaderStageCreateInfo();
		if(usedStages.contains(stageInfo.stage)) {
			std::cout << "Warning: Duplicate shader stage detected\n";
		}
		usedStages.insert(stageInfo.stage);
		_shaderStages.push_back(stageInfo);
	}
	// Sort shader stages based on their type
	std::sort(_shaderStages.begin(), _shaderStages.end(),
						[](const VkPipelineShaderStageCreateInfo& a, const VkPipelineShaderStageCreateInfo& b) {
		return getShaderStageOrder(a.stage) < getShaderStageOrder(b.stage);
	});
}

int PipelineBuilder::getShaderStageOrder(VkShaderStageFlagBits stage) {
	switch(stage) {
		case VK_SHADER_STAGE_VERTEX_BIT: return 0;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return 1;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return 2;
		case VK_SHADER_STAGE_GEOMETRY_BIT: return 3;
		case VK_SHADER_STAGE_FRAGMENT_BIT: return 4;
		case VK_SHADER_STAGE_COMPUTE_BIT: return 5;
		case VK_SHADER_STAGE_TASK_BIT_NV: return 6;
		case VK_SHADER_STAGE_MESH_BIT_NV: return 7;
		default: return 8;
	}
}

Pipeline PipelineBuilder::buildPipeline()
{
	if(_shaderStages.empty()) {
		std::cout << "Error: No shader stages added to the pipeline\n";
		return {};
	}
	Pipeline newPipeline;

	if(!prebuiltLayouts) {
		for(const auto& descriptorSetLayoutInfo : _descriptorSetLayoutsInfo) {
			VkDescriptorSetLayout descriptorSetLayout;
			if(vkCreateDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
				std::cout << "failed to create descriptor set layout\n";
				return {};
			}
			newPipeline._descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		_pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(newPipeline._descriptorSetLayouts.size());
		_pipelineLayoutInfo.pSetLayouts = newPipeline._descriptorSetLayouts.data();
	}
	else {
		_pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(_prebuiltDescriptorSetLayouts.size());
		_pipelineLayoutInfo.pSetLayouts = _prebuiltDescriptorSetLayouts.data();
	}

	if(vkCreatePipelineLayout(VulkanManager::Get().VkDevice().handle(), &_pipelineLayoutInfo, nullptr, &newPipeline._pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &_scissor;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	for(size_t i = 0; i < _shaderStages.size(); ++i) {
		if(i < _specializationInfo.size() && _specializationInfo[i].mapEntryCount > 0) {
			_shaderStages[i].pSpecializationInfo = &_specializationInfo[i];
		}
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &_renderingInfo;
	pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &_dynamicState;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = newPipeline._pipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if(vkCreateGraphicsPipelines(VulkanManager::Get().VkDevice().handle(), VK_NULL_HANDLE, 1, &pipelineInfo, pAllocator, &newPipeline._pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline");
	}
	return newPipeline;
}

Pipeline PipelineBuilder::buildComputePipeline() {
	if (_shaderModules.size() != 1) {
		throw std::runtime_error("Compute pipeline requires exactly one compute shader!");
	}

	Pipeline newPipeline;

	// Handle descriptor set layouts similarly to graphics pipeline
	if (!prebuiltLayouts) {
		for (const auto& layoutInfo : _descriptorSetLayoutsInfo) {
			VkDescriptorSetLayout descriptorSetLayout;
			if (vkCreateDescriptorSetLayout(
					    VulkanManager::Get().VkDevice().handle(), 
					    &layoutInfo, nullptr, 
					    &descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create descriptor set layout!");
			}
			newPipeline._descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		_pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(
			newPipeline._descriptorSetLayouts.size());
		_pipelineLayoutInfo.pSetLayouts = newPipeline._descriptorSetLayouts.data();
	} else {
		_pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(
			_prebuiltDescriptorSetLayouts.size());
		_pipelineLayoutInfo.pSetLayouts = _prebuiltDescriptorSetLayouts.data();
	}

	// Create pipeline layout
	if (vkCreatePipelineLayout(
			    VulkanManager::Get().VkDevice().handle(),
			    &_pipelineLayoutInfo, nullptr,
			    &newPipeline._pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline layout!");
	}

	// Setup compute pipeline create info
	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage = _shaderStages[0]; // Use first (and only) shader stage
	computePipelineInfo.layout = newPipeline._pipelineLayout;

	// Handle specialization constants if present
	if (!_specializationInfo.empty() && 
	    _specializationInfo[0].mapEntryCount > 0) {
		computePipelineInfo.stage.pSpecializationInfo = &_specializationInfo[0];
	}

	// Create compute pipeline
	if (vkCreateComputePipelines(
			    VulkanManager::Get().VkDevice().handle(),
			    VK_NULL_HANDLE, 1,
			    &computePipelineInfo, nullptr,
			    &newPipeline._pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline!");
	}

	return newPipeline;
}

PipelineManager::PipelineManager() = default;
PipelineManager::PipelineManager(PipelineManager&&) noexcept = default;
PipelineManager& PipelineManager::operator=(PipelineManager&&) noexcept = default;

PipelineManager::~PipelineManager() {
	destroyAllPipelines();
}

void PipelineManager::createPipeline(const std::string& name, PipelineBuilder parameters) {
	if(m_pipelines.contains(name)) {
		throw std::runtime_error("Pipeline with name " + name + " already exists");
	}

	m_pipelines[name] = parameters.buildPipeline();
}

void PipelineManager::createComputePipeline(const std::string& name, PipelineBuilder& builder) {
	if (m_pipelines.contains(name)) {
		throw std::runtime_error(
				"Pipeline with name " + name + " already exists");
	}

	builder._isComputePipeline = true;
	m_pipelines[name] = builder.buildComputePipeline();
}

Pipeline& PipelineManager::getPipeline(const std::string& name) {
	auto it = m_pipelines.find(name);
	if(it == m_pipelines.end()) {
		throw std::runtime_error("Pipeline with name " + name + " not found");
	}
	return it->second;
}

void PipelineManager::destroyPipeline(const std::string& name) {
	auto it = m_pipelines.find(name);
	if(it != m_pipelines.end()) {
		vkDestroyPipeline(VulkanManager::Get().VkDevice().handle(), it->second._pipeline, pAllocator);
		vkDestroyPipelineLayout(VulkanManager::Get().VkDevice().handle(), it->second._pipelineLayout, pAllocator);
		for(auto& descriptorSetLayout : it->second._descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(), descriptorSetLayout, pAllocator);
		}
		m_pipelines.erase(it);
	}
}

void PipelineManager::destroyAllPipelines() {
	for(auto& pair : m_pipelines) {
		vkDestroyPipeline(VulkanManager::Get().VkDevice().handle(), pair.second._pipeline, pAllocator);
		vkDestroyPipelineLayout(VulkanManager::Get().VkDevice().handle(), pair.second._pipelineLayout, pAllocator);
		for(auto& descriptorSetLayout : pair.second._descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(), descriptorSetLayout, pAllocator);
		}
	}
	m_pipelines.clear();
}
