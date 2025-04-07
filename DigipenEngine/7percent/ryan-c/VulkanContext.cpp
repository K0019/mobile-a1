/******************************************************************************/
/*!
\file   VulkanContext.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Vulkan Context

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VulkanContext.h"
#include "DescriptorPoolManager.h"
#include "Device.h"
#include "Engine.h"
#include "TextureManager.h"
#include "Mesh3D.h"
#include "PipelineManager.h"
#include "Renderer.h"
#include "Surface.h"
#include "Swapchain.h"
#include "VkInit.h"

#ifdef IMGUI_ENABLED
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif
#else
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

void FrameInFlight::cleanup()
{
	//vkDestroyCommandPool( device, _commandPool, pAllocator);
	auto device = VulkanManager::Get().VkDevice().handle();
	vkDestroyFence(device, _renderFence, pAllocator);
	vkDestroySemaphore(device, _renderSemaphore, pAllocator);
	vkDestroySemaphore(device, _swapchainSemaphore, pAllocator);
	cameraBuffer.cleanup();
	instanceBuffers.cleanup();
	non_lit_instanceBuffer.cleanup();
	emissiveGlowBuffer.cleanup();
	//textInstanceBuffer.cleanup();
	debugInstanceBuffer.cleanup();
}

std::vector<VkVertexInputBindingDescription> SpriteInstanceData::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(2);

	// Vertex data binding
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex2D);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Instance data binding
	bindingDescriptions[1].binding = 1;
	bindingDescriptions[1].stride = sizeof(SpriteInstanceData);
	bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> SpriteInstanceData::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(10);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex2D, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex2D, texCoord);

	// Attribute descriptions for instance data (binding 1)
	attributeDescriptions[2].binding = 1;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(SpriteInstanceData, model);

	attributeDescriptions[3].binding = 1;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(SpriteInstanceData, model) + sizeof(glm::vec4);

	attributeDescriptions[4].binding = 1;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(SpriteInstanceData, model) + 2 * sizeof(glm::vec4);

	attributeDescriptions[5].binding = 1;
	attributeDescriptions[5].location = 5;
	attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[5].offset = offsetof(SpriteInstanceData, model) + 3 * sizeof(glm::vec4);

	attributeDescriptions[6].binding = 1;
	attributeDescriptions[6].location = 6;
	attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[6].offset = offsetof(SpriteInstanceData, color);

	attributeDescriptions[7].binding = 1;
	attributeDescriptions[7].location = 7;
	attributeDescriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[7].offset = offsetof(SpriteInstanceData, texCoords);

	attributeDescriptions[8].binding = 1;
	attributeDescriptions[8].location = 8;
	attributeDescriptions[8].format = VK_FORMAT_R32_UINT;
	attributeDescriptions[8].offset = offsetof(SpriteInstanceData, textureIndex);

	attributeDescriptions[9].binding = 1;
	attributeDescriptions[9].location = 9;
	attributeDescriptions[9].format = VK_FORMAT_R32_UINT;
	attributeDescriptions[9].offset = offsetof(SpriteInstanceData, flags);

	return attributeDescriptions;
}

/*std::vector<VkVertexInputBindingDescription> TextInstanceData::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(2);

	// Vertex data binding
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex2D);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Instance data binding
	bindingDescriptions[1].binding = 1;
	bindingDescriptions[1].stride = sizeof(TextInstanceData);
	bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> TextInstanceData::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(9);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex2D, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex2D, texCoord);

	// Attribute descriptions for instance data (binding 1)
	attributeDescriptions[2].binding = 1;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(TextInstanceData, model);

	attributeDescriptions[3].binding = 1;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(TextInstanceData, model) + sizeof(glm::vec4);

	attributeDescriptions[4].binding = 1;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(TextInstanceData, model) + 2 * sizeof(glm::vec4);

	attributeDescriptions[5].binding = 1;
	attributeDescriptions[5].location = 5;
	attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[5].offset = offsetof(TextInstanceData, model) + 3 * sizeof(glm::vec4);

	attributeDescriptions[6].binding = 1;
	attributeDescriptions[6].location = 6;
	attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[6].offset = offsetof(TextInstanceData, color);

	attributeDescriptions[7].binding = 1;
	attributeDescriptions[7].location = 7;
	attributeDescriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[7].offset = offsetof(TextInstanceData, texCoords);

	attributeDescriptions[8].binding = 1;
	attributeDescriptions[8].location = 8;
	attributeDescriptions[8].format = VK_FORMAT_R32_UINT;
	attributeDescriptions[8].offset = offsetof(TextInstanceData, textureIndex);

	/*
	attributeDescriptions[9].binding = 1;
	attributeDescriptions[9].location = 9;
	attributeDescriptions[9].format = VK_FORMAT_R32_UINT;
	attributeDescriptions[9].offset = offsetof(TextInstanceData, effectFlags);#1#

	return attributeDescriptions;
}*/

void VulkanContext::UploadContext::cleanup()
{
	auto device = VulkanManager::Get().VkDevice().handle();
	vkDestroyFence(device, _uploadFence, pAllocator);
}

VulkanContext::VulkanContext()
#ifdef IMGUI_ENABLED
	: m_imGuiRenderPass{ nullptr }
	, m_viewportDescriptorSets{}
#endif
{
}

VulkanContext::~VulkanContext()
{
	shutdown();
}

#ifdef IMGUI_ENABLED
void VulkanContext::initImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	initImgui_nonDynamic();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(VulkanManager::Get().VkSurface().window(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = VulkanManager::Get().VkInstance().handle();
	init_info.PhysicalDevice = VulkanManager::Get().VkDevice().chosenGPU();
	init_info.Device = VulkanManager::Get().VkDevice().handle();
	init_info.QueueFamily = VulkanManager::Get().VkDevice().graphicsQueueFamily();
	init_info.Queue = VulkanManager::Get().VkDevice().graphicsQueue();
	init_info.PipelineCache = VK_NULL_HANDLE;

	uint32_t magic_number = 1000;

	std::vector<VkDescriptorPoolSize> pool_sizes = { { VK_DESCRIPTOR_TYPE_SAMPLER, magic_number },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, magic_number},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, magic_number},
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, magic_number},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, magic_number },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, magic_number },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, magic_number },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, magic_number },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, magic_number },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, magic_number },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, magic_number } };

	init_info.DescriptorPool = VulkanManager::Get().VkDescriptorPoolManager().getPool(VulkanManager::Get().VkDescriptorPoolManager().createPool(pool_sizes, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT));
	init_info.RenderPass = m_imGuiRenderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = Constant::FRAME_OVERLAP;
	init_info.ImageCount = static_cast<uint32_t>(VulkanManager::Get().VkSwapchain().images().size());
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = nullptr;

	ImGui_ImplVulkan_Init(&init_info);

	vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());
}
#endif

void VulkanContext::init()
{
#ifdef _DEBUG
	const char* layerPath = "bin";
	SetEnvironmentVariableA("VK_LAYER_PATH", layerPath);
#endif

	if(Constant::enableValidationLayers) {
		if(!VulkanHelper::checkValidationLayerSupport())
			throw std::runtime_error("validation layers requested, but not available!");
	}

	VulkanManager::Get().Initialize();
	initCommands(); // for initing in context command objects
	initSyncObjs(); // encapsulate later
	setupSpritePipeline(); //leave as is
	//setupTextPipeline();
	setupDebugPipeline();
	initDescriptors(); // leave as is
#ifdef IMGUI_ENABLED
	initImGui();
#endif

	_renderer = std::make_unique<Renderer>(this);
	_renderer->initialize();
	setupLightingPipelines();
	setupCompositionPipeline();
	setupPostProcessingPipeline();
	isInitialized = true;
}

void VulkanContext::shutdown() noexcept
{
	if(!isInitialized) {
		return;
	}
	vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());
#ifdef IMGUI_ENABLED
	vkDestroyRenderPass(VulkanManager::Get().VkDevice().handle(), m_imGuiRenderPass, pAllocator);
	for(auto& framebuffer : m_imGuiFramebuffers) {
		vkDestroyFramebuffer(VulkanManager::Get().VkDevice().handle(), framebuffer, pAllocator);
	}
	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; i++) {
		if(m_viewportDescriptorSets[i] != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(m_viewportDescriptorSets[i]);
		}
	}
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
#endif

	_uploadContext.cleanup();
	for(auto& frame : _frames)
	{
		frame.cleanup();
	}

	_renderer.reset();
	VulkanManager::Get().Cleanup();
	isInitialized = false;
}

void VulkanContext::beginFrame()
{
#ifdef IMGUI_ENABLED
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
#endif
}

void VulkanContext::endFrame()
{
	auto swapchain = VulkanManager::Get().VkSwapchain().handle();
	FrameInFlight& frame = getCurrentFrame();

	VkPresentInfoKHR presentInfo = VkInit::PresentInfo();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &_renderer->imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &frame._renderSemaphore;

	VkResult result = vkQueuePresentKHR(VulkanManager::Get().VkDevice().graphicsQueue(), &presentInfo);

	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapchain();
		resized = false;
	}
	else if(result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image!");
	}

	_frameNumber++;
}

FrameInFlight& VulkanContext::getCurrentFrame() {
	return _frames[_frameNumber % Constant::FRAME_OVERLAP];
}
#ifdef IMGUI_ENABLED
VkDescriptorSet VulkanContext::getViewportDescriptorSet() const
{
	return m_viewportDescriptorSets[_frameNumber % Constant::FRAME_OVERLAP];
}
#endif

uint32_t VulkanContext::getCurrentFrameNumber() const
{
	return _frameNumber % Constant::FRAME_OVERLAP;
}

void VulkanContext::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(_uploadContext._commandBuffer);

	VulkanManager::Get().VkCommandManager().beginCommandBuffer(_uploadContext._commandBuffer, beginInfo);

	//execute the function
	function(cmd);

	VulkanManager::Get().VkCommandManager().endCommandBuffer(_uploadContext._commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	vkQueueSubmit(VulkanManager::Get().VkDevice().graphicsQueue(), 1, &submitInfo, _uploadContext._uploadFence);
	vkWaitForFences(VulkanManager::Get().VkDevice().handle(), 1, &_uploadContext._uploadFence, true, UINT64_MAX);
	vkResetFences(VulkanManager::Get().VkDevice().handle(), 1, &_uploadContext._uploadFence);
	// reset the command buffers inside the command pool
	VulkanManager::Get().VkCommandManager().resetCommandBuffer(_uploadContext._commandBuffer);
}

void VulkanContext::initCommands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = VulkanManager::Get().VkDevice().graphicsQueueFamily();

	auto pools = VulkanManager::Get().VkCommandManager().createCommandPools(commandPoolInfo, Constant::FRAME_OVERLAP + 1);

	for(int i = 0; i < Constant::FRAME_OVERLAP; i++) {

		_frames[i]._commandPool = pools[i];
		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo{};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		_frames[i]._mainCommandBuffer = VulkanManager::Get().VkCommandManager().allocateCommandBuffers(pools[i], cmdAllocInfo, 1)[0];
	}

	_uploadContext._commandPool = pools[Constant::FRAME_OVERLAP];

	VkCommandBufferAllocateInfo cmdAllocInfo{};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.pNext = nullptr;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	_uploadContext._commandBuffer = VulkanManager::Get().VkCommandManager().allocateCommandBuffers(_uploadContext._commandPool, cmdAllocInfo, 1)[0];
}

void VulkanContext::initSyncObjs()
{
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;

	for(unsigned i = 0; i < Constant::FRAME_OVERLAP; i++) {
		if(vkCreateFence(VulkanManager::Get().VkDevice().handle(), &fenceInfo, pAllocator, &_frames[i]._renderFence) != VK_SUCCESS ||
			 vkCreateSemaphore(VulkanManager::Get().VkDevice().handle(), &semaphoreInfo, pAllocator, &_frames[i]._swapchainSemaphore) != VK_SUCCESS ||
			 vkCreateSemaphore(VulkanManager::Get().VkDevice().handle(), &semaphoreInfo, pAllocator, &_frames[i]._renderSemaphore) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	fenceInfo.flags = 0;
	if(vkCreateFence(VulkanManager::Get().VkDevice().handle(), &fenceInfo, pAllocator, &_uploadContext._uploadFence) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create synchronization objects for the upload context!");
	}

	VulkanManager::Get().VkSwapchain().initialiseImages();
}

void VulkanContext::setupSpritePipeline()
{
	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo sceneLayoutInfo{};
	sceneLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	sceneLayoutInfo.bindingCount = 1;
	sceneLayoutInfo.pBindings = &uboBinding;

	VkDescriptorSetLayoutBinding samplerArrayBinding{};
	samplerArrayBinding.binding = 1;
	samplerArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerArrayBinding.descriptorCount = Constant::MAX_TEXTURES;
	samplerArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerArrayBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
	textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutInfo.bindingCount = 1;
	textureLayoutInfo.pBindings = &samplerArrayBinding;
	textureLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	const std::vector layout = { sceneLayoutInfo, textureLayoutInfo };

	PipelineBuilder builder;

	std::vector dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
	builder._dynamicState = dynamicState;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// In your pipeline creation:
	builder._depthStencil = depthStencil;

	builder._descriptorSetLayoutsInfo = layout;
	builder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	builder._viewport.x = 0.0f;
	builder._viewport.y = 0.0f;
	builder._viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
	builder._viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
	builder._viewport.minDepth = 0.0f;
	builder._viewport.maxDepth = 1.0f;

	builder._scissor.offset = { 0, 0 };
	builder._scissor.extent = VulkanManager::Get().VkSwapchain().extent();

	builder._rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

	builder._multisampling = VkInit::MultisamplingStateCreateInfo();

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	builder._colorBlendAttachment = colorBlendAttachment;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkInit::VertexInputStateCreateInfo();

	auto bindingDescriptions = SpriteInstanceData::getBindingDescriptions();
	auto attributeDescriptions = SpriteInstanceData::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	builder._vertexInputInfo = vertexInputInfo;

	auto image_format = VulkanManager::Get().VkSwapchain().imageFormat();

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &image_format;
	renderingInfo.depthAttachmentFormat = VulkanManager::Get().VkSwapchain().depthResources().format;
	// Add the rendering info to the pipeline creation
	builder._renderingInfo = renderingInfo;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(TextEffectData);

	builder._pipelineLayoutInfo.pushConstantRangeCount = 1;
	builder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	ShaderModule vertShaderModule((ST<Filepaths>::Get()->shadersSave + "/shader.vert.spv").c_str(), ShaderModule::ShaderType::Vertex);
	ShaderModule fragShaderModule((ST<Filepaths>::Get()->shadersSave + "/shader.frag.spv").c_str(), ShaderModule::ShaderType::Fragment);
	builder.addShaderModule(vertShaderModule);
	builder.addShaderModule(fragShaderModule);

	VulkanManager::Get().VkPipelineManager().createPipeline("image", builder);

	auto bindless = VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[1];
	VulkanManager::Get().VkTextureManager().SetBindlessLayout(bindless);
}

/*void VulkanContext::setupTextPipeline()
{
	PipelineBuilder builder;

	// Load text shaders
	ShaderModule textVertShader((ST<Filepaths>::Get()->shadersSave + "/text.vert.spv").c_str(), ShaderModule::ShaderType::Vertex);
	ShaderModule textFragShader((ST<Filepaths>::Get()->shadersSave + "/text.frag.spv").c_str(), ShaderModule::ShaderType::Fragment);
	builder.addShaderModule(textVertShader);
	builder.addShaderModule(textFragShader);

	builder.addDescriptorSetLayout(VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[0]);
	builder.addDescriptorSetLayout(VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[1]);

	std::vector dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
	builder._dynamicState = dynamicState;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;  // Change back to LESS_OR_EQUAL
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	builder._depthStencil = depthStencil;
	builder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	builder._viewport.x = 0.0f;
	builder._viewport.y = 0.0f;
	builder._viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
	builder._viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
	builder._viewport.minDepth = 0.0f;
	builder._viewport.maxDepth = 1.0f;

	builder._scissor.offset = { 0, 0 };
	builder._scissor.extent = VulkanManager::Get().VkSwapchain().extent();

	builder._rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

	builder._multisampling = VkInit::MultisamplingStateCreateInfo();

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	builder._colorBlendAttachment = colorBlendAttachment;

	auto bindingDescriptions = TextInstanceData::getBindingDescriptions();
	auto attributeDescriptions = TextInstanceData::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkInit::VertexInputStateCreateInfo();
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	builder._vertexInputInfo = vertexInputInfo;

	auto image_format = VulkanManager::Get().VkSwapchain().imageFormat();

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &image_format;
	renderingInfo.depthAttachmentFormat = VulkanManager::Get().VkSwapchain().depthResources().format;
	// Add the rendering info to the pipeline creation
	builder._renderingInfo = renderingInfo;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(TextEffectData);

	builder._pipelineLayoutInfo.pushConstantRangeCount = 1;
	builder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VulkanManager::Get().VkPipelineManager().createPipeline("text", builder);

}*/
void VulkanContext::setupDebugPipeline()
{
	PipelineBuilder lineBuilder;

	ShaderModule debugVertex((ST<Filepaths>::Get()->shadersSave + "/debug.vert.spv").c_str(), ShaderModule::ShaderType::Vertex);
	ShaderModule debugFrag((ST<Filepaths>::Get()->shadersSave + "/debug.frag.spv").c_str(), ShaderModule::ShaderType::Fragment);

	lineBuilder.addShaderModule(debugVertex);
	lineBuilder.addShaderModule(debugFrag);

	lineBuilder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

	auto lineVertexDescription = LineInstanceData::getBindingDescriptions();
	auto lineAttributeDescriptions = LineInstanceData::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo lineVertexInputInfo = VkInit::VertexInputStateCreateInfo();
	lineVertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(lineVertexDescription.size());
	lineVertexInputInfo.pVertexBindingDescriptions = lineVertexDescription.data();
	lineVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(lineAttributeDescriptions.size());
	lineVertexInputInfo.pVertexAttributeDescriptions = lineAttributeDescriptions.data();

	lineBuilder._vertexInputInfo = lineVertexInputInfo;

	lineBuilder._rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	lineBuilder._rasterizer.lineWidth = 2.0f;

	std::vector dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	lineBuilder._dynamicState = dynamicState;

	// We still need to set up a viewport and scissor, even though they're dynamic
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
	viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = VulkanManager::Get().VkSwapchain().extent();

	lineBuilder._viewport = viewport;
	lineBuilder._scissor = scissor;

	lineBuilder._multisampling = VkInit::MultisamplingStateCreateInfo();

	lineBuilder._colorBlendAttachment = VkInit::ColorBlendAttachmentState();

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	lineBuilder._depthStencil = depthStencil;

	lineBuilder.addDescriptorSetLayout(VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[0]);

	auto image_format = VulkanManager::Get().VkSwapchain().imageFormat();

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &image_format;
	renderingInfo.depthAttachmentFormat = VulkanManager::Get().VkSwapchain().depthResources().format;

	lineBuilder._renderingInfo = renderingInfo;

	VulkanManager::Get().VkPipelineManager().createPipeline("debug", lineBuilder);
}

void VulkanContext::setupLightingPipelines()
{
	{
		PipelineBuilder shadowBuilder;

		// Add vertex, geometry (optional), and fragment shaders
		ShaderModule shadowVertex(
			(ST<Filepaths>::Get()->shadersSave + "/shadow-gen.vert.spv").c_str(),
			ShaderModule::ShaderType::Vertex);
		ShaderModule shadowFragment(
			(ST<Filepaths>::Get()->shadersSave + "/shadow-gen.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment);

		shadowBuilder.addShaderModule(shadowVertex);
		shadowBuilder.addShaderModule(shadowFragment);

		// Configure vertex input for line segments and instance data
		VkVertexInputBindingDescription bindings[] = {
				{
				// Binding 0: Shadow caster line segments
				.binding = 0,
				.stride = sizeof(Renderer::LightingManager::ShadowCaster),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
		}
		};

		VkVertexInputAttributeDescription attributes[] = {
			// Shadow caster start position
			{
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Renderer::LightingManager::ShadowCaster, start)
			},
			{
				.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Renderer::LightingManager::ShadowCaster, end)
			}

		};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = bindings;
		vertexInputInfo.vertexAttributeDescriptionCount = 2;
		vertexInputInfo.pVertexAttributeDescriptions = attributes;
		shadowBuilder._vertexInputInfo = vertexInputInfo;
		// Line list topology for shadow casters
		shadowBuilder._inputAssembly = shadowBuilder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		shadowBuilder._inputAssembly.primitiveRestartEnable = VK_FALSE;
		// Modify rasterization state for optimal line rendering
		auto rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			rasterizer.depthBiasEnable = VK_FALSE;
		shadowBuilder._rasterizer = rasterizer;

		// Remove depth stencil state entirely as we're using color attachment
		shadowBuilder._depthStencil = {};  // Clear/remove depth testing configuration

		// Add multisample state configuration before pipeline creation
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // No MSAA needed for shadow maps
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;
		shadowBuilder._multisampling = multisampling;

		// Critical: Configure color blending for MIN operation
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_MIN;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MIN;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;  // Only need R channel
		shadowBuilder._colorBlendAttachment = colorBlendAttachment;

		VkFormat shadowMapFormat = VK_FORMAT_R32_SFLOAT;  // Single precision float
		// Update rendering info for color attachment
		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachmentFormats = &shadowMapFormat;  // Single precision float
		renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;  // No depth attachment
		shadowBuilder._renderingInfo = renderingInfo;

		// Dynamic state configuration
		static const VkDynamicState dynamicStates[] = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = 2,
				.pDynamicStates = dynamicStates
		};
		shadowBuilder._dynamicState = dynamicState;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(Renderer::LightingManager::LightData);

		shadowBuilder._pipelineLayoutInfo.pushConstantRangeCount = 1;
		shadowBuilder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		// Fixed viewport for shadow map resolution
		VkViewport viewport{
				.x = 0.0f,
				.y = 0.0f,
				.width = static_cast<float>(Renderer::LightingManager::SHADOW_MAP_WIDTH),  // 540-degree coverage
				.height = static_cast<float>(Renderer::LightingManager::MAX_ACTIVE_LIGHTS),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
		};

		VkRect2D scissor{
				.extent = {Renderer::LightingManager::SHADOW_MAP_WIDTH, Renderer::LightingManager::MAX_ACTIVE_LIGHTS}
		};

		shadowBuilder._viewport = viewport;
		shadowBuilder._scissor = scissor;

		VulkanManager::Get().VkPipelineManager().createPipeline("shadow_generation", shadowBuilder);
	}
	// 2. Shadow Refinement Compute Pipeline
	{
		PipelineBuilder shadowRefineBuilder;
		ShaderModule shadowRefineCompute(
			(ST<Filepaths>::Get()->shadersSave + "/post-shadow.comp.spv").c_str(),
			ShaderModule::ShaderType::Compute);

		shadowRefineBuilder.addShaderModule(shadowRefineCompute);

		// Add descriptor set layouts
		shadowRefineBuilder.addDescriptorSetLayout(_renderer->m_lightingSystem.shadowMapRefinementLayout);

		// Specialization constants for workgroup size
		std::vector<VkSpecializationMapEntry> specializationEntries = {
				{0, 0, sizeof(uint32_t)},  // local_size_x
				{1, sizeof(uint32_t), sizeof(uint32_t)},  // local_size_y
		};
		uint32_t specializationData[] = { 32, 1 };  // Matches refinement shader workgroup size

		VkSpecializationInfo specInfo{};
		specInfo.mapEntryCount = static_cast<uint32_t>(specializationEntries.size());
		specInfo.pMapEntries = specializationEntries.data();
		specInfo.dataSize = sizeof(specializationData);
		specInfo.pData = specializationData;

		shadowRefineBuilder.setSpecializationInfo(0, &specInfo);

		VulkanManager::Get().VkPipelineManager().createComputePipeline("shadow_refine", shadowRefineBuilder);
	}

	// 3. Final Lighting Graphics Pipeline
	{
		PipelineBuilder lightingBuilder;

		// Add shaders
		ShaderModule lightingVertex(
			(ST<Filepaths>::Get()->shadersSave + "/lighting.vert.spv").c_str(),
			ShaderModule::ShaderType::Vertex);
		ShaderModule lightingFragment(
			(ST<Filepaths>::Get()->shadersSave + "/lighting.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment);

		lightingBuilder.addShaderModule(lightingVertex);
		lightingBuilder.addShaderModule(lightingFragment);

		// Configure vertex input (fullscreen quad)
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;    // No vertex buffers needed
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;  // No vertex attributes needed
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		lightingBuilder._vertexInputInfo = vertexInputInfo;

		// Input assembly configuration for optimal triangle strip generation
		lightingBuilder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		lightingBuilder._inputAssembly.primitiveRestartEnable = VK_FALSE;
		// Configure rasterization
		lightingBuilder._rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

		// Dynamic viewport and scissor
		std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
		lightingBuilder._dynamicState = dynamicState;

		// Default viewport and scissor (will be dynamic)
		VkViewport viewport{
				.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width),
				.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
		};

		VkRect2D scissor{
				.extent = VulkanManager::Get().VkSwapchain().extent()
		};

		lightingBuilder._viewport = viewport;
		lightingBuilder._scissor = scissor;

		// Configure blending for HDR output
		VkPipelineColorBlendAttachmentState colorBlendAttachment = VkInit::ColorBlendAttachmentState();
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		lightingBuilder._colorBlendAttachment = colorBlendAttachment;

		// No depth testing needed for fullscreen pass
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		lightingBuilder._depthStencil = depthStencil;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // Basic 1x MSAA
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		lightingBuilder._multisampling = multisampling;

		// Add descriptor layouts
		lightingBuilder.addDescriptorSetLayout(VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[0]);  // Set 0 for scene data
		lightingBuilder.addDescriptorSetLayout(_renderer->m_lightingSystem.lightingPassDescriptorLayout);  // Set 1 for lighting data

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(screenData);

		lightingBuilder._pipelineLayoutInfo.pushConstantRangeCount = 1;
		lightingBuilder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		auto format = VulkanManager::Get().VkSwapchain().imageFormat();
		// Configure dynamic rendering
		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachmentFormats = &format;
		lightingBuilder._renderingInfo = renderingInfo;

		VulkanManager::Get().VkPipelineManager().createPipeline("lighting", lightingBuilder);
	}
	{
		PipelineBuilder builder;

		// Load emissive shaders
		ShaderModule emissiveVertShader((ST<Filepaths>::Get()->shadersSave + "/emissive.vert.spv").c_str(), ShaderModule::ShaderType::Vertex);
		ShaderModule emissiveFragShader((ST<Filepaths>::Get()->shadersSave + "/emissive.frag.spv").c_str(), ShaderModule::ShaderType::Fragment);
		builder.addShaderModule(emissiveVertShader);
		builder.addShaderModule(emissiveFragShader);

		// Use the same descriptor sets as regular images (camera UBO and bindless textures)
		builder.addDescriptorSetLayout(VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[0]);

		// Dynamic state for viewport and scissor
		std::vector dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
		builder._dynamicState = dynamicState;

		// Depth settings - enable test but disable writes (so glow renders over geometry but respects depth)
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_FALSE;  // Don't write to depth buffer
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		builder._depthStencil = depthStencil;
		builder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		// Set viewport and scissor (will be overridden by dynamic state)
		builder._viewport.x = 0.0f;
		builder._viewport.y = 0.0f;
		builder._viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
		builder._viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
		builder._viewport.minDepth = 0.0f;
		builder._viewport.maxDepth = 1.0f;

		builder._scissor.offset = { 0, 0 };
		builder._scissor.extent = VulkanManager::Get().VkSwapchain().extent();

		// Rasterization settings
		builder._rasterizer = VkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

		// Multisampling settings
		builder._multisampling = VkInit::MultisamplingStateCreateInfo();//

		// IMPORTANT: Use additive blending for the emissive glow effect
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;           // Additive blending
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;           // Additive blending
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;           // Not used with additive
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;          // Not used with additive
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		builder._colorBlendAttachment = colorBlendAttachment;

		// Vertex input bindings for EmissiveGlowInstance
		auto bindingDescriptions = Renderer::GlowTrailInstanceData::getBindingDescriptions();
		auto attributeDescriptions = Renderer::GlowTrailInstanceData::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkInit::VertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		builder._vertexInputInfo = vertexInputInfo;

		// Set up rendering info
		auto image_format = VulkanManager::Get().VkSwapchain().imageFormat();

		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachmentFormats = &image_format;
		renderingInfo.depthAttachmentFormat = VulkanManager::Get().VkSwapchain().depthResources().format;
		builder._renderingInfo = renderingInfo;

		// Create the pipeline
		VulkanManager::Get().VkPipelineManager().createPipeline("emissive", builder);////
	}
}

void VulkanContext::setupCompositionPipeline() {
	PipelineBuilder builder;

	// Load composition shaders
	ShaderModule composeVertShader(
		(ST<Filepaths>::Get()->shadersSave + "/compose.vert.spv").c_str(),
		ShaderModule::ShaderType::Vertex
	);
	ShaderModule composeFragShader(
		(ST<Filepaths>::Get()->shadersSave + "/compose.frag.spv").c_str(),
		ShaderModule::ShaderType::Fragment
	);
	builder.addShaderModule(composeVertShader);
	builder.addShaderModule(composeFragShader);

	// Configure descriptor layouts for composition
	// We need access to the lighting result texture
	builder.addDescriptorSetLayout(_renderer->m_renderTargetLayout);

	// Dynamic state configuration - essential for viewport/scissor flexibility
	std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
	};
	builder._dynamicState = dynamicState;

	// Depth testing configuration - disabled for fullscreen quad
	VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_FALSE,
			.depthWriteEnable = VK_FALSE
	};
	builder._depthStencil = depthStencil;

	// Configure primitive assembly for efficient quad rendering
	builder._inputAssembly = VkInit::InputAssemblyCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	);

	// Viewport/scissor configuration matching swapchain
	builder._viewport.x = 0.0f;
	builder._viewport.y = 0.0f;
	builder._viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
	builder._viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
	builder._viewport.minDepth = 0.0f;
	builder._viewport.maxDepth = 1.0f;

	builder._scissor.offset = { 0, 0 };
	builder._scissor.extent = VulkanManager::Get().VkSwapchain().extent();

	// Rasterizer configuration optimized for fullscreen quad
	VkPipelineRasterizationStateCreateInfo rasterizer{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,  // Both sides visible for quad
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE
	};
	builder._rasterizer = rasterizer;

	// Basic multisampling - no MSAA needed for composition
	builder._multisampling = VkInit::MultisamplingStateCreateInfo();

	// Configure blending for proper composition
	VkPipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,  // Additive blending
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
												VK_COLOR_COMPONENT_G_BIT |
												VK_COLOR_COMPONENT_B_BIT |
												VK_COLOR_COMPONENT_A_BIT
	};
	builder._colorBlendAttachment = colorBlendAttachment;

	// Vertex input state for the quad
	auto bindingDescriptions = std::vector<VkVertexInputBindingDescription>{
			{
					.binding = 0,
					.stride = sizeof(Vertex2D),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			}
	};

	auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription>{
			{
			// Position
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex2D, position)
	},
	{
		// TexCoord
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(Vertex2D, texCoord)
}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
		VkInit::VertexInputStateCreateInfo();
	vertexInputInfo.vertexBindingDescriptionCount =
		static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount =
		static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	builder._vertexInputInfo = vertexInputInfo;

	// Configure dynamic rendering with appropriate format
	auto imageFormat = VulkanManager::Get().VkSwapchain().imageFormat();
	VkPipelineRenderingCreateInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &imageFormat
	};
	builder._renderingInfo = renderingInfo;
	// Create the pipeline
	VulkanManager::Get().VkPipelineManager().createPipeline("compose", builder);
}

void VulkanContext::setupPostProcessingPipeline()
{
	{
		PipelineBuilder builder;
		// Load shaders
		ShaderModule brightVertShader(
			(ST<Filepaths>::Get()->shadersSave + "/compose.vert.spv").c_str(),
			ShaderModule::ShaderType::Vertex
		);
		ShaderModule brightFragShader(
			(ST<Filepaths>::Get()->shadersSave + "/bright.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment
		);
		builder.addShaderModule(brightVertShader);
		builder.addShaderModule(brightFragShader);

		// Descriptor layout for scene texture (lightingResult)
		builder.addDescriptorSetLayout(_renderer->m_renderTargetLayout);

		// Dynamic state (viewport/scissor)
		std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
				.pDynamicStates = dynamicStates.data()
		};
		builder._dynamicState = dynamicState;

		// No depth testing
		VkPipelineDepthStencilStateCreateInfo depthStencil{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_FALSE,
				.depthWriteEnable = VK_FALSE
		};
		builder._depthStencil = depthStencil;

		// Triangle list for quad
		builder._inputAssembly = VkInit::InputAssemblyCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		);

		// Viewport and scissor (half resolution for bloom)
		uint32_t bloomWidth = VulkanManager::Get().VkSwapchain().extent().width / 2;
		uint32_t bloomHeight = VulkanManager::Get().VkSwapchain().extent().height / 2;
		builder._viewport = { 0.0f, 0.0f, static_cast<float>(bloomWidth), static_cast<float>(bloomHeight), 0.0f, 1.0f };
		builder._scissor = { {0, 0}, {bloomWidth, bloomHeight} };

		// Rasterizer (no culling)
		VkPipelineRasterizationStateCreateInfo rasterizer{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE
		};
		builder._rasterizer = rasterizer;

		// No multisampling
		builder._multisampling = VkInit::MultisamplingStateCreateInfo();

		// No blending (direct output to bloom texture)
		VkPipelineColorBlendAttachmentState colorBlendAttachment{
				.blendEnable = VK_FALSE,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		builder._colorBlendAttachment = colorBlendAttachment;

		// Vertex input (same as composite)
		auto bindingDescriptions = std::vector<VkVertexInputBindingDescription>{
				{0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX}
		};
		auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription>{
				{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, position)},
				{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, texCoord)}
		};
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkInit::VertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		builder._vertexInputInfo = vertexInputInfo;

		// Dynamic rendering (match bloom texture format, e.g., R8G8B8A8)
		auto imageFormat = VulkanManager::Get().VkSwapchain().imageFormat();
		VkPipelineRenderingCreateInfo renderingInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &imageFormat
		};
		builder._renderingInfo = renderingInfo;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(float);

		builder._pipelineLayoutInfo.pushConstantRangeCount = 1;
		builder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		// Create pipeline
		VulkanManager::Get().VkPipelineManager().createPipeline("brightPass", builder);
	}
	{
		PipelineBuilder builder;
		// Load shaders
		ShaderModule blurVertShader(
			(ST<Filepaths>::Get()->shadersSave + "/compose.vert.spv").c_str(),
			ShaderModule::ShaderType::Vertex
		);
		ShaderModule blurHorizFragShader(
			(ST<Filepaths>::Get()->shadersSave + "/blur_horiz.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment
		);
		builder.addShaderModule(blurVertShader);
		builder.addShaderModule(blurHorizFragShader);
		// Descriptor layout for bloom texture
		builder.addDescriptorSetLayout(_renderer->m_renderTargetLayout);

		// Dynamic state
		std::vector dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
				.pDynamicStates = dynamicStates.data()
		};
		builder._dynamicState = dynamicState;

		// No depth
		VkPipelineDepthStencilStateCreateInfo depthStencil{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_FALSE,
				.depthWriteEnable = VK_FALSE
		};
		builder._depthStencil = depthStencil;

		// Triangle list
		builder._inputAssembly = VkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		// Half-resolution viewport/scissor
		uint32_t bloomWidth = VulkanManager::Get().VkSwapchain().extent().width / 2;
		uint32_t bloomHeight = VulkanManager::Get().VkSwapchain().extent().height / 2;
		builder._viewport = { 0.0f, 0.0f, static_cast<float>(bloomWidth), static_cast<float>(bloomHeight), 0.0f, 1.0f };
		builder._scissor = { {0, 0}, {bloomWidth, bloomHeight} };

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
		};
		builder._rasterizer = rasterizer;

		// No multisampling
		builder._multisampling = VkInit::MultisamplingStateCreateInfo();

		// No blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{
				.blendEnable = VK_FALSE,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		builder._colorBlendAttachment = colorBlendAttachment;

		// Vertex input
		auto bindingDescriptions = std::vector<VkVertexInputBindingDescription>{ {0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX} };
		auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription>{
				{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, position)},
				{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, texCoord)}
		};
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkInit::VertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		builder._vertexInputInfo = vertexInputInfo;

		// Dynamic rendering
		auto imageFormat = VulkanManager::Get().VkSwapchain().imageFormat();
		VkPipelineRenderingCreateInfo renderingInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &imageFormat
		};
		builder._renderingInfo = renderingInfo;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(float);

		builder._pipelineLayoutInfo.pushConstantRangeCount = 1;
		builder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		// Create pipeline
		VulkanManager::Get().VkPipelineManager().createPipeline("horizontalBlur", builder);

		builder.clearShaderModules();

		ShaderModule blurVertFragShader(
			(ST<Filepaths>::Get()->shadersSave + "/blur_vert.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment
		);
		builder.addShaderModule(blurVertShader);
		builder.addShaderModule(blurVertFragShader);
		VulkanManager::Get().VkPipelineManager().createPipeline("verticalBlur", builder);
	}
	{
		PipelineBuilder builder;
		// Load shaders
		ShaderModule compose(
			(ST<Filepaths>::Get()->shadersSave + "/compose.vert.spv").c_str(),
			ShaderModule::ShaderType::Vertex
		);
		ShaderModule bloomCompose(
			(ST<Filepaths>::Get()->shadersSave + "/bloom_compose.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment
		);
		builder.addShaderModule(compose);
		builder.addShaderModule(bloomCompose);
		// Descriptor layout for bloom texture
		builder.addDescriptorSetLayout(_renderer->m_renderTargetLayout);

		std::vector dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
				.pDynamicStates = dynamicStates.data()
		};
		builder._dynamicState = dynamicState;

		// Depth testing configuration - disabled for fullscreen quad
		VkPipelineDepthStencilStateCreateInfo depthStencil{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_FALSE,
				.depthWriteEnable = VK_FALSE
		};
		builder._depthStencil = depthStencil;

		// Configure primitive assembly for efficient quad rendering
		builder._inputAssembly = VkInit::InputAssemblyCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		);

		// Viewport/scissor configuration matching swapchain
		builder._viewport.x = 0.0f;
		builder._viewport.y = 0.0f;
		builder._viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
		builder._viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
		builder._viewport.minDepth = 0.0f;
		builder._viewport.maxDepth = 1.0f;

		builder._scissor.offset = { 0, 0 };
		builder._scissor.extent = VulkanManager::Get().VkSwapchain().extent();

		// Rasterizer configuration optimized for fullscreen quad
		VkPipelineRasterizationStateCreateInfo rasterizer{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,  // Both sides visible for quad
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE
		};
		builder._rasterizer = rasterizer;

		// Basic multisampling - no MSAA needed for composition
		builder._multisampling = VkInit::MultisamplingStateCreateInfo();

		// Configure blending for proper composition
		VkPipelineColorBlendAttachmentState colorBlendAttachment{
				 .blendEnable = VK_TRUE,
				 .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
				 .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
				 .colorBlendOp = VK_BLEND_OP_ADD,
				 .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				 .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				 .alphaBlendOp = VK_BLEND_OP_ADD,
				 .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
													 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		builder._colorBlendAttachment = colorBlendAttachment;

		// Vertex input state for the quad
		auto bindingDescriptions = std::vector<VkVertexInputBindingDescription>{
				{
						.binding = 0,
						.stride = sizeof(Vertex2D),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
				}
		};

		auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription>{
				{
				// Position
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex2D, position)
		},
		{
			// TexCoord
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex2D, texCoord)
	}
		};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo =
			VkInit::VertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount =
			static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		builder._vertexInputInfo = vertexInputInfo;

		// Configure dynamic rendering with appropriate format
		auto imageFormat = VulkanManager::Get().VkSwapchain().imageFormat();
		VkPipelineRenderingCreateInfo renderingInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &imageFormat
		};
		builder._renderingInfo = renderingInfo;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(float);

		builder._pipelineLayoutInfo.pushConstantRangeCount = 1;
		builder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		// Create pipeline
		VulkanManager::Get().VkPipelineManager().createPipeline("bloomCompose", builder);
	}
	{
		PipelineBuilder builder;

		// Load composition shaders
		ShaderModule composeVertShader(
			(ST<Filepaths>::Get()->shadersSave + "/compose.vert.spv").c_str(),
			ShaderModule::ShaderType::Vertex
		);
		ShaderModule composeFragShader(
			(ST<Filepaths>::Get()->shadersSave + "/vignette.frag.spv").c_str(),
			ShaderModule::ShaderType::Fragment
		);
		builder.addShaderModule(composeVertShader);
		builder.addShaderModule(composeFragShader);

		// Configure descriptor layouts for composition
		// We need access to the lighting result texture
		builder.addDescriptorSetLayout(_renderer->m_renderTargetLayout);

		// Dynamic state configuration - essential for viewport/scissor flexibility
		std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
				.pDynamicStates = dynamicStates.data()
		};
		builder._dynamicState = dynamicState;

		// Depth testing configuration - disabled for fullscreen quad
		VkPipelineDepthStencilStateCreateInfo depthStencil{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_FALSE,
				.depthWriteEnable = VK_FALSE
		};
		builder._depthStencil = depthStencil;

		// Configure primitive assembly for efficient quad rendering
		builder._inputAssembly = VkInit::InputAssemblyCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		);

		// Viewport/scissor configuration matching swapchain
		builder._viewport.x = 0.0f;
		builder._viewport.y = 0.0f;
		builder._viewport.width = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().width);
		builder._viewport.height = static_cast<float>(VulkanManager::Get().VkSwapchain().extent().height);
		builder._viewport.minDepth = 0.0f;
		builder._viewport.maxDepth = 1.0f;

		builder._scissor.offset = { 0, 0 };
		builder._scissor.extent = VulkanManager::Get().VkSwapchain().extent();

		// Rasterizer configuration optimized for fullscreen quad
		VkPipelineRasterizationStateCreateInfo rasterizer{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,  // Both sides visible for quad
				.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				.depthBiasEnable = VK_FALSE
		};
		builder._rasterizer = rasterizer;

		// Basic multisampling - no MSAA needed for composition
		builder._multisampling = VkInit::MultisamplingStateCreateInfo();
		// Configure blending for proper composition
		VkPipelineColorBlendAttachmentState colorBlendAttachment{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR, // Multiply with destination
				.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,      // Ignore destination
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
													VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		builder._colorBlendAttachment = colorBlendAttachment;

		// Vertex input state for the quad
		auto bindingDescriptions = std::vector<VkVertexInputBindingDescription>{
				{
						.binding = 0,
						.stride = sizeof(Vertex2D),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
				}
		};

		auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription>{
				{
				// Position
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex2D, position)
		},
		{
			// TexCoord
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex2D, texCoord)
	}
		};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo =
			VkInit::VertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount =
			static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		builder._vertexInputInfo = vertexInputInfo;

		// Configure dynamic rendering with appropriate format
		auto imageFormat = VulkanManager::Get().VkSwapchain().imageFormat();
		VkPipelineRenderingCreateInfo renderingInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &imageFormat
		};
		builder._renderingInfo = renderingInfo;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // This should match your shader stage
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(Renderer::VignetteSettings);

		builder._pipelineLayoutInfo.pushConstantRangeCount = 1;
		builder._pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		// Create the pipeline
		VulkanManager::Get().VkPipelineManager().createPipeline("vignette", builder);
	}
}

void VulkanContext::initDescriptors()
{
	auto UBOlayout = VulkanManager::Get().VkPipelineManager().getPipeline("image")._descriptorSetLayouts[0];
	cameraDescriptor = VulkanManager::Get().VkDescriptorSetManager().createDescriptorSet(UBOlayout, DescriptorSetManager::SetType::Dynamic);
	for(unsigned i = 0; i < Constant::FRAME_OVERLAP; i++)
	{
		// Allocate buffer for camera data
		VkBufferCreateInfo camerabufferInfo{};
		camerabufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		camerabufferInfo.size = sizeof(SceneUBO);
		camerabufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vmaCreateBuffer(VulkanManager::Get().VkAllocator(), &camerabufferInfo, &allocInfo, &_frames[i].cameraBuffer._buffer, &_frames[i].cameraBuffer._allocation, nullptr);

		// Prepare buffer info for UBO
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _frames[i].cameraBuffer._buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(SceneUBO);

		// Prepare descriptor write for UBO
		VkWriteDescriptorSet uboWrite{};
		uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboWrite.dstSet = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(cameraDescriptor, i);
		uboWrite.dstBinding = 0;
		uboWrite.dstArrayElement = 0;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboWrite.descriptorCount = 1;
		uboWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(VulkanManager::Get().VkDevice().handle(), 1, &uboWrite, 0, nullptr);
	}
}

#ifdef IMGUI_ENABLED
void VulkanContext::initImgui_nonDynamic()
{
	VkAttachmentDescription attachment = {};
	attachment.format = VulkanManager::Get().VkSwapchain().imageFormat();
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachment = {};
	colorAttachment.attachment = 0;
	colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if(vkCreateRenderPass(VulkanManager::Get().VkDevice().handle(), &renderPassInfo, nullptr, &m_imGuiRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create ImGui render pass!");
	}

	auto& swapChainImageViews = VulkanManager::Get().VkSwapchain().imageViews();
	auto swapChainExtent = VulkanManager::Get().VkSwapchain().extent();

	m_imGuiFramebuffers.resize(swapChainImageViews.size());
	for(size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
				swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_imGuiRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if(vkCreateFramebuffer(VulkanManager::Get().VkDevice().handle(), &framebufferInfo, nullptr, &m_imGuiFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create ImGui framebuffer!");
		}
	}
}

void VulkanContext::recreateImGuiFramebuffers()
{
	vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());
	vkDestroyRenderPass(VulkanManager::Get().VkDevice().handle(), m_imGuiRenderPass, pAllocator);
	for(auto& framebuffer : m_imGuiFramebuffers) {
		vkDestroyFramebuffer(VulkanManager::Get().VkDevice().handle(), framebuffer, pAllocator);
	}

	initImgui_nonDynamic();
}

#endif
void VulkanContext::recreateSwapchain()
{
	auto& engine = *ST<Engine>::Get();

	int width = 0, height = 0;
	glfwGetFramebufferSize(engine._window, &width, &height);
	while(width == 0 || height == 0) {
		glfwGetFramebufferSize(engine._window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());
	VulkanManager::Get().VkSwapchain().recreate();
#ifdef IMGUI_ENABLED
	auto& extent = engine._viewportExtent;
#else
	auto extent = VulkanManager::Get().VkSwapchain().extent();
#endif
	_renderer->resize(extent.width, extent.height);
#ifdef IMGUI_ENABLED
	recreateImGuiFramebuffers();
#endif
}
