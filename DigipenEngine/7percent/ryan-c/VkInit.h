#pragma once
/******************************************************************************/
/*!
\file   VkInit.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
namespace that contains functions to initialize Vulkan objects with default or custom values.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Vk_Incl.h"

namespace VkInit {
  VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
  VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
  //< init_cmd

  VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
  VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);

  VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

  VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

  VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                           VkSemaphoreSubmitInfo* waitSemaphoreInfo);
  VkPresentInfoKHR PresentInfo();

  VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view,
                                                VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment,
                                VkRenderingAttachmentInfo* depthAttachment);

  VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

  VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
  VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags,
                                                          uint32_t binding);
  VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding* bindings,
                                                                uint32_t bindingCount);
  VkWriteDescriptorSet WriteDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet,
                                            VkDescriptorImageInfo* imageInfo, uint32_t binding);
  VkWriteDescriptorSet WriteDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet,
                                             VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
  VkDescriptorBufferInfo BufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

  VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
  VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
  VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
  VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                                                VkShaderModule shaderModule,
                                                                const char* entry = "main");
  VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo(VkPrimitiveTopology vk_primitive_topology);
  VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo(VkPolygonMode polygonMode);
  VkPipelineMultisampleStateCreateInfo MultisamplingStateCreateInfo();

  VkPipelineColorBlendAttachmentState ColorBlendAttachmentState();
  VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();
  VkSamplerCreateInfo SamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
}  // namespace VkInit
