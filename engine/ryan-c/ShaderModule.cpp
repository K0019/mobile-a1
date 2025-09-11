/******************************************************************************/
/*!
\file   ShaderModule.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
VkShaderModule.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "ShaderModule.h"
#include "Device.h"
ShaderModule::ShaderModule(const char* filePath, ShaderType type): m_type(type) {
  auto shaderModuleOpt = loadShaderModule(filePath);
  if (!shaderModuleOpt.has_value()) {
    std::string errorMsg = "Failed to load shader module " + std::string(filePath);
    throw std::runtime_error(errorMsg);
  }
  m_shaderModule = shaderModuleOpt.value();
}
ShaderModule::~ShaderModule() {
  cleanup();
}
ShaderModule::ShaderModule(ShaderModule&& other) noexcept:  m_shaderModule(other.m_shaderModule), m_type(other.m_type) {
  other.m_shaderModule = VK_NULL_HANDLE;
}
ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept {
  if (this != &other) {
    cleanup();
    m_shaderModule = other.m_shaderModule;
    m_type = other.m_type;
    other.m_shaderModule = VK_NULL_HANDLE;
  }
  return *this;
}
VkShaderModule ShaderModule::handle() const { return m_shaderModule; }
ShaderModule::ShaderType ShaderModule::type() const { return m_type; }
VkPipelineShaderStageCreateInfo ShaderModule::getPipelineShaderStageCreateInfo() const {
  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = getShaderStage(m_type);
  shaderStageInfo.module = m_shaderModule;
  shaderStageInfo.pName = "main";  // Assuming the entry point is always "main"
  return shaderStageInfo;
}
VkShaderStageFlagBits ShaderModule::getShaderStage(ShaderType type) {
  static const std::unordered_map<ShaderType, VkShaderStageFlagBits> stageMap = {
    {ShaderType::Vertex, VK_SHADER_STAGE_VERTEX_BIT},
    {ShaderType::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
    {ShaderType::Compute, VK_SHADER_STAGE_COMPUTE_BIT},
    {ShaderType::Geometry, VK_SHADER_STAGE_GEOMETRY_BIT},
    {ShaderType::TessellationControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
    {ShaderType::TessellationEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT}
  };
  return stageMap.at(type);
}
void ShaderModule::cleanup() {
  if (m_shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(VulkanManager::Get().VkDevice().handle(), m_shaderModule, pAllocator);
    m_shaderModule = VK_NULL_HANDLE;
  }
}
std::optional<VkShaderModule> ShaderModule::loadShaderModule(const char* filePath) {
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    return std::nullopt;
  }
  size_t fileSize = file.tellg();
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
  file.close();

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(VulkanManager::Get().VkDevice().handle(), &createInfo, pAllocator, &shaderModule) != VK_SUCCESS) {
    return std::nullopt;
  }
  return shaderModule;
}
