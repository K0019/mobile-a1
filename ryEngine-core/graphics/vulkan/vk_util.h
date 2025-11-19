/*
 * This file is based on the book "Vulkan 3D Graphics Rendering Cookbook - Second Edition"
 * and code originally published under the MIT License
 * by Sergey Kosarevsky at https://github.com/corporateshark/lightweightvk.
 *
 * The original code has been copied as a starting point and has been
 * significantly modified since.
 *
 * Copyright (c) Sergey Kosarevsky, 2023-2025
 *
 * Licensed under the MIT License. See LICENSE for more details.
 */
#pragma once
#define VMA_VULKAN_VERSION 1003000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
// set to 1 to see very verbose debug console logs with Vulkan commands
#define VULKAN_PRINT_COMMANDS 0
#if !defined(VK_NO_PROTOTYPES)
#define VK_NO_PROTOTYPES 1
#endif // !defined(VK_NO_PROTOTYPES)
#include <cassert>
#include <cstdio>
#include <vector>
#include "volk.h"
#include <vk_mem_alloc.h>
#include "graphics/interface.h"
#include "vulkan/vk_enum_string_helper.h"
#include "logging/log.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VK_ASSERT(func)                                                  \
  {                                                                      \
    const VkResult vk_assert_result = func;                              \
    if (vk_assert_result != VK_SUCCESS) {                                \
      LOG_WARNING("Vulkan API call failed: {}\n  {}\n",                  \
                  #func,                                                 \
                  vk::getVulkanResultString(vk_assert_result));          \
      assert(false);                                                     \
    }                                                                    \
  }
#define VK_ASSERT_RETURN(func)                                           \
  {                                                                      \
    const VkResult vk_assert_result = func;                              \
    if (vk_assert_result != VK_SUCCESS) {                                \
      LOG_WARNING("Vulkan API call failed: {}\n  {}\n",                  \
                  #func,                                                 \
                  vk::getVulkanResultString(vk_assert_result));          \
      assert(false);                                                     \
      return getResultFromVkResult(vk_assert_result);                    \
    }                                                                    \
  }
typedef struct glslang_resource_s glslang_resource_t;

struct StageAccess
{
  VkPipelineStageFlags2 stage;
  VkAccessFlags2 access;
};

namespace vk
{
  VkSemaphore createSemaphore(VkDevice device, const char* debugName);

  VkSemaphore createSemaphoreTimeline(VkDevice device, uint64_t initialValue, const char* debugName);

  VkFence createFence(VkDevice device, const char* debugName);

  VmaAllocator createVmaAllocator(VkPhysicalDevice physDev, VkDevice device, VkInstance instance, uint32_t apiVersion);

  uint32_t findQueueFamilyIndex(VkPhysicalDevice physDev, VkQueueFlags flags);

  VkResult setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name);

  glslang_resource_t getGlslangResource(const VkPhysicalDeviceLimits& limits);

  Result compileShader(VkShaderStageFlagBits stage, const char* code, std::vector<uint8_t>* outSPIRV,
                       const glslang_resource_t* glslLangResource = nullptr);

  VkSamplerCreateInfo samplerStateDescToVkSamplerCreateInfo(const SamplerStateDesc& desc,
                                                            const VkPhysicalDeviceLimits& limits);

  VkDescriptorSetLayoutBinding getDSLBinding(uint32_t binding, VkDescriptorType descriptorType,
                                             uint32_t descriptorCount, VkShaderStageFlags stageFlags,
                                             const VkSampler* immutableSamplers = nullptr);

  VkSpecializationInfo getPipelineShaderStageSpecializationInfo(SpecializationConstantDesc desc,
                                                                VkSpecializationMapEntry* outEntries);

  VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                                                   VkShaderModule shaderModule, const char* entryPoint,
                                                                   const VkSpecializationInfo* specializationInfo);

  StageAccess getPipelineStageAccess(VkImageLayout layout);

  void imageMemoryBarrier2(VkCommandBuffer buffer, VkImage image, StageAccess src, StageAccess dst,
                           VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                           VkImageSubresourceRange subresourceRange);

  VkSampleCountFlagBits getVulkanSampleCountFlags(uint32_t numSamples, VkSampleCountFlags maxSamplesMask);

  VkShaderStageFlagBits getShaderStageFlagBitsFromFileName(const char* fileName);

  void setResultFrom(Result* outResult, VkResult result);

  Result getResultFromVkResult(VkResult result);

  const char* getVulkanResultString(VkResult result);

  uint32_t getBytesPerPixel(VkFormat format);

  uint32_t getNumImagePlanes(VkFormat format);

  Format vkFormatToFormat(VkFormat format);

  ColorSpace vkColorSpaceToColorSpace(VkColorSpaceKHR colorSpace);

  VkFormat formatToVkFormat(Format format);

  VkCompareOp compareOpToVkCompareOp(CompareOp func);

  VkExtent2D getImagePlaneExtent(VkExtent2D plane0, Format format, uint32_t plane);

  // raw Vulkan helpers: use this if you want to interop LightweightVK API with your own raw Vulkan API calls
  VkDevice getVkDevice(const IContext* ctx);

  VkPhysicalDevice getVkPhysicalDevice(const IContext* ctx);

  VkCommandBuffer getVkCommandBuffer(const ICommandBuffer& buffer);

  VkBuffer getVkBuffer(const IContext* ctx, BufferHandle buffer);

  VkImage getVkImage(const IContext* ctx, TextureHandle texture);

  VkImageView getVkImageView(const IContext* ctx, TextureHandle texture);

  VkShaderModule getVkShaderModule(const IContext* ctx, ShaderModuleHandle shader);

  VkDeviceAddress getVkAccelerationStructureDeviceAddress(const IContext* ctx, AccelStructHandle accelStruct);

  VkAccelerationStructureKHR getVkAccelerationStructure(const IContext* ctx, AccelStructHandle accelStruct);

  VkBuffer getVkBuffer(const IContext* ctx, AccelStructHandle accelStruct);

  VkPipeline getVkPipeline(const IContext* ctx, RayTracingPipelineHandle pipeline);

  VkPipelineLayout getVkPipelineLayout(const IContext* ctx, RayTracingPipelineHandle pipeline);

  // properties/limits
  const VkPhysicalDeviceProperties2& getVkPhysicalDeviceProperties2(const IContext* ctx);

  const VkPhysicalDeviceVulkan11Properties& getVkPhysicalDeviceVulkan11Properties(const IContext* ctx);

  const VkPhysicalDeviceVulkan12Properties& getVkPhysicalDeviceVulkan12Properties(const IContext* ctx);

  const VkPhysicalDeviceVulkan13Properties& getVkPhysicalDeviceVulkan13Properties(const IContext* ctx);

#if defined(__ANDROID__)
  // Android Pre-Rotation Helper: Converts SurfaceTransform enum to GLM rotation matrix
  inline glm::mat4 getPreRotationMatrix(SurfaceTransform transform)
  {
      switch (transform)
      {
      case SurfaceTransform::Rotate90:
          return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

      case SurfaceTransform::Rotate180:
          return glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

      case SurfaceTransform::Rotate270:
          return glm::rotate(glm::mat4(1.0f), glm::radians(-270.0f), glm::vec3(0.0f, 0.0f, 1.0f));

      case SurfaceTransform::Identity:
      default:
          return glm::mat4(1.0f);
      }
  }
#endif
} // namespace vk
