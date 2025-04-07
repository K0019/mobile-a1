/******************************************************************************/
/*!
\file   Renderer.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Render!

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Renderer.h"

#include <ComponentLookupWorkaround.h>
#include <ComponentLookupWorkaround.h>
#include <ComponentLookupWorkaround.h>
#include <ComponentLookupWorkaround.h>

#include "CameraController.h"
#include "CommandManager.h"
#include "Device.h"
#include "PipelineManager.h"
#include "Swapchain.h"
#include "DescriptorSetManager.h"
#include "Engine.h"
#include "TextureManager.h"

Renderer::Renderer(VulkanContext* context) : m_context(context) {
}

Renderer::~Renderer()
{
	m_lightingSystem.cleanup();
	if(m_renderTargetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(),
																 m_renderTargetLayout, nullptr);
		m_renderTargetLayout = VK_NULL_HANDLE;
	}
	m_quad.vertexBuffer.cleanup();
	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; i++) {
		m_renderTargets[i].cleanup();
		m_lightingTargets[i].cleanup();
		m_bloomTargets[i].cleanup();
	}
}

void Renderer::drawFrame() {
	FrameInFlight& frame = m_context->getCurrentFrame();
	auto& commandManager = VulkanManager::Get().VkCommandManager();
	auto cmd = commandManager.getCommandBufferHandle(frame._mainCommandBuffer);
	auto& query = VulkanManager::Get().VkQueryManager();
	auto frameIndex = m_context->getCurrentFrameNumber();
	// Frame synchronization
	vkWaitForFences(VulkanManager::Get().VkDevice().handle(), 1, &frame._renderFence, VK_TRUE, UINT64_MAX);

	// Swapchain image acquisition
	VkResult result = vkAcquireNextImageKHR(
		VulkanManager::Get().VkDevice().handle(),
		VulkanManager::Get().VkSwapchain().handle(),
		UINT64_MAX,
		frame._swapchainSemaphore,
		VK_NULL_HANDLE,
		&imageIndex
	);

	// Handle swapchain status
	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		m_context->recreateSwapchain();
		return;
	}
	if(result == VK_SUBOPTIMAL_KHR) {
		m_context->resized = true;
	}
	else if(result != VK_SUCCESS) {
		throw std::runtime_error("Failed to acquire swapchain image!");
	}

	// Reset frame resources
	vkResetFences(VulkanManager::Get().VkDevice().handle(), 1, &frame._renderFence);
	commandManager.resetCommandBuffer(frame._mainCommandBuffer);

	// Begin command buffer recording
	commandManager.beginCommandBuffer(frame._mainCommandBuffer,
																		VkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
	query.StartFrame(cmd);

	// Update scene data
	updateCameraData(ST<CameraController>::Get()->GetCameraData());
	// Setup rendering extent
#ifdef IMGUI_ENABLED
	VkExtent2D renderTarget_extent = ST<Engine>::Get()->_viewportExtent;
#else
	VkExtent2D renderTarget_extent = VulkanManager::Get().VkSwapchain().extent();
#endif
	// Main scene rendering pass
	{
		VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
			m_renderTargets[m_context->getCurrentFrameNumber()].RenderView,
			&clearColor,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		VkRenderingAttachmentInfo depthAttachment = VkInit::DepthAttachmentInfo(
			VulkanManager::Get().VkSwapchain().depthResources().view
		);

		VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
			renderTarget_extent,
			&colorAttachment,
			&depthAttachment
		);

		VulkanHelper::TransitionImage(
			cmd,
			m_renderTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		vkCmdBeginRendering(cmd, &renderingInfo);
		vkCmdSetViewport(cmd, 0, 1, &m_viewport);
		vkCmdSetScissor(cmd, 0, 1, &m_scissor);

		updateCameraBuffer();

		auto& instanceBuffer = m_context->getCurrentFrame().instanceBuffers;
		auto& BufferSize = m_context->getCurrentFrame()._instanceBufferSize;
		renderSprites(m_spriteBatches.lit, instanceBuffer, BufferSize);

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(
			cmd,
			m_renderTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
	{
		renderShadows();
		renderEmissiveGlow();
		composePasses();
	}
	{
		renderPostProcessing();
	}
	{
		VkRenderingAttachmentInfo colorAttachment{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = m_renderTargets[frameIndex].RenderView,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Preserve existing content
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		};

		VkRenderingAttachmentInfo depthAttachment = VkInit::DepthAttachmentInfo(
			VulkanManager::Get().VkSwapchain().depthResources().view
		);

		VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
			renderTarget_extent,
			&colorAttachment,
			&depthAttachment
		);

		VulkanHelper::TransitionImage(
			cmd,
			m_renderTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		vkCmdBeginRendering(cmd, &renderingInfo);
		vkCmdSetViewport(cmd, 0, 1, &m_viewport);
		vkCmdSetScissor(cmd, 0, 1, &m_scissor);

		auto& instanceBuffer = m_context->getCurrentFrame().non_lit_instanceBuffer;
		auto& BufferSize = m_context->getCurrentFrame()._non_lit_instanceBufferSize;
		renderSprites(m_spriteBatches.non_lit, instanceBuffer, BufferSize);
		renderDebug();

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(
			cmd,
			m_renderTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}


#ifdef IMGUI_ENABLED
	// ImGui integration
	VulkanHelper::TransitionImage(cmd,
																VulkanManager::Get().VkSwapchain().images()[imageIndex],
																VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
																VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	renderImGui(cmd, imageIndex);//
#else
	VulkanHelper::TransitionImage
	(
		cmd,
		m_renderTargets[frameIndex].image._image,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	// Present final image
	VulkanHelper::TransitionImage(
		cmd,
		VulkanManager::Get().VkSwapchain().images()[imageIndex],
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	// Copy lighting result to swapchain
	VkExtent2D swapchainExtent = VulkanManager::Get().VkSwapchain().extent();
	VulkanHelper::CopyImageToImage(
		cmd,
		m_renderTargets[m_context->getCurrentFrameNumber()].image._image,
		VulkanManager::Get().VkSwapchain().images()[imageIndex],
		VulkanManager::Get().VkSwapchain().extent(),
		swapchainExtent
	);

	// Final transitions for presentation
	VulkanHelper::TransitionImage(
		cmd,
		VulkanManager::Get().VkSwapchain().images()[imageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	VulkanHelper::TransitionImage
	(
		cmd,
		m_renderTargets[frameIndex].image._image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT
	);
#endif

	// End command recording
	query.EndFrame(cmd);
	commandManager.endCommandBuffer(frame._mainCommandBuffer);


	//_glyphs.clear();
	m_spriteBatches.clear();
	_lines.clear();
	m_lightingSystem.frameStates[frameIndex].clear();
	// Submit command buffer
	VkCommandBufferSubmitInfo cmdinfo = VkInit::CommandBufferSubmitInfo(cmd);
	VkSemaphoreSubmitInfo waitInfo = VkInit::SemaphoreSubmitInfo(
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
		frame._swapchainSemaphore
	);
	VkSemaphoreSubmitInfo signalInfo = VkInit::SemaphoreSubmitInfo(
		VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
		frame._renderSemaphore
	);
	VkSubmitInfo2 submit = VkInit::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

	if(vkQueueSubmit2(VulkanManager::Get().VkDevice().graphicsQueue(),
										1, &submit, frame._renderFence) != VK_SUCCESS) {

		throw std::runtime_error("Failed to submit draw command buffer!");
	}
}

Renderer::Renderer(Renderer&&) noexcept = default;

Renderer& Renderer::operator=(Renderer&&) noexcept = default;

/*
void Renderer::SpriteBatches::reserve(size_t expectedSize) {
	lit.reserve(expectedSize);
	transparent.reserve(expectedSize);
}

void Renderer::SpriteBatches::clear() {
	lit.clear();
	transparent.clear();
}

size_t Renderer::SpriteBatches::totalSize() const {
	return lit.size() + transparent.size();
}

void Renderer::SpriteBatches::concatenateInto(std::vector<SpriteInstanceData>& target) {
	target.clear();
	target.reserve(totalSize());

	// Sort and submit opaque sprites front-to-back for optimal early-z
	if(!lit.empty()) {
		std::ranges::sort(lit, [](const auto& a, const auto& b) {
			return a.model[3][2] < b.model[3][2];
		});
		target.insert(target.end(), lit.begin(), lit.end());
	}

	// Sort and submit transparent sprites back-to-front
	if(!transparent.empty()) {
		std::ranges::sort(transparent, [](const auto& a, const auto& b) {
			return a.model[3][2] > b.model[3][2];
		});
		target.insert(target.end(), transparent.begin(), transparent.end());
	}
}
*/

void Renderer::initialize() {
	// Initialize any renderer-specific resources
#ifdef IMGUI_ENABLED
	auto extent = ST<Engine>::Get()->_worldExtent;
#else
	auto extent = VulkanManager::Get().VkSwapchain().extent();
#endif
	m_lightingSystem.initialize(this);
	resize(extent.width, extent.height);
	auto Worldextent = ST<Engine>::Get()->_worldExtent;
	float halfWidth = static_cast<float>(Worldextent.width) * 0.5f;
	float halfHeight = static_cast<float>(Worldextent.height) * 0.5f;
	updateProjectionMatrix(glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -10.0f, 0.0f));
	initializeQuad();
	//
}//

VkClearValue Renderer::getClearColor() const
{
	return clearColor;
}

void Renderer::setClearColor(float r, float g, float b, float a)
{
	clearColor = { {{r, g, b, a}} };
}

void Renderer::setClearColor(const VkClearValue& color)
{
	clearColor = color;
}

void Renderer::updateCameraData(const CameraData& camera_data)
{
	updateCameraPosition(camera_data.position);
	updateCameraZoom(camera_data.zoom);
	updateCameraRotation(camera_data.rotation);
}

void Renderer::renderSprites(std::vector<SpriteInstanceData>& sprites, AllocatedBuffer& buffer, VkDeviceSize& size) const
{
	if(sprites.empty()) return;
	updateTextureBuffer(sprites, buffer, size);
	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);

	// Bind pipeline, descriptor sets, etc.
	const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("image");
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

	const auto ubo_descriptor = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(m_context->cameraDescriptor, m_context->_frameNumber % Constant::FRAME_OVERLAP);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout, 0, 1, &ubo_descriptor, 0, nullptr);

	auto bindless_set = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(VulkanManager::Get().VkTextureManager().getBindlessSet());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout, 1, 1, &bindless_set, 0, nullptr);
	// Bind vertex and instance buffers
	vkCmdPushConstants(cmd, pipeline._pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
										 0, sizeof(TextEffectData), &DEFAULT_EFFECT_DATA);//#1#

	VkBuffer vertexBuffers[] = { m_quad.vertexBuffer._buffer, buffer._buffer };
	VkDeviceSize offsets[] = { 0, 0 };
	vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);

	vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), static_cast<uint32_t>(sprites.size()), 0, 0);
}

/*void Renderer::renderText() {
	if(_glyphs.empty()) return;
	updateTextBuffer();
	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);

	const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("text");
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

	const auto ubo_descriptor = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(m_context->cameraDescriptor, m_context->_frameNumber % Constant::FRAME_OVERLAP);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout, 0, 1, &ubo_descriptor, 0, nullptr);

	auto bindless_set = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(VulkanManager::Get().VkTextureManager().getBindlessSet());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout, 1, 1, &bindless_set, 0, nullptr);

	VkBuffer vertexBuffers[] = { m_quad.vertexBuffer._buffer, frame.textInstanceBuffer._buffer };
	VkDeviceSize offsets[] = { 0, 0 };
	vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);

	vkCmdPushConstants(cmd, pipeline._pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
										 0, sizeof(TextEffectData), &DEFAULT_EFFECT_DATA);//#2#

	vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), static_cast<uint32_t>(_glyphs.size()), 0, 0);
}*/

void Renderer::renderDebug()
{
	if(_lines.empty()) return;

	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);

	updateLineBuffer();
	// Bind pipeline, descriptor sets, etc.
	const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("debug");//
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

	const auto ubo_descriptor = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(m_context->cameraDescriptor, m_context->_frameNumber % Constant::FRAME_OVERLAP);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout, 0, 1, &ubo_descriptor, 0, nullptr);

	// Bind vertex and instance buffers
	VkDeviceSize offset{ 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &frame.debugInstanceBuffer._buffer, &offset);

	vkCmdDraw(cmd, 2, static_cast<uint32_t>(_lines.size()), 0, 0);
}

void Renderer::renderShadows() {
	//
	auto frameIndex = m_context->getCurrentFrameNumber();
	auto& currentFrame = m_lightingSystem.frameStates[m_context->_frameNumber % Constant::FRAME_OVERLAP];
	if(currentFrame.lightData.empty() && currentFrame.blockers.empty()) return;

	m_lightingSystem.updateLightingData(frameIndex);

	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);//

	// 1. Shadow Generation Pass (Rasterization)
	{
		// Initial transition: Prepare shadow map for color attachment writes
		VulkanHelper::TransitionImage(cmd,
																	currentFrame.shadowMapResources.shadowMapImage._image,
																	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // Changed from DEPTH_ATTACHMENT
																	VK_IMAGE_ASPECT_COLOR_BIT);  // Changed from DEPTH_BIT

		// Configure color attachment with MIN blending
		float clearVAl = 10000.0f;	// "Infinite" distance

		VkRenderingAttachmentInfo colorAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = currentFrame.shadowMapResources.shadowMapImageView,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {.color = {clearVAl, clearVAl, clearVAl, clearVAl}},  // Initialize to "infinite" distance
		};

		colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfo renderInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = {
				.offset = {0, 0},
				.extent = {
					LightingManager::SHADOW_MAP_WIDTH,
					LightingManager::MAX_ACTIVE_LIGHTS
				}
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,  // Now using color attachment
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = nullptr  // No depth attachment needed
		};

		// Begin dynamic rendering
		vkCmdBeginRendering(cmd, &renderInfo);

		// Pipeline must be reconfigured for color attachment
		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("shadow_generation");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

		// Vertex buffer binding remains unchanged//
		VkBuffer vertexBuffers[] = {
			currentFrame.shadowCasterResources.blockerBuffer._buffer,
		};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
		// Set full viewport once
		VkViewport viewport{
				.x = 0.0f,
				.y = 0.0f,
				.width = static_cast<float>(LightingManager::SHADOW_MAP_WIDTH),
				.height = static_cast<float>(LightingManager::MAX_ACTIVE_LIGHTS),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
		};
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Process each light with row-specific scissor
		for(uint32_t lightIndex = 0; lightIndex < currentFrame.lightData.size(); ++lightIndex) {
			// Set scissor to restrict rendering to just this light's row
			VkRect2D scissor{
					.offset = {0, static_cast<int32_t>(lightIndex)},
					.extent = {LightingManager::SHADOW_MAP_WIDTH, 1}
			};
			vkCmdSetScissor(cmd, 0, 1, &scissor);

			const auto& light = currentFrame.lightData[lightIndex];
			vkCmdPushConstants(cmd, pipeline._pipelineLayout,
												 VK_SHADER_STAGE_FRAGMENT_BIT,
												 0, sizeof(LightingManager::LightData), &light);

			// Draw all shadow casters as instances
			vkCmdDraw(cmd,
								4,                                              // vertexCount
								currentFrame.shadowCasterResources.activeVertexCount,  // instanceCount
								0,                                              // firstVertex
								0);                                             // firstInstance
		}
		////
		vkCmdEndRendering(cmd);
		//
		// Transition to shader read layout for sampling
		VulkanHelper::TransitionImage(cmd,
																	currentFrame.shadowMapResources.shadowMapImage._image,
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
																	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
																	VK_IMAGE_ASPECT_COLOR_BIT);  // Changed from DEPTH_BIT
	}

	// 2. Shadow Refinement Pass (Compute)
	{
		VulkanHelper::TransitionImage(cmd,
																	currentFrame.shadowMapResources.finalShadowMapImage._image,
																	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
																	VK_IMAGE_LAYOUT_GENERAL,
																	VK_IMAGE_ASPECT_COLOR_BIT);

		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("shadow_refine");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline._pipeline);

		const auto refine_descriptor = VulkanManager::Get().VkDescriptorSetManager()
			.getDescriptorSet(currentFrame.computeResources.shadowRefinement);
		vkCmdBindDescriptorSets(cmd,
														VK_PIPELINE_BIND_POINT_COMPUTE,
														pipeline._pipelineLayout,
														0, 1, &refine_descriptor,
														0, nullptr);

		uint32_t angleGroupCount = (LightingManager::SHADOW_MAP_WIDTH + 31) / (32); // 12 groups (360 angles)
		uint32_t lightGroupCount = LightingManager::MAX_ACTIVE_LIGHTS;  // 2 groups (64 lights)
		vkCmdDispatch(cmd, angleGroupCount, lightGroupCount, 1);

		VulkanHelper::TransitionImage(cmd,
																	currentFrame.shadowMapResources.finalShadowMapImage._image,
																	VK_IMAGE_LAYOUT_GENERAL,
																	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
																	VK_IMAGE_ASPECT_COLOR_BIT);
	}

	// 3. Final Lighting Pass (Graphics)
	{
		VulkanHelper::TransitionImage(cmd,
																	m_lightingTargets[m_context->_frameNumber % Constant::FRAME_OVERLAP].image._image,
																	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
																	VK_IMAGE_ASPECT_COLOR_BIT);//

		// Begin rendering to lighting target
		VkRenderingAttachmentInfo colorAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = m_lightingTargets[m_context->_frameNumber % Constant::FRAME_OVERLAP].view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
		};


#ifdef IMGUI_ENABLED
		VkExtent2D renderTarget_extent = ST<Engine>::Get()->_viewportExtent;
#else
		VkExtent2D renderTarget_extent = VulkanManager::Get().VkSwapchain().extent();
#endif

		VkRenderingInfo renderInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = {
				.offset = {0, 0},
				.extent = renderTarget_extent
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment
		};

		vkCmdBeginRendering(cmd, &renderInfo);
		vkCmdSetViewport(cmd, 0, 1, &m_viewport);
		vkCmdSetScissor(cmd, 0, 1, &m_scissor);
		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("lighting");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

		const auto ubo_descriptor = VulkanManager::Get().VkDescriptorSetManager()
			.getDescriptorSet(m_context->cameraDescriptor,
												m_context->_frameNumber % Constant::FRAME_OVERLAP);
		vkCmdBindDescriptorSets(cmd,
														VK_PIPELINE_BIND_POINT_GRAPHICS,
														pipeline._pipelineLayout,
														0, 1, &ubo_descriptor,
														0, nullptr);

		const auto lighting_descriptor = VulkanManager::Get().VkDescriptorSetManager()
			.getDescriptorSet(currentFrame.lightResources.LightDescriptorSet);
		vkCmdBindDescriptorSets(cmd,
														VK_PIPELINE_BIND_POINT_GRAPHICS,
														pipeline._pipelineLayout,
														1, 1, &lighting_descriptor,
														0, nullptr);

		vkCmdPushConstants(cmd, pipeline._pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
											 0, sizeof(screenData), &m_screenData);//#1#

		// Draw fullscreen quad
		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(cmd,
																	m_lightingTargets[m_context->_frameNumber % Constant::FRAME_OVERLAP].image._image,
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
																	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
																	VK_IMAGE_ASPECT_COLOR_BIT);

	}
}

void Renderer::renderEmissiveGlow()
{
	if(m_spriteBatches.glow.empty()) return;
	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);
	uint32_t frameIndex = m_context->_frameNumber % Constant::FRAME_OVERLAP;
	auto& currentFrame = m_lightingSystem.frameStates[frameIndex];
	auto& bloomRes = m_bloomTargets[frameIndex];
	auto& instanceBuffer = m_context->getCurrentFrame().emissiveGlowBuffer;
	auto& BufferSize = m_context->getCurrentFrame()._emissiveGlowBufferSize;

	updateInstanceBuffer(
		m_spriteBatches.glow,
		instanceBuffer,
		BufferSize,
		[](const GlowTrailInstanceData& sprite) {return sprite.widthsAges.z; }
	);
	// Setup render extent for glow (half resolution)
#ifdef IMGUI_ENABLED
	VkExtent2D renderTargetExtent = ST<Engine>::Get()->_viewportExtent;
#else
	VkExtent2D renderTargetExtent = VulkanManager::Get().VkSwapchain().extent();
#endif
	uint32_t glowWidth = renderTargetExtent.width / 2;
	uint32_t glowHeight = renderTargetExtent.height / 2;

	VkClearValue clear_value = { {0.0f, 0.0f, 0.0f, 1.0f} };

	float base_step_size = 2.0f; // Slightly wider blur for smoother glow
	// Create a viewport for the glow rendering
	VkViewport glowViewport = {
			.x = 0.0f,
			.y = static_cast<float>(glowHeight),  // Start from the bottom like your main viewport
			.width = static_cast<float>(glowWidth),
			.height = -static_cast<float>(glowHeight),  // Negative height to match your convention
			.minDepth = 0.0f,
			.maxDepth = 1.0f
	};

	VkRect2D glowScissor = {
			.offset = {0, 0},
			.extent = {glowWidth, glowHeight}
	};

	// 1. Render emissive glow to bloomTextureA (initial glow pass)
	{
		VulkanHelper::TransitionImage(
			cmd,
			bloomRes.bloomTextureA.image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
			bloomRes.bloomTextureA.view,
			&clear_value,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
			{ glowWidth, glowHeight },
			&colorAttachment,
			nullptr
		);

		vkCmdBeginRendering(cmd, &renderingInfo);
		vkCmdSetViewport(cmd, 0, 1, &glowViewport);
		vkCmdSetScissor(cmd, 0, 1, &glowScissor);

		// Bind pipeline and descriptors
		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("emissive");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

		const auto ubo_descriptor = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(
			m_context->cameraDescriptor, frameIndex);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout,
														0, 1, &ubo_descriptor, 0, nullptr);

		// Bind vertex and instance buffers
		VkBuffer vertexBuffers[] = { m_quad.vertexBuffer._buffer, instanceBuffer._buffer };
		VkDeviceSize offsets[] = { 0, 0 };
		vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);

		// Draw instanced glow
		vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()),
							static_cast<uint32_t>(m_spriteBatches.glow.size()), 0, 0);

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(
			cmd,
			bloomRes.bloomTextureA.image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}

	// 2. Apply horizontal blur to the glow
	{
		VulkanHelper::TransitionImage(
			cmd,
			bloomRes.bloomTextureB.image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
			bloomRes.bloomTextureB.view,
			&clear_value,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
			{ glowWidth, glowHeight },
			&colorAttachment,
			nullptr
		);

		vkCmdBeginRendering(cmd, &renderingInfo);
		vkCmdSetViewport(cmd, 0, 1, &glowViewport);
		vkCmdSetScissor(cmd, 0, 1, &glowScissor);

		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("horizontalBlur");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

		VkDescriptorSet inputDescriptor = VulkanManager::Get().VkDescriptorSetManager()
			.getDescriptorSet(bloomRes.bloomTextureA.descriptorHandle);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline._pipelineLayout,
			0, 1,
			&inputDescriptor,
			0, nullptr
		);

		// You may want to use a different blur size for glow vs bloom
		float step = base_step_size * (1.0f / glowWidth);

		vkCmdPushConstants(cmd, pipeline._pipelineLayout,
											 VK_SHADER_STAGE_FRAGMENT_BIT,
											 0, sizeof(float), &step);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
		vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(
			cmd,
			bloomRes.bloomTextureB.image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}

	// 3. Apply vertical blur to the glow
	{
		VulkanHelper::TransitionImage(
			cmd,
			bloomRes.bloomTextureC.image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
			bloomRes.bloomTextureC.view,
			&clear_value,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
			{ glowWidth, glowHeight },
			&colorAttachment,
			nullptr
		);

		vkCmdBeginRendering(cmd, &renderingInfo);
		vkCmdSetViewport(cmd, 0, 1, &glowViewport);
		vkCmdSetScissor(cmd, 0, 1, &glowScissor);

		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("verticalBlur");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

		VkDescriptorSet inputDescriptor = VulkanManager::Get().VkDescriptorSetManager()
			.getDescriptorSet(bloomRes.bloomTextureB.descriptorHandle);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline._pipelineLayout,
			0, 1,
			&inputDescriptor,
			0, nullptr
		);

		float step = base_step_size * (1.0f / glowHeight);

		vkCmdPushConstants(cmd, pipeline._pipelineLayout,
											 VK_SHADER_STAGE_FRAGMENT_BIT,
											 0, sizeof(float), &step);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
		vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(
			cmd,
			bloomRes.bloomTextureC.image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}

	// 4. Composite blurred glow onto lighting target
	{
		VulkanHelper::TransitionImage(
			cmd,
			m_lightingTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		VkRenderingAttachmentInfo colorAttachment{
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = m_lightingTargets[frameIndex].view,
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
		};

		if(currentFrame.lightData.empty() && currentFrame.blockers.empty())
		{
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		VkRenderingInfo renderInfo{
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.renderArea = {
						.offset = {0, 0},
						.extent = renderTargetExtent
				},
				.layerCount = 1,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachment
		};

		vkCmdBeginRendering(cmd, &renderInfo);
		vkCmdSetViewport(cmd, 0, 1, &m_viewport);
		vkCmdSetScissor(cmd, 0, 1, &m_scissor);

		// Use the bloom compose pipeline or create a dedicated glow compose pipeline
		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("bloomCompose");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

		// Bind descriptor sets for both textures (lighting target and blurred glow)
		VkDescriptorSet descriptors =
			VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(bloomRes.bloomTextureC.descriptorHandle);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
														pipeline._pipelineLayout, 0, 1, &descriptors, 0, nullptr);

		// Push glow intensity constant (you might want a separate value for glow vs bloom
		float glowIntensity = 0.8f; // Adjust as needed
		vkCmdPushConstants(cmd, pipeline._pipelineLayout,
											 VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &glowIntensity);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
		vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

		vkCmdEndRendering(cmd);

		VulkanHelper::TransitionImage(
			cmd,
			m_lightingTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
}

void Renderer::composePasses() {
	uint32_t frameIndex = m_context->getCurrentFrameNumber() % Constant::FRAME_OVERLAP;
	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);
	auto& currentFrame = m_lightingSystem.frameStates[m_context->_frameNumber % Constant::FRAME_OVERLAP];

	if(m_spriteBatches.glow.empty() && (currentFrame.lightData.empty() && currentFrame.blockers.empty()))
	{
		return;
	}

#ifdef IMGUI_ENABLED
	VkExtent2D renderTargetExtent = ST<Engine>::Get()->_viewportExtent;
#else
	VkExtent2D renderTargetExtent = VulkanManager::Get().VkSwapchain().extent();
#endif

	// Synchronize access to render targets through explicit barriers
	VulkanHelper::TransitionImage(
		cmd,
		m_renderTargets[frameIndex].image._image,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	// Configure final composition attachment
	VkRenderingAttachmentInfo colorAttachment{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = m_renderTargets[frameIndex].RenderView,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Preserve existing content
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	};

	VkRenderingInfo renderInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.offset = {0, 0},
			.extent = renderTargetExtent
		},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachment
	};

	// Begin dynamic rendering for composition
	vkCmdBeginRendering(cmd, &renderInfo);
	vkCmdSetViewport(cmd, 0, 1, &m_viewport);
	vkCmdSetScissor(cmd, 0, 1, &m_scissor);

	// Bind composition pipeline that combines scene and lighting
	const auto& compositePipeline = VulkanManager::Get().VkPipelineManager().getPipeline("compose");
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline._pipeline);

	// Bind descriptor sets for sampling lighting results
	VkDescriptorSet lightingDescriptor = VulkanManager::Get().VkDescriptorSetManager()
		.getDescriptorSet(m_lightingTargets[frameIndex].descriptorHandle);

	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		compositePipeline._pipelineLayout,
		0, 1,
		&lightingDescriptor,
		0, nullptr
	);

	// Draw fullscreen quad to apply lighting
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
	vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

	vkCmdEndRendering(cmd);

	// Transition render target back to sampling layout for potential subsequent passes
	VulkanHelper::TransitionImage(
		cmd,
		m_renderTargets[frameIndex].image._image,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT
	);
}

void Renderer::renderPostProcessing()
{
	uint32_t frameIndex = m_context->getCurrentFrameNumber() % Constant::FRAME_OVERLAP;
	const FrameInFlight& frame = m_context->getCurrentFrame();
	auto cmd = VulkanManager::Get().VkCommandManager().getCommandBufferHandle(frame._mainCommandBuffer);
	if(m_bloomEnabled)
	{
		auto& bloomRes = m_bloomTargets[frameIndex];
#ifdef IMGUI_ENABLED
		VkExtent2D renderTargetExtent = ST<Engine>::Get()->_viewportExtent;
#else
		VkExtent2D renderTargetExtent = VulkanManager::Get().VkSwapchain().extent();
#endif
		uint32_t bloomWidth = renderTargetExtent.width / 2;
		uint32_t bloomHeight = renderTargetExtent.height / 2;
		VkClearValue clearColor2 = { {0.0f, 0.0f, 0.0f, 1.0f} };
		VkViewport bloomViewport = {
			.x = 0.0f,
			.y = static_cast<float>(bloomHeight),  // Start from the bottom like your main viewport
			.width = static_cast<float>(bloomWidth),
			.height = -static_cast<float>(bloomHeight),  // Negative height to match your convention
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkRect2D bloomScissor = {
				.offset = {0, 0},
				.extent = {bloomWidth, bloomHeight}
		};
		// 1. Bright-Pass: Extract bright areas from m_renderTargets to bloomTextureA
		{
			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureA.image._image,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
				bloomRes.bloomTextureA.view,
				&clearColor2,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
			VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
				{ bloomWidth, bloomHeight },
				&colorAttachment,
				nullptr
			);

			vkCmdBeginRendering(cmd, &renderingInfo);
			vkCmdSetViewport(cmd, 0, 1, &bloomViewport);
			vkCmdSetScissor(cmd, 0, 1, &bloomScissor);

			const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("brightPass");
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

			VkDescriptorSet inputDescriptor = VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(m_renderTargets[frameIndex].descriptorHandle);
			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline._pipelineLayout,
				0, 1,
				&inputDescriptor,
				0, nullptr
			);

			vkCmdPushConstants(cmd, pipeline._pipelineLayout,
												 VK_SHADER_STAGE_FRAGMENT_BIT,
												 0, sizeof(float), &m_bloomThreshold);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
			vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

			vkCmdEndRendering(cmd);

			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureA.image._image,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		// 2. Horizontal Blur: Blur bloomTextureA into bloomTextureB
		{
			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureB.image._image,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
				bloomRes.bloomTextureB.view,
				&clearColor2,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
			VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
				{ bloomWidth, bloomHeight },
				&colorAttachment,
				nullptr
			);

			vkCmdBeginRendering(cmd, &renderingInfo);
			vkCmdSetViewport(cmd, 0, 1, &bloomViewport);
			vkCmdSetScissor(cmd, 0, 1, &bloomScissor);

			const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("horizontalBlur");
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

			VkDescriptorSet inputDescriptor = VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(bloomRes.bloomTextureA.descriptorHandle);
			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline._pipelineLayout,
				0, 1,
				&inputDescriptor,
				0, nullptr
			);

			float step = m_blurSize / bloomWidth;

			vkCmdPushConstants(cmd, pipeline._pipelineLayout,
												 VK_SHADER_STAGE_FRAGMENT_BIT,
												 0, sizeof(float), &step);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
			vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

			vkCmdEndRendering(cmd);

			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureB.image._image,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		// 3. Vertical Blur: Blur bloomTextureB into bloomTextureC
		{
			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureC.image._image,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(
				bloomRes.bloomTextureC.view,
				&clearColor2,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);

			VkRenderingInfo renderingInfo = VkInit::RenderingInfo(
				{ bloomWidth, bloomHeight },
				&colorAttachment,
				nullptr
			);

			vkCmdBeginRendering(cmd, &renderingInfo);
			vkCmdSetViewport(cmd, 0, 1, &bloomViewport);
			vkCmdSetScissor(cmd, 0, 1, &bloomScissor);

			const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("verticalBlur");
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

			VkDescriptorSet inputDescriptor = VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(bloomRes.bloomTextureB.descriptorHandle);
			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline._pipelineLayout,
				0, 1,
				&inputDescriptor,
				0, nullptr
			);

			float step = m_blurSize / bloomHeight;

			vkCmdPushConstants(cmd, pipeline._pipelineLayout,
												 VK_SHADER_STAGE_FRAGMENT_BIT,
												 0, sizeof(float), &step);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
			vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

			vkCmdEndRendering(cmd);

			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureC.image._image,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		{
			// Synchronize access to render targets through explicit barriers
			VulkanHelper::TransitionImage(
				cmd,
				m_renderTargets[frameIndex].image._image,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			// Configure final composition attachment
			VkRenderingAttachmentInfo colorAttachment{
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = m_renderTargets[frameIndex].RenderView,
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Preserve existing content
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			};

			VkRenderingInfo renderInfo{
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.renderArea = {
					.offset = {0, 0},
					.extent = renderTargetExtent
				},
				.layerCount = 1,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachment
			};

			// Begin dynamic rendering for composition
			vkCmdBeginRendering(cmd, &renderInfo);
			vkCmdSetViewport(cmd, 0, 1, &m_viewport);
			vkCmdSetScissor(cmd, 0, 1, &m_scissor);

			const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("bloomCompose");
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);

			// Bind descriptor sets for both textures
			VkDescriptorSet descriptors = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(bloomRes.bloomTextureC.descriptorHandle);       // Bloom//
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipelineLayout, 0, 1, &descriptors, 0, nullptr);
			// Push intensity constant
			vkCmdPushConstants(cmd, pipeline._pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &m_bloomIntensity);

			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
			vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

			vkCmdEndRendering(cmd);

			// Transition render target back to sampling layout for potential subsequent passes
			VulkanHelper::TransitionImage(
				cmd,
				m_renderTargets[frameIndex].image._image,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}
	}
	if(m_vignetteEnabled)
	{
#ifdef IMGUI_ENABLED
		VkExtent2D renderTarget_extent = ST<Engine>::Get()->_viewportExtent;
#else
		VkExtent2D renderTarget_extent = VulkanManager::Get().VkSwapchain().extent();
#endif

		// Synchronize access to render targets through explicit barriers
		VulkanHelper::TransitionImage(
			cmd,
			m_renderTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		// Configure final composition attachment
		VkRenderingAttachmentInfo colorAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = m_renderTargets[frameIndex].RenderView,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Preserve existing content
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		};

		VkRenderingInfo renderInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = {
				.offset = {0, 0},
				.extent = renderTarget_extent
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment
		};

		// Begin dynamic rendering for composition
		vkCmdBeginRendering(cmd, &renderInfo);
		vkCmdSetViewport(cmd, 0, 1, &m_viewport);
		vkCmdSetScissor(cmd, 0, 1, &m_scissor);

		const auto& pipeline = VulkanManager::Get().VkPipelineManager().getPipeline("vignette");
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline);
		// Push intensity constant
		vkCmdPushConstants(
			cmd,
			pipeline._pipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(VignetteSettings),
			&m_vignetteSettings
		);


		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &m_quad.vertexBuffer._buffer, &offset);
		vkCmdDraw(cmd, static_cast<uint32_t>(m_quad.vertices.size()), 1, 0, 0);

		vkCmdEndRendering(cmd);

		// Transition render target back to sampling layout for potential subsequent passes
		VulkanHelper::TransitionImage(
			cmd,
			m_renderTargets[frameIndex].image._image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
}

#ifdef IMGUI_ENABLED
void Renderer::renderImGui(VkCommandBuffer cmd, uint32_t index)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_context->m_imGuiRenderPass;
	renderPassInfo.framebuffer = m_context->m_imGuiFramebuffers[index];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = VulkanManager::Get().VkSwapchain().extent();
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRenderPass(cmd);
}
#endif
// Add these static methods to your GlowTrailInstanceData struct
std::vector<VkVertexInputBindingDescription> Renderer::GlowTrailInstanceData::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(2);

	// Binding 0: Vertex data (quad vertices)
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(glm::vec2);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Binding 1: Instance data (all the glow trail data)
	bindingDescriptions[1].binding = 1;
	bindingDescriptions[1].stride = sizeof(GlowTrailInstanceData);
	bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Renderer::GlowTrailInstanceData::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(6);

	// Vertex attributes (binding 0)
	// Position attribute (location 0)
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2
	attributeDescriptions[0].offset = 0;

	// Instance attributes (binding 1)
	// Points attribute (location 1)
	attributeDescriptions[1].binding = 1;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	attributeDescriptions[1].offset = offsetof(GlowTrailInstanceData, points);

	// Perps attribute (location 2)
	attributeDescriptions[2].binding = 1;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	attributeDescriptions[2].offset = offsetof(GlowTrailInstanceData, perps);

	// WidthsAges attribute (location 3)
	attributeDescriptions[3].binding = 1;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	attributeDescriptions[3].offset = offsetof(GlowTrailInstanceData, widthsAges);

	// GlowColor attribute (location 4)
	attributeDescriptions[4].binding = 1;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	attributeDescriptions[4].offset = offsetof(GlowTrailInstanceData, glowColor);

	// GlowParams attribute (location 5)
	attributeDescriptions[5].binding = 1;
	attributeDescriptions[5].location = 5;
	attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	attributeDescriptions[5].offset = offsetof(GlowTrailInstanceData, glowParams);

	return attributeDescriptions;
}

void Renderer::SpriteBatches::clear()
{
	lit.clear();
	non_lit.clear();
	glow.clear();
}
void Renderer::updateProjectionMatrix(const glm::mat4& projection) {
	m_projectionMatrix = projection;
	projection_dirty = true;
}

void Renderer::updateCameraPosition(const glm::vec2& position) {
	m_cameraPosition = position;
	projection_dirty = true;
}

void Renderer::updateCameraZoom(float zoom) {
	m_zoom = zoom;
	projection_dirty = true;
}

void Renderer::updateCameraRotation(float rotation)
{
	m_rotation = rotation;
	projection_dirty = true;
}

void Renderer::resize(uint32_t width, uint32_t height) {
	vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());
	resizeAllTargets();
	resizeViewport(width, height);
}

void Renderer::resizeViewport(uint32_t width, uint32_t height) {
	m_viewport.x = 0.0f;
	m_viewport.y = static_cast<float>(height);  // Start from the bottom
	m_viewport.width = static_cast<float>(width);
	m_viewport.height = -static_cast<float>(height);  // Negative height to flip Y-axis
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { width, height };
}

VkExtent2D Renderer::getViewport() const
{
	return { static_cast<uint32_t>(m_viewport.width), static_cast<uint32_t>(m_viewport.height) };
}

void Renderer::updatePostProcessing(const PostProcessingComponent& post_processing)
{
	m_bloomEnabled = post_processing.bloomEnabled;
	m_bloomThreshold = post_processing.bloomThreshold;
	m_blurSize = post_processing.blurSize;
	m_bloomIntensity = post_processing.bloomIntensity;

	m_vignetteEnabled = post_processing.vignetteEnabled;
	m_vignetteSettings.vignetteColor = post_processing.vignetteColor;
	m_vignetteSettings.vignetteIntensity = post_processing.vignetteIntensity;
	m_vignetteSettings.vignetteRadius = post_processing.vignetteRadius;
	m_vignetteSettings.vignetteSmoothness = post_processing.vignetteSmoothness;
}

void Renderer::ResetPostProcessing()
{
	m_bloomEnabled = false;
	m_bloomThreshold = 1.0f;
	m_blurSize = 1.0f;
	m_bloomIntensity = 1.0f;
	m_vignetteEnabled = false;
	m_vignetteSettings.vignetteColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_vignetteSettings.vignetteIntensity = 0.5f;
	m_vignetteSettings.vignetteRadius = 0.5f;
	m_vignetteSettings.vignetteSmoothness = 0.5f;
}

void Renderer::renderDebugBounds(const Transform& transform)
{
	glm::vec2 position = transform.GetWorldPosition();
	glm::vec2 scale = transform.GetWorldScale();
	float rotation = transform.GetWorldRotation();
	glm::vec2 halfScale = scale * 0.5f;
	glm::vec2 corners[] = {
		position + glm::vec2(-halfScale.x, -halfScale.y),
		position + glm::vec2(halfScale.x, -halfScale.y),
		position + glm::vec2(halfScale.x, halfScale.y),
		position + glm::vec2(-halfScale.x, halfScale.y)
	};
	for(auto& corner : corners)
	{
		corner = util::RotatePoint(corner, Vector2(0, 0), rotation) + position;
	}
	Vector4 lineColor{ 1.0, 0.0,0.0,1.0f };
	// Draw all four sides of the bounding box
	for(int i = 0; i < 4; ++i)
	{
		int next = (i + 1) % 4;
		AddLineInstance(corners[i], corners[next], lineColor);
	}
	AddLineInstance(corners[3], corners[1], lineColor);
	AddLineInstance(corners[2], corners[0], lineColor);
}

void Renderer::AddRenderInstance(const RenderComponent& render_component) {
	const auto& transform = ecs::GetEntityTransform(&render_component);
	const auto& materialInstance = render_component.GetMaterialInstance();

	// Get all material information up front
	uint32_t materialFlags;
	const MaterialParameters* params;

	// Determine which parameters and flags to use
	if(materialInstance.hasParameterOverrides()) {
		materialFlags = materialInstance.getOverrideFlags();
		params = &materialInstance.getOverrideParameters();
	}
	else {
		materialFlags = ST<MaterialSystem>::Get()->getEffectiveFlags(materialInstance);
		params = &ST<MaterialSystem>::Get()->getEffectiveParameters(materialInstance);
	}

	// Extract color for base rendering
	Vector4 color = params->baseColor;

	// Early viewport culling optimization
	glm::vec2 position = transform.GetWorldPosition();
	glm::vec2 scale = transform.GetWorldScale();
	if(materialFlags & MaterialFlags::OccludesLight) {
		uint32_t frameIndex = m_context->getCurrentFrameNumber();
		m_lightingSystem.addBlocker(transform, frameIndex);
	}
	if(!isInViewport(position, scale, transform.GetWorldRotation())) {
		return;
	}

	// Validate sprite resource integrity
	const Sprite* sprite = &ResourceManager::GetSprite(render_component.GetSpriteID());
	if(!ResourceManager::SpriteExists(render_component.GetSpriteID()) ||
		 sprite->textureID == ResourceManager::INVALID_TEXTURE_ID) {
		renderDebugBounds(transform);
		return;
	}

	// Construct GPU-optimized instance data
	SpriteInstanceData data{};
	transform.SetMat4ToWorld(&data.model);
	data.texCoords = sprite->texCoords;

	if(materialFlags & MaterialFlags::Repeating) {
		glm::vec2 worldScale = transform.GetWorldScale();

		// Calculate how much to scale UVs for tiling
		// This expands the UV range beyond [0,1] to create repetition
		float scaleX = worldScale.x / sprite->width / params->tiling.x;
		float scaleY = worldScale.y / sprite->height / params->tiling.y;

		// Calculate the range between min and max coordinates
		float rangeX = data.texCoords.z - data.texCoords.x;
		float rangeY = data.texCoords.w - data.texCoords.y;

		// Scale the range (not the base coordinates)
		data.texCoords.z = data.texCoords.x + (rangeX * scaleX);
		data.texCoords.w = data.texCoords.y + (rangeY * scaleY);
	}
	// Handle sprite flipping with texture coordinate manipulation
	if(render_component.GetFlippedX()) {
		std::swap(data.texCoords.x, data.texCoords.z);
	}
	if(render_component.GetFlippedY()) {
		std::swap(data.texCoords.y, data.texCoords.w);
	}
	data.textureIndex = sprite->textureID;

	// Material property analysis
	data.color = color;
	data.flags = 0;

	// Set solid color flag if requested
	if(materialFlags & MaterialFlags::SolidColor) {
		data.flags |= RENDER_FLAG_SOLID_COLOR;
	}

	// Add to appropriate batch based on lighting needs
	if(materialFlags & MaterialFlags::ReceivesLight) {
		m_spriteBatches.lit.push_back(data);
	}
	else {
		m_spriteBatches.non_lit.push_back(data);
	}
}

void Renderer::AddTextInstance(const TextComponent& text_component) {
	const auto& atlas = ResourceManager::GetFont(text_component.GetFontHash());
	std::string const text = text_component.GetText();
	const auto& transform = ecs::GetEntityTransform(&text_component);
	auto textureIndex = atlas.textureID;
	uint32_t previousChar = 0;

	bool UI = text_component.isUI();

	glm::vec2 scale = transform.GetWorldScale();
	glm::vec2 baselineOffset = glm::vec2(0, atlas.ascender * scale.y);
	glm::vec2 TextScale{ text_component.GetWorldTextTransform().GetWorldScale() };

	// Get the pre-calculated starting position
	glm::vec2 currentPos{ text_component.GetTextStart() };

	if(!isInViewport(currentPos, TextScale, transform.GetWorldRotation())) {
		return;
	}

	previousChar = 0;
	for(char const c : text) {
		uint32_t currentChar = static_cast<uint32_t>(c);
		if(currentChar < FontAtlas::FIRST_CHAR || currentChar > FontAtlas::LAST_CHAR) continue;

		size_t glyphIndex = currentChar - FontAtlas::FIRST_CHAR;
		const Glyph& glyph = atlas.glyphs[glyphIndex];

		if(previousChar != 0) {
			float kerning = atlas.getKerning(previousChar, currentChar);
			currentPos.x += kerning * scale.x;
		}

		glm::vec2 pos = currentPos + baselineOffset;
		float glyphCenterOffsetX = (glyph.planeBounds[0].x + glyph.planeBounds[1].x) * scale.x * 0.5f;
		float glyphCenterOffsetY = (glyph.planeBounds[0].y + glyph.planeBounds[1].y) * scale.y * 0.5f;

		pos.x += glyphCenterOffsetX;
		pos.y -= glyphCenterOffsetY;

		glm::vec2 quadSize = {
			(glyph.planeBounds[1].x - glyph.planeBounds[0].x) * scale.x,
			(glyph.planeBounds[1].y - glyph.planeBounds[0].y) * scale.y
		};

		glm::mat4 model = glm::mat4(1.0f);
		model = translate(model, glm::vec3(pos, transform.GetZPos()));
		model = glm::scale(model, glm::vec3(quadSize.x, quadSize.y, 1.0f));

		glm::vec4 texCoords;
		texCoords.x = glyph.atlasBounds[0].x / atlas.width;
		texCoords.y = 1.0f - (glyph.atlasBounds[1].y / atlas.height);
		texCoords.z = (glyph.atlasBounds[1].x - glyph.atlasBounds[0].x) / atlas.width;
		texCoords.w = (glyph.atlasBounds[1].y - glyph.atlasBounds[0].y) / atlas.height;

		uint32_t flag;

		glm::vec4 color = text_component.GetColor();
		flag = RENDER_FLAG_TEXT;
		if(UI)
		{
			m_spriteBatches.non_lit.emplace_back(model,
																					 color,
																					 texCoords,
																					 textureIndex, flag);
		}
		else
		{
			m_spriteBatches.lit.emplace_back(model,
																			 color,
																			 texCoords,
																			 textureIndex, flag);
		}
		currentPos.x += (glyph.advance * scale.x);
		previousChar = currentChar;
	}
}

void Renderer::AddTrailInstance(const TrailRendererComponent& trailComp)
{
	if(trailComp.GetPointCount() < 2)
	{
		return;
	}

	const auto& transform = ecs::GetEntityTransform(&trailComp);
	float zPos = transform.GetZPos() - 0.0001f;

	// Calculate the total age span for proper interpolation
	float oldestAge = 0.0f;
	if(trailComp.GetPointCount() > 0) {
		oldestAge = trailComp.GetPoint(0).age;
	}

	// Pre-calculate segment directions for all points for joint handling
	std::vector<Vector2> directions;
	for(int i = 0; i < trailComp.GetPointCount() - 1; i++) {
		const auto& p0 = trailComp.GetPoint(i);
		const auto& p1 = trailComp.GetPoint(i + 1);

		Vector2 dir = p1.position - p0.position;
		float length = dir.Length();

		if(length > 0.0001f) {
			directions.push_back(dir / length);
		}
		else {
			// Use default or previous direction if segment is too short
			directions.push_back(directions.empty() ? Vector2(1.0f, 0.0f) : directions.back());
		}
	}

	// Now render each segment
	for(int i = 0; i < trailComp.GetPointCount() - 1; i++) {
		const auto& p0 = trailComp.GetPoint(i);
		const auto& p1 = trailComp.GetPoint(i + 1);

		// Calculate normalized ages
		float p0NormalizedAge = p0.age / trailComp.lifetime;
		float p1NormalizedAge = p1.age / trailComp.lifetime;

		// Calculate lifetime percentages (1.0 = newest, 0.0 = oldest)
		float p0AgePercent = 1.0f - p0NormalizedAge;
		float p1AgePercent = 1.0f - p1NormalizedAge;

		// Calculate widths and colors
		float p0Width = trailComp.CalculateWidth(p0AgePercent);
		float p1Width = trailComp.CalculateWidth(p1AgePercent);
		Vector4 p0Color = trailComp.CalculateColor(p0AgePercent);
		Vector4 p1Color = trailComp.CalculateColor(p1AgePercent);

		// Get current segment direction
		Vector2 currentDir = directions[i];

		// Get neighboring segment directions for joint handling
		Vector2 prevDir = (i > 0) ? directions[i - 1] : currentDir;
		Vector2 nextDir = (i < directions.size() - 1) ? directions[i + 1] : currentDir;

		// Calculate average directions at the joints for smooth connections
		// This is key to fixing the gap issue!
		Vector2 startJointDir = (prevDir + currentDir).Normalize();
		Vector2 endJointDir = (currentDir + nextDir).Normalize();

		// Calculate perpendicular directions
		Vector2 startPerp = Vector2(-startJointDir.y, startJointDir.x);
		Vector2 endPerp = Vector2(-endJointDir.y, endJointDir.x);

		// Calculate expanded bounding box
		float maxWidth = glm::max(p0Width, p1Width) * 1.2f; // Extra margin for miter joints

		Vector2 boundMin = Vector2(
			std::min(p0.position.x, p1.position.x) - maxWidth,
			std::min(p0.position.y, p1.position.y) - maxWidth
		);
		Vector2 boundMax = Vector2(
			std::max(p0.position.x, p1.position.x) + maxWidth,
			std::max(p0.position.y, p1.position.y) + maxWidth
		);
		Vector2 quadSize = boundMax - boundMin;

		// Create instance data
		SpriteInstanceData data{};
		data.model = glm::mat4(1.0f);

		// Set scale and translation for the quad
		data.model[0][0] = quadSize.x;
		data.model[1][1] = quadSize.y;
		data.model[3][0] = boundMin.x;
		data.model[3][1] = boundMin.y;
		data.model[3][2] = zPos;

		// Store points and widths
		data.model[0][3] = p0.position.x;
		data.model[1][3] = p0.position.y;
		data.model[2][3] = p1.position.x;
		data.model[2][0] = p1.position.y;
		data.model[2][1] = p0Width;
		data.model[2][2] = p1Width;

		// Use the unused parts of the matrix to store joint information
		// We can use these spots which are usually set to 0 or 1 in a standard matrix:
		data.model[0][1] = startPerp.x;  // Usually 0
		data.model[0][2] = startPerp.y;  // Usually 0
		data.model[1][0] = endPerp.x;    // Usually 0
		data.model[1][2] = endPerp.y;    // Usually 0

		// Store colors
		data.color = p0Color;
		data.texCoords = p1Color;

		// Pack just the smoothing into textureIndex (much safer)
		data.textureIndex = static_cast<uint32_t>(65535.0f * trailComp.smoothing);

		// Set trail flag
		data.flags = RENDER_FLAG_TRAIL;

		// Add to appropriate batch
		m_spriteBatches.lit.push_back(data);

		if(trailComp.glow.enabled)
		{
			// Create glow trail instance data
			GlowTrailInstanceData glowData;

			// Pack points and perpendicular vectors (same as trail)
			glowData.points = glm::vec4(p0.position.x, p0.position.y, p1.position.x, p1.position.y);
			glowData.perps = glm::vec4(startPerp.x, startPerp.y, endPerp.x, endPerp.y);

			// Apply radius multiplier to the widths specifically for glow
			float p0GlowWidth = p0Width * trailComp.glow.radius;
			float p1GlowWidth = p1Width * trailComp.glow.radius;

			// Pack widths and normalized ages
			float p0NormalizedAgeFromLifetime = p0.age / trailComp.lifetime; // Use lifetime instead of oldestAge
			float p1NormalizedAgeFromLifetime = p1.age / trailComp.lifetime;
			glowData.widthsAges = glm::vec4(p0GlowWidth, p1GlowWidth, p0NormalizedAgeFromLifetime, p1NormalizedAgeFromLifetime);

			// Set glow color (either direct or modified by the trail color)
			Vector4 glowColor = trailComp.glow.color;
			glowColor.w *= p0Color.w; // Multiply glow alpha by trail alpha
			// This makes the glow fade out with the trail
			glowData.glowColor = glowColor;

			// Pack glow parameters
			glowData.glowParams = glm::vec4(
				trailComp.glow.intensity,
				trailComp.glow.decay,
				trailComp.smoothing,
				0.0f // Unused
			);

			// Add to the glow batch
			m_spriteBatches.glow.push_back(glowData);
		}
	}
}
void Renderer::AddLineInstance(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color)
{
	LineInstanceData data;
	data.start = start;
	data.end = end;
	data.color = color;
	_lines.push_back(data);
}

void Renderer::AddLightInstance(const LightComponent& light_component)
{
	m_lightingSystem.addLight(light_component, m_context->getCurrentFrameNumber());
}

void Renderer::RenderTarget::cleanup()
{
	vmaDestroyImage(VulkanManager::Get().VkAllocator(), image._image, image._allocation);
	vkDestroySampler(VulkanManager::Get().VkDevice().handle(), sampler, nullptr);
	vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), RenderView, nullptr);
#ifdef IMGUI_ENABLED
	vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), ImGuiView, nullptr);
#endif
	VulkanManager::Get().VkDescriptorSetManager().freeDescriptorSet(descriptorHandle);
}

void Renderer::Target::cleanup()
{
	vmaDestroyImage(VulkanManager::Get().VkAllocator(), image._image, image._allocation);
	vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), view, nullptr);
	vkDestroySampler(VulkanManager::Get().VkDevice().handle(), sampler, nullptr);
	VulkanManager::Get().VkDescriptorSetManager().freeDescriptorSet(descriptorHandle);
}

void Renderer::BloomResources::cleanup()
{
	bloomTextureA.cleanup();
	bloomTextureB.cleanup();
	bloomTextureC.cleanup();
}

void Renderer::LightingManager::ShadowMapResources::cleanup()
{
	if(shadowMapImage._image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(VulkanManager::Get().VkAllocator(), shadowMapImage._image, shadowMapImage._allocation);
	}
	if(shadowMapImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), shadowMapImageView, nullptr);
	}
	if(shadowMapSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(VulkanManager::Get().VkDevice().handle(), shadowMapSampler, nullptr);
	}
	if(finalShadowMapImage._image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(VulkanManager::Get().VkAllocator(), finalShadowMapImage._image, finalShadowMapImage._allocation);
	}
	if(finalShadowMapImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(VulkanManager::Get().VkDevice().handle(), finalShadowMapImageView, nullptr);
	}
}

void Renderer::LightingManager::LightResources::cleanup()
{
	if(lightBuffer._buffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(VulkanManager::Get().VkAllocator(), lightBuffer._buffer, lightBuffer._allocation);
	}
	if(lightPropertiesBuffer._buffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(VulkanManager::Get().VkAllocator(), lightPropertiesBuffer._buffer, lightPropertiesBuffer._allocation);
	}
	VulkanManager::Get().VkDescriptorSetManager().freeDescriptorSet(LightDescriptorSet);
}

void Renderer::LightingManager::FrameData::clear()
{
	lights.clear();
	lightData.clear();
	blockers.clear();
}

void Renderer::LightingManager::initialize(Renderer* renderr)
{
	m_renderer = renderr;
	createDescriptorSetLayouts();
	createShadowMapResources();
	initializeFrameResources();
}

void Renderer::LightingManager::cleanup()
{
	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; i++) {
		frameStates[i].shadowMapResources.cleanup();
		frameStates[i].lightResources.cleanup();
		frameStates[i].shadowCasterResources.blockerBuffer.cleanup();
		frameStates[i].lights.clear();
		frameStates[i].lightData.clear();
		frameStates[i].blockers.clear();
		VulkanManager::Get().VkDescriptorSetManager().freeDescriptorSet(frameStates[i].computeResources.shadowRefinement);
	}
	if(lightingPassDescriptorLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(), lightingPassDescriptorLayout, nullptr);
	}
	if(shadowMapRefinementLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(), shadowMapRefinementLayout, nullptr);
	}
	/*if(blockerDescriptorLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(), blockerDescriptorLayout, nullptr);
	}*/
}

void Renderer::LightingManager::addLight(const LightComponent& light, uint32_t frameIndex) {
	// Early exit if light is disabled
	if(!light.state.enabled) {
		return;
	}

	const auto& transform = ecs::GetEntityTransform(&light);
	const float worldRotation = transform.GetWorldRotation();
	const glm::vec2 worldPosition = transform.GetWorldPosition();

	// ----- STAGE 1: Quick coarse culling -----
	// Get camera parameters
	glm::vec2 cameraPosition = ST<CameraController>::Get()->GetPosition();
	float zoom = ST<CameraController>::Get()->GetZoom();

	// Calculate conservative viewport bounds in world space with large buffer zone
	float worldViewWidth = m_renderer->m_viewport.width / zoom;
	float worldViewHeight = std::abs(m_renderer->m_viewport.height) / zoom;
	float halfWidth = worldViewWidth * 0.6f;  // 50% buffer + 10% original buffer
	float halfHeight = worldViewHeight * 0.6f;

	// Effective world-space bounds
	float minX = cameraPosition.x - halfWidth;
	float maxX = cameraPosition.x + halfWidth;
	float minY = cameraPosition.y - halfHeight;
	float maxY = cameraPosition.y + halfHeight;

	// Calculate effective light radius based on light type
	float effectiveRadius = light.radius;
	if(light.state.castShadows) {
		effectiveRadius *= 3.0f;

		// For spot lights with shadows, check along cast direction
		if(light.state.isSpot) {
			float angle = glm::radians(worldRotation);
			glm::vec2 direction(cos(angle), sin(angle));
			glm::vec2 extendedPos = worldPosition + direction * effectiveRadius;

			// Check both light position and extended position
			bool lightVisible = (worldPosition.x + effectiveRadius >= minX &&
													 worldPosition.x - effectiveRadius <= maxX &&
													 worldPosition.y + effectiveRadius >= minY &&
													 worldPosition.y - effectiveRadius <= maxY);

			bool extendedVisible = (extendedPos.x + effectiveRadius >= minX &&
															extendedPos.x - effectiveRadius <= maxX &&
															extendedPos.y + effectiveRadius >= minY &&
															extendedPos.y - effectiveRadius <= maxY);

			if(!lightVisible && !extendedVisible) {
				return; // Reject if neither is visible
			}
		}
		else {
			// Point light with shadows
			if(worldPosition.x + effectiveRadius < minX ||
				 worldPosition.x - effectiveRadius > maxX ||
				 worldPosition.y + effectiveRadius < minY ||
				 worldPosition.y - effectiveRadius > maxY) {
				return;
			}
		}
	}
	else {
		// Regular light without shadows
		if(worldPosition.x + effectiveRadius < minX ||
			 worldPosition.x - effectiveRadius > maxX ||
			 worldPosition.y + effectiveRadius < minY ||
			 worldPosition.y - effectiveRadius > maxY) {
			return;
		}
	}

	// Rest of the original function remains the same
	uint32_t flags = 0;
	flags |= light.state.enabled ? 1u : 0u;
	flags |= light.state.castShadows ? 1u << 1 : 0u;
	flags |= light.state.isSpot ? 1u << 2 : 0u;

	LightData shadowData{
			.position = worldPosition,
			.lightAngle = glm::radians(worldRotation),
			.lightConeAngle = light.state.isSpot ?
					light.getConeAngleRadians() * 0.5f :
					glm::pi<float>()
	};

	GPULightProperties renderData{
			.color = light.color,
			.intensity = light.intensity,
			.radius = light.radius,
			.innerRadius = light.innerRadius,
			.distanceFalloff = light.falloffExponent,
			.coneFalloff = light.coneFalloff,
			.flags = flags
	};

	frameStates[frameIndex].lights.emplace_back(renderData);
	frameStates[frameIndex].lightData.emplace_back(shadowData);
}

void Renderer::LightingManager::addBlocker(const Transform& transform, uint32_t frameIndex) {
	glm::vec2 position = transform.GetWorldPosition();
	glm::vec2 scale = transform.GetWorldScale();
	float rad = -glm::radians(transform.GetWorldRotation());
	glm::vec2 halfScale = scale * 0.5f;

	// Generate corners in counter-clockwise order
	std::array<glm::vec2, 4> corners;
	glm::mat2 rotMat(
		std::cos(rad), -std::sin(rad),
		std::sin(rad), std::cos(rad)
	);

	// Generate corners in deterministic order
	corners[0] = position + rotMat * glm::vec2(-halfScale.x, -halfScale.y); // Bottom-left
	corners[1] = position + rotMat * glm::vec2(halfScale.x, -halfScale.y);  // Bottom-right
	corners[2] = position + rotMat * glm::vec2(halfScale.x, halfScale.y);   // Top-right
	corners[3] = position + rotMat * glm::vec2(-halfScale.x, halfScale.y);  // Top-left

	auto& blockers = frameStates[frameIndex].blockers;
	// Ensure deterministic vertex order for each line segment
	blockers.emplace_back(ShadowCaster{ corners[0], corners[1] }); // Bottom
	blockers.emplace_back(ShadowCaster{ corners[1], corners[2] }); // Right
	blockers.emplace_back(ShadowCaster{ corners[2], corners[3] }); // Top
	blockers.emplace_back(ShadowCaster{ corners[3], corners[0] }); // Left
}


void Renderer::LightingManager::updateLightingData(uint32_t frameIndex)
{
	auto& lightResources = frameStates[frameIndex].lightResources;
	auto& lightData = frameStates[frameIndex].lightData;
	auto& lightProps = frameStates[frameIndex].lights;
	auto& shadowCasterResources = frameStates[frameIndex].shadowCasterResources;
	auto& splitCasters = frameStates[frameIndex].blockers;

	// Create vectors for different priority levels
	std::vector<size_t> highPriorityLights;
	std::vector<std::pair<size_t, float>> standardPriorityLights;

	// Get camera parameters
	glm::vec2 cameraPosition = ST<CameraController>::Get()->GetPosition();
	float zoom = ST<CameraController>::Get()->GetZoom();

	// Calculate more generous viewport bounds to include off-screen influences
	float worldViewWidth = m_renderer->m_viewport.width / zoom;
	float worldViewHeight = std::abs(m_renderer->m_viewport.height) / zoom;
	float visibilityBuffer = 0.75f;  // More generous 75% buffer instead of 55%
	float influenceBuffer = 1.25f;   // Extra buffer for light influence

	float halfVisibleWidth = worldViewWidth * visibilityBuffer;
	float halfVisibleHeight = worldViewHeight * visibilityBuffer;
	float halfInfluenceWidth = worldViewWidth * influenceBuffer;
	float halfInfluenceHeight = worldViewHeight * influenceBuffer;

	// Viewport bounds for visible checking
	float visMinX = cameraPosition.x - halfVisibleWidth;
	float visMaxX = cameraPosition.x + halfVisibleWidth;
	float visMinY = cameraPosition.y - halfVisibleHeight;
	float visMaxY = cameraPosition.y + halfVisibleHeight;

	// Larger bounds for influence checking
	float infMinX = cameraPosition.x - halfInfluenceWidth;
	float infMaxX = cameraPosition.x + halfInfluenceWidth;
	float infMinY = cameraPosition.y - halfInfluenceHeight;
	float infMaxY = cameraPosition.y + halfInfluenceHeight;

	// Minimum screen-space radius to prevent disappearing when zoomed in
	float minVisibleRadius = 5.0f / zoom;

	// Get absolute target maximum light count or a dynamic percentage
	size_t targetMaxLights = std::min(static_cast<uint32_t>(lightData.size()), MAX_ACTIVE_LIGHTS);
	// If we have fewer lights than the max, we can still cull less important ones
	if(lightData.size() < MAX_ACTIVE_LIGHTS) {
		// Keep at least 75% of available lights if we're under the maximum
		targetMaxLights = std::max(targetMaxLights,
															 static_cast<size_t>(lightData.size() * 0.75f));
	}

	for(size_t i = 0; i < lightData.size(); ++i) {
		const auto& light = lightProps[i];
		const auto& data = lightData[i];

		// Ensure lights don't disappear at high zoom levels
		float effectiveRadius = std::max(light.radius, minVisibleRadius);

		// Basic importance calculation
		float distanceToCamera = glm::distance(data.position, cameraPosition);
		float brightnessScore = light.intensity * glm::length(light.color);

		// Check visibility (with generous bounds)
		bool isVisible = (data.position.x + effectiveRadius >= visMinX &&
											data.position.x - effectiveRadius <= visMaxX &&
											data.position.y + effectiveRadius >= visMinY &&
											data.position.y - effectiveRadius <= visMaxY);

		// Check if the light might influence the visible area even if not directly visible
		bool hasInfluence = (data.position.x + effectiveRadius >= infMinX &&
												 data.position.x - effectiveRadius <= infMaxX &&
												 data.position.y + effectiveRadius >= infMinY &&
												 data.position.y - effectiveRadius <= infMaxY);

		// For spot lights, also check in cast direction
		if(!isVisible && (light.flags & LightFlags::IsSpot)) {
			float castDistance = light.radius * 2.0f;
			glm::vec2 castDir(cos(data.lightAngle), sin(data.lightAngle));
			glm::vec2 castPos = data.position + castDir * castDistance;

			// Check if the spot light beam might be visible
			isVisible = (castPos.x + effectiveRadius >= visMinX &&
									 castPos.x - effectiveRadius <= visMaxX &&
									 castPos.y + effectiveRadius >= visMinY &&
									 castPos.y - effectiveRadius <= visMaxY);

			// Check if the spot light beam might influence visible area
			if(!isVisible && !hasInfluence) {
				hasInfluence = (castPos.x + effectiveRadius >= infMinX &&
												castPos.x - effectiveRadius <= infMaxX &&
												castPos.y + effectiveRadius >= infMinY &&
												castPos.y - effectiveRadius <= infMaxY);
			}
		}

		// Calculate importance score
		float importance = brightnessScore * (1.0f / (1.0f + distanceToCamera * 0.1f));

		// Apply multipliers for special light types
		if(light.flags & LightFlags::CastShadows) {
			importance *= 1.5f;
			// Shadow-casting lights have wider influence
			if(!isVisible && hasInfluence) {
				importance *= 1.3f;  // Boost importance of shadow casters in influence zone
			}
		}
		if(light.flags & LightFlags::IsSpot) {
			importance *= 1.2f;
		}

		// Prioritize based on visibility and importance
		if(isVisible) {
			// Always keep visible lights
			highPriorityLights.push_back(i);
		}
		else if(hasInfluence) {
			// Lights in influence zone go to standard priority with boosted score
			standardPriorityLights.emplace_back(i, importance * 1.2f);
		}
		else {
			// Non-visible, non-influencing lights go into standard priority
			standardPriorityLights.emplace_back(i, importance);
		}
	}

	// Sort non-visible lights by importance score
	std::ranges::sort(standardPriorityLights,
										[](const auto& a, const auto& b) { return a.second > b.second; });

	// Prepare culled vectors
	std::vector<LightData> culledLightData;
	std::vector<GPULightProperties> culledLightProps;
	culledLightData.reserve(targetMaxLights);
	culledLightProps.reserve(targetMaxLights);

	// First add all visible lights (high priority)
	size_t remainingSlots = targetMaxLights;
	for(size_t i = 0; i < highPriorityLights.size() && remainingSlots > 0; ++i) {
		size_t idx = highPriorityLights[i];
		culledLightData.push_back(lightData[idx]);
		culledLightProps.push_back(lightProps[idx]);
		remainingSlots--;
	}

	// Then add most important non-visible lights
	for(size_t i = 0; i < standardPriorityLights.size() && remainingSlots > 0; ++i) {
		size_t idx = standardPriorityLights[i].first;
		culledLightData.push_back(lightData[idx]);
		culledLightProps.push_back(lightProps[idx]);
		remainingSlots--;
	}

	// Replace original vectors with culled ones
	lightData = std::move(culledLightData);
	lightProps = std::move(culledLightProps);
	VmaAllocationInfo lightAllocInfo{}, propsAllocInfo{};
	vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(),
											 lightResources.lightBuffer._allocation, &lightAllocInfo);
	vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(),
											 lightResources.lightPropertiesBuffer._allocation, &propsAllocInfo);

	// Zero-initialize entire buffers
	memset(lightAllocInfo.pMappedData, 0, sizeof(LightData) * MAX_ACTIVE_LIGHTS);
	memset(propsAllocInfo.pMappedData, 0, sizeof(GPULightProperties) * MAX_ACTIVE_LIGHTS);

	// Direct memory transfer of pre-initialized active entries
	if(!lightData.empty()) {
		memcpy(lightAllocInfo.pMappedData,
					 lightData.data(),
					 sizeof(LightData) * lightData.size());
	}

	if(!lightProps.empty()) {
		memcpy(propsAllocInfo.pMappedData,
					 lightProps.data(),
					 sizeof(GPULightProperties) * lightProps.size());
	}

	VmaAllocationInfo blockerAllocInfo{};
	vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(),
											 shadowCasterResources.blockerBuffer._allocation, &blockerAllocInfo);

	// Zero-initialize buffer
	memset(blockerAllocInfo.pMappedData, 0, sizeof(ShadowCaster) * MAX_ACTIVE_BLOCKERS);

	// Direct memory transfer of pre-initialized active entries
	const size_t uploadSize = (std::min)(
		splitCasters.size() * sizeof(ShadowCaster),
		MAX_ACTIVE_BLOCKERS * sizeof(ShadowCaster)
		);

	if(!splitCasters.empty()) {
		memcpy(blockerAllocInfo.pMappedData,
					 splitCasters.data(),
					 uploadSize);
	}

	// Store the count of active vertices for rendering
	shadowCasterResources.activeVertexCount =
		static_cast<uint32_t>(splitCasters.size());  // Two vertices per segment
}
void Renderer::LightingManager::createDescriptorSetLayouts()
{
	// Combined lighting pass descriptor layout (Set 1)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			{
				// Shadow map sampler
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			},
			{
				// Light data buffer
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			},
			{
				// Light properties buffer
				.binding = 2,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			}
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data()
		};

		vkCreateDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(),
																&layoutInfo, nullptr, &lightingPassDescriptorLayout);  // Rename to reflect purpose
	}

	{
		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
			},
			{
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
			}
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data()
		};

		vkCreateDescriptorSetLayout(VulkanManager::Get().VkDevice().handle(),
																&layoutInfo, nullptr, &shadowMapRefinementLayout);
	}
}

void Renderer::LightingManager::createShadowMapResources() {
	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; i++) {
		auto& resources = frameStates[i].shadowMapResources;
		{
			VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(
				VK_FORMAT_R32_SFLOAT,  // Changed from D32_SFLOAT
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |  // Changed from DEPTH_STENCIL
				VK_IMAGE_USAGE_SAMPLED_BIT,
				{ SHADOW_MAP_WIDTH, MAX_ACTIVE_LIGHTS, 1 }
			);

			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			allocInfo.priority = 1.0f;



			vmaCreateImage(VulkanManager::Get().VkAllocator(),
										 &imageInfo, &allocInfo,
										 &resources.shadowMapImage._image,
										 &resources.shadowMapImage._allocation,
										 nullptr);

			// Modified image view for color attachment
			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = resources.shadowMapImage._image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = VK_FORMAT_R32_SFLOAT,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,  // Changed from DEPTH_BIT
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			vkCreateImageView(VulkanManager::Get().VkDevice().handle(),
												&viewInfo, nullptr,
												&resources.shadowMapImageView);
		}


		// Final shadow map (360 degree normalized output)
		{
			VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(
				SHADOW_MAP_FORMAT,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				{ SHADOW_MAP_WIDTH, MAX_ACTIVE_LIGHTS, 1 }  // 360-degree final coverage
			);

			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

			vmaCreateImage(VulkanManager::Get().VkAllocator(),
										 &imageInfo, &allocInfo,
										 &resources.finalShadowMapImage._image,
										 &resources.finalShadowMapImage._allocation,
										 nullptr);

			// Create image view for final shadow map
			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = resources.finalShadowMapImage._image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = SHADOW_MAP_FORMAT,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			vkCreateImageView(VulkanManager::Get().VkDevice().handle(),
												&viewInfo, nullptr,
												&resources.finalShadowMapImageView);
		}

		// Create sampler for shadow map sampling in fragment shade
		VkSamplerCreateInfo samplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.compareEnable = VK_FALSE,  // Removed depth comparison
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			.unnormalizedCoordinates = VK_FALSE
		};

		vkCreateSampler(VulkanManager::Get().VkDevice().handle(),
										&samplerInfo, nullptr,
										&resources.shadowMapSampler);

		ST<Engine>::Get()->_vulkan->immediateSubmit([&](VkCommandBuffer cmd) {
			// Initialize shadow map for depth attachment
			VulkanHelper::TransitionImage(cmd,
																		resources.shadowMapImage._image,
																		VK_IMAGE_LAYOUT_UNDEFINED,
																		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // Changed initial layout
																		VK_IMAGE_ASPECT_COLOR_BIT);

			// Initialize final shadow map for sampling
			VulkanHelper::TransitionImage(cmd,
																		resources.finalShadowMapImage._image,
																		VK_IMAGE_LAYOUT_UNDEFINED,
																		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // Changed from GENERAL
																		VK_IMAGE_ASPECT_COLOR_BIT);
		});
	}
}

void Renderer::LightingManager::initializeFrameResources() {
	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; i++) {
		auto& frameData = frameStates[i];

		// Pre-allocate vector capacity for deterministic memory behavior
		frameData.lights.reserve(MAX_ACTIVE_LIGHTS);
		frameData.lightData.reserve(MAX_ACTIVE_LIGHTS);
		frameData.blockers.reserve(MAX_ACTIVE_BLOCKERS);

		// Initialize descriptor sets for shadow refinement compute pass
		frameData.computeResources.shadowRefinement =
			VulkanManager::Get().VkDescriptorSetManager()
			.createDescriptorSet(shadowMapRefinementLayout,
													 DescriptorSetManager::SetType::Static);

		// Initialize descriptor sets for final lighting pass
		frameData.lightResources.LightDescriptorSet =
			VulkanManager::Get().VkDescriptorSetManager()
			.createDescriptorSet(lightingPassDescriptorLayout,
													 DescriptorSetManager::SetType::Static);

		VkBufferCreateInfo buffer_info{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		};

		VmaAllocationCreateInfo allocInfo{
			.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		};
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		buffer_info.size = sizeof(LightData) * MAX_ACTIVE_LIGHTS;

		// Create minimal placeholder buffers
		vmaCreateBuffer(VulkanManager::Get().VkAllocator(),
										&buffer_info,
										&allocInfo,
										&frameData.lightResources.lightBuffer._buffer,
										&frameData.lightResources.lightBuffer._allocation,
										nullptr);

		buffer_info.size = sizeof(GPULightProperties) * MAX_ACTIVE_LIGHTS;

		vmaCreateBuffer(VulkanManager::Get().VkAllocator(),
										&buffer_info,
										&allocInfo,
										&frameData.lightResources.lightPropertiesBuffer._buffer,
										&frameData.lightResources.lightPropertiesBuffer._allocation,
										nullptr);

		buffer_info.size = sizeof(ShadowCaster) * MAX_ACTIVE_BLOCKERS;

		vmaCreateBuffer(VulkanManager::Get().VkAllocator(),
										&buffer_info,
										&allocInfo,
										&frameData.shadowCasterResources.blockerBuffer._buffer,
										&frameData.shadowCasterResources.blockerBuffer._allocation,
										nullptr);

		// 2. Update Shadow Refinement Compute Pass Descriptors (second compute pass)
		{
			std::vector<VkWriteDescriptorSet> writes;

			// Input shadow map (binding 0)
			VkDescriptorImageInfo inputShadowMapInfo{
				.sampler = frameData.shadowMapResources.shadowMapSampler,
				.imageView = frameData.shadowMapResources.shadowMapImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			// Output refined shadow map (binding 1)
			VkDescriptorImageInfo outputShadowMapInfo{
				.imageView = frameData.shadowMapResources.finalShadowMapImageView,
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL
			};

			writes.push_back(VkInit::WriteDescriptorImage(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(frameData.computeResources.shadowRefinement),
				&inputShadowMapInfo, 0));

			writes.push_back(VkInit::WriteDescriptorImage(
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(frameData.computeResources.shadowRefinement),
				&outputShadowMapInfo, 1));

			VulkanManager::Get().VkDescriptorSetManager()
				.updateDescriptorSet(frameData.computeResources.shadowRefinement, writes);
		}

		// 3. Update Final Lighting Pass Descriptors (fragment shader)
		{
			std::vector<VkWriteDescriptorSet> writes;

			// Final shadow map sampler (binding 0)
			VkDescriptorImageInfo shadowMapInfo{
				.sampler = frameData.shadowMapResources.shadowMapSampler,
				.imageView = frameData.shadowMapResources.finalShadowMapImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			// Light data buffer - uses uniform buffer access (binding 1)
			VkDescriptorBufferInfo lightBufferInfo{
				.buffer = frameData.lightResources.lightBuffer._buffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE
			};

			// Light properties buffer (binding 2)
			VkDescriptorBufferInfo lightPropsBufferInfo{
				.buffer = frameData.lightResources.lightPropertiesBuffer._buffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE
			};

			writes.push_back(VkInit::WriteDescriptorImage(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(frameData.lightResources.LightDescriptorSet),
				&shadowMapInfo, 0));

			writes.push_back(VkInit::WriteDescriptorBuffer(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(frameData.lightResources.LightDescriptorSet),
				&lightBufferInfo, 1));

			writes.push_back(VkInit::WriteDescriptorBuffer(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(frameData.lightResources.LightDescriptorSet),
				&lightPropsBufferInfo, 2));

			VulkanManager::Get().VkDescriptorSetManager()
				.updateDescriptorSet(frameData.lightResources.LightDescriptorSet, writes);
		}
	}
}

void Renderer::uploadQuad()
{
	VkDeviceSize const bufferSize = sizeof(m_quad.vertices[0]) * m_quad.vertices.size();

	// Create staging buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingAllocCreateInfo = {};
	stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer{};

	vmaCreateBuffer(VulkanManager::Get().VkAllocator(), &stagingBufferInfo, &stagingAllocCreateInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, &stagingAllocInfo);

	// Copy data to staging buffer
	memcpy(stagingAllocInfo.pMappedData, m_quad.vertices.data(), bufferSize);

	// Create vertex buffer (GPU only)
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = bufferSize;
	vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo vertexAllocInfo = {};
	vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vertexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vertexAllocInfo.priority = 1.0f;

	vmaCreateBuffer(VulkanManager::Get().VkAllocator(), &vertexBufferInfo, &vertexAllocInfo, &m_quad.vertexBuffer._buffer, &m_quad.vertexBuffer._allocation, nullptr);

	// Copy data from staging buffer to vertex buffer
	m_context->immediateSubmit([bufferSize, stagingBuffer, this](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer._buffer, m_quad.vertexBuffer._buffer, 1, &copy);

		// Add a buffer memory barrier to ensure the copy is complete before vertex buffer is used
		VkBufferMemoryBarrier2 bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
		bufferBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		bufferBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		bufferBarrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		bufferBarrier.buffer = m_quad.vertexBuffer._buffer;
		bufferBarrier.offset = 0;
		bufferBarrier.size = VK_WHOLE_SIZE;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.bufferMemoryBarrierCount = 1;
		dependencyInfo.pBufferMemoryBarriers = &bufferBarrier;

		vkCmdPipelineBarrier2(cmd, &dependencyInfo);
	});

	// Cleanup staging buffer
	stagingBuffer.cleanup();
}

void Renderer::initializeQuad()
{
	m_quad.vertices.resize(6);

	// First triangle
	m_quad.vertices[0].position = { -0.5f, 0.5f };
	m_quad.vertices[0].texCoord = { 0.0f, 0.0f };

	m_quad.vertices[1].position = { 0.5f, 0.5f };
	m_quad.vertices[1].texCoord = { 1.0f, 0.0f };

	m_quad.vertices[2].position = { -0.5f, -0.5f };
	m_quad.vertices[2].texCoord = { 0.0f, 1.0f };

	// Second triangle
	m_quad.vertices[3].position = { 0.5f, 0.5f };
	m_quad.vertices[3].texCoord = { 1.0f, 0.0f };

	m_quad.vertices[4].position = { 0.5f, -0.5f };
	m_quad.vertices[4].texCoord = { 1.0f, 1.0f };

	m_quad.vertices[5].position = { -0.5f, -0.5f };
	m_quad.vertices[5].texCoord = { 0.0f, 1.0f };

	uploadQuad();
}

Renderer::Target Renderer::createBloomTarget(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage) {
	Target target;

	// Create image
	VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(
		format,
		usage,
		extent
	);

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vmaCreateImage(VulkanManager::Get().VkAllocator(), &imageInfo, &allocInfo,
								 &target.image._image,
								 &target.image._allocation,
								 nullptr);

	// Create image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = target.image._image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCreateImageView(VulkanManager::Get().VkDevice().handle(), &viewInfo, nullptr, &target.view);

	// Create sampler with linear filtering for bloom
	VkSamplerCreateInfo samplerInfo = VkInit::SamplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	vkCreateSampler(VulkanManager::Get().VkDevice().handle(), &samplerInfo, nullptr, &target.sampler);

	target.format = format;

	// Create and update descriptor set
	target.descriptorHandle = VulkanManager::Get().VkDescriptorSetManager()
		.createDescriptorSet(m_renderTargetLayout, DescriptorSetManager::SetType::Static);

	VkDescriptorImageInfo descImageInfo{
			.sampler = target.sampler,
			.imageView = target.view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	std::vector<VkWriteDescriptorSet> write;
	write.push_back({
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &descImageInfo
									});

	auto descriptorSet = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(target.descriptorHandle);
	write[0].dstSet = descriptorSet;

	VulkanManager::Get().VkDescriptorSetManager().updateDescriptorSet(target.descriptorHandle, write);

	return target;
}

void Renderer::resizeAllTargets() {
	// Release existing resources for both main and lighting targets
	vkDeviceWaitIdle(VulkanManager::Get().VkDevice().handle());
	for(auto& m_renderTarget : m_renderTargets) {
		m_renderTarget.cleanup();
	}
	for(auto& target : m_lightingTargets)
	{
		target.cleanup();
	}
	for(auto& bloomRes : m_bloomTargets) {
		bloomRes.cleanup();
	}
	// Determine render extent based on configuration
#ifdef IMGUI_ENABLED
	VkExtent2D SCextent = ST<Engine>::Get()->_viewportExtent;
#else
	VkExtent2D SCextent = VulkanManager::Get().VkSwapchain().extent();
#endif
	VkExtent3D extent{ SCextent.width, SCextent.height, 1 };
	VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;

	// Define bloom extent (half resolution)
	VkExtent2D bloomExtent = { SCextent.width / 2, SCextent.height / 2 };
	VkExtent3D bloomExtent3D = { bloomExtent.width, bloomExtent.height, 1 };
	VkFormat bloomFormat = colorFormat;
	VkImageUsageFlags bloomUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	// Descriptor set layout creation for render target sampling
	VkDescriptorSetLayoutBinding samplerBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &samplerBinding
	};

	if(m_renderTargetLayout == VK_NULL_HANDLE) {
		if(vkCreateDescriptorSetLayout(
			VulkanManager::Get().VkDevice().handle(),
			&layoutInfo,
			nullptr,
			&m_renderTargetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render target descriptor set layout!");
		}
	}

	// Create per-frame resources
	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; i++) {
		// Main render target creation
		{
			VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(
				colorFormat,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				extent
			);

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			vmaCreateImage(VulkanManager::Get().VkAllocator(), &imageInfo, &allocInfo,
										 &m_renderTargets[i].image._image,
										 &m_renderTargets[i].image._allocation,
										 nullptr);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_renderTargets[i].image._image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = colorFormat;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(VulkanManager::Get().VkDevice().handle(), &viewInfo, nullptr, &m_renderTargets[i].RenderView);

			viewInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;
#ifdef IMGUI_ENABLED
			vkCreateImageView(VulkanManager::Get().VkDevice().handle(), &viewInfo, nullptr, &m_renderTargets[i].ImGuiView);
#endif
			VkSamplerCreateInfo samplerInfo = VkInit::SamplerCreateInfo(VK_FILTER_NEAREST);
			vkCreateSampler(VulkanManager::Get().VkDevice().handle(), &samplerInfo, nullptr, &m_renderTargets[i].sampler);

			m_renderTargets[i].format = colorFormat;
			//
			// Create descriptor set for render target
			m_renderTargets[i].descriptorHandle = VulkanManager::Get().VkDescriptorSetManager()
				.createDescriptorSet(m_renderTargetLayout, DescriptorSetManager::SetType::Static);

			VkDescriptorImageInfo descimageInfo{
				.sampler = m_renderTargets[i].sampler,
				.imageView = m_renderTargets[i].RenderView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			std::vector<VkWriteDescriptorSet> writes = {
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &descimageInfo
				}
			};

			// Get the actual VkDescriptorSet handle from the DescriptorSetHandle
			auto descriptorSet = VulkanManager::Get().VkDescriptorSetManager()
				.getDescriptorSet(m_renderTargets[i].descriptorHandle);
			writes[0].dstSet = descriptorSet;  // Set the actual descriptor set handle

			VulkanManager::Get().VkDescriptorSetManager().updateDescriptorSet(m_renderTargets[i].descriptorHandle, writes);
		}
		VkFormat lightingFormat = VulkanManager::Get().VkSwapchain().imageFormat();
		{
			VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(
				lightingFormat,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				extent
			);

			// Enable concurrent access if needed
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			vmaCreateImage(VulkanManager::Get().VkAllocator(),
										 &imageInfo, &allocInfo,
										 &m_lightingTargets[i].image._image,
										 &m_lightingTargets[i].image._allocation,
										 nullptr);

			// Create view with proper HDR format
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_lightingTargets[i].image._image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = lightingFormat;
			viewInfo.subresourceRange = {
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1, 0, 1
			};

			vkCreateImageView(VulkanManager::Get().VkDevice().handle(),
												&viewInfo, nullptr,
												&m_lightingTargets[i].view);

			// Create HDR-aware sampler
			VkSamplerCreateInfo samplerInfo = VkInit::SamplerCreateInfo(VK_FILTER_LINEAR);
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			vkCreateSampler(VulkanManager::Get().VkDevice().handle(),
											&samplerInfo, nullptr,
											&m_lightingTargets[i].sampler);

			m_lightingTargets[i].format = lightingFormat;

			// Create descriptor set for lighting target
			m_lightingTargets[i].descriptorHandle =
				VulkanManager::Get().VkDescriptorSetManager()
				.createDescriptorSet(m_renderTargetLayout,
														 DescriptorSetManager::SetType::Static);

			// Update descriptor set
			VkDescriptorImageInfo descImageInfo{
				.sampler = m_lightingTargets[i].sampler,
				.imageView = m_lightingTargets[i].view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			std::vector<VkWriteDescriptorSet> write;
			write.push_back(
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = VulkanManager::Get().VkDescriptorSetManager()
								.getDescriptorSet(m_lightingTargets[i].descriptorHandle),
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &descImageInfo
				});

			VulkanManager::Get().VkDescriptorSetManager()
				.updateDescriptorSet(m_lightingTargets[i].descriptorHandle,
														 write);
		}
		m_bloomTargets[i].bloomTextureA = createBloomTarget(bloomExtent3D, bloomFormat, bloomUsage);
		m_bloomTargets[i].bloomTextureB = createBloomTarget(bloomExtent3D, bloomFormat, bloomUsage);
		m_bloomTargets[i].bloomTextureC = createBloomTarget(bloomExtent3D, bloomFormat, bloomUsage);
	}

	// Perform initial layout transitions
	ST<Engine>::Get()->_vulkan->immediateSubmit([&](VkCommandBuffer cmd) {
		for(auto& image : m_renderTargets) {
			VulkanHelper::TransitionImage(
				cmd,
				image.image._image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}
		for(auto& target : m_lightingTargets) {
			VulkanHelper::TransitionImage(
				cmd,
				target.image._image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}
		for(auto& bloomRes : m_bloomTargets) {
			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureA.image._image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureB.image._image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			VulkanHelper::TransitionImage(
				cmd,
				bloomRes.bloomTextureC.image._image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}
	});

	for(uint32_t i = 0; i < Constant::FRAME_OVERLAP; ++i) {
#ifdef IMGUI_ENABLED
		if(m_context->m_viewportDescriptorSets[i] != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(m_context->m_viewportDescriptorSets[i]);
		}
		m_context->m_viewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
			m_renderTargets[i].sampler,
			m_renderTargets[i].ImGuiView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}
#else
}

#endif

}

void Renderer::updateCameraBuffer() {
	if(!projection_dirty) return;
	m_screenData.aspectRatio = m_viewport.x / m_viewport.y;
	m_screenData.screenSize = { m_viewport.x, m_viewport.y };
	SceneUBO ubo{};
	// Create a view matrix that centers the camera
	glm::mat4 view = glm::mat4(1.0f);
	// Apply zoom
	view = scale(view, glm::vec3(m_zoom, m_zoom, 1.0f));

	view = rotate(view, glm::radians(-m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	// Move the world in the opposite direction of the camera
	view = translate(view, glm::vec3(-m_cameraPosition, 0.0f));
	// Combine projection and view matrices
	ubo.projection = m_projectionMatrix * view;
	VmaAllocation allocation = m_context->getCurrentFrame().cameraBuffer._allocation;
	VmaAllocationInfo allocInfo;
	vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(), allocation, &allocInfo);
	memcpy(allocInfo.pMappedData, &ubo, sizeof(SceneUBO));
	projection_dirty = false;
}

template <typename InstanceType, typename ZDepthFunc >
void Renderer::updateInstanceBuffer(std::vector<InstanceType>& instances, AllocatedBuffer& instanceBuffer,
																		VkDeviceSize& bufferSize, ZDepthFunc getZDepth
) const {
	// Sort instances by Z depth using the provided accessor function
	std::ranges::stable_sort(instances, [&getZDepth](const auto& a, const auto& b) {
		return getZDepth(a) < getZDepth(b);
	});

	VkDeviceSize requiredSize = sizeof(InstanceType) * instances.size();
	VmaAllocationInfo mapped_alloc_info{};

	if(instanceBuffer._buffer == VK_NULL_HANDLE || requiredSize > bufferSize) {
		// Create or recreate the buffer if it doesn't exist or is too small
		if(instanceBuffer._buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(VulkanManager::Get().VkAllocator(), instanceBuffer._buffer, instanceBuffer._allocation);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = requiredSize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vmaCreateBuffer(VulkanManager::Get().VkAllocator(), &bufferInfo, &allocInfo,
										&instanceBuffer._buffer, &instanceBuffer._allocation, &mapped_alloc_info);

		bufferSize = requiredSize;
	}
	else {
		vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(), instanceBuffer._allocation, &mapped_alloc_info);
	}

	if(mapped_alloc_info.pMappedData != nullptr) {
		memcpy(mapped_alloc_info.pMappedData, instances.data(), requiredSize);
	}
}

void Renderer::updateTextureBuffer(std::vector<SpriteInstanceData>& sprites, AllocatedBuffer& instanceBuffer, VkDeviceSize& _instanceBufferSize) const
{
	updateInstanceBuffer<SpriteInstanceData>(
		sprites,
		instanceBuffer,
		_instanceBufferSize,
		[](const SpriteInstanceData& sprite) { return sprite.model[3][2]; }
	);
}

/*void Renderer::updateTextBuffer() const
{
	VkDeviceSize bufferSize = sizeof(TextInstanceData) * _glyphs.size();
	VmaAllocationInfo mapped_alloc_info{};
	auto& instanceBuffer = m_context->getCurrentFrame().textInstanceBuffer;
	auto& _textBufferSize = m_context->getCurrentFrame()._textBufferSize;
	if (instanceBuffer._buffer == VK_NULL_HANDLE || bufferSize > _textBufferSize) {
		// Create or recreate the buffer if it doesn't exist or is too small
		if (instanceBuffer._buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(VulkanManager::Get().VkAllocator(), instanceBuffer._buffer, instanceBuffer._allocation);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vmaCreateBuffer(VulkanManager::Get().VkAllocator(), &bufferInfo, &allocInfo, &instanceBuffer._buffer, &instanceBuffer._allocation, &mapped_alloc_info);

		_textBufferSize = bufferSize;
	}
	else
	{
		vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(), instanceBuffer._allocation, &mapped_alloc_info);
	}
	if (mapped_alloc_info.pMappedData != nullptr)
	{
		memcpy(mapped_alloc_info.pMappedData, _glyphs.data(), bufferSize);
	}
}*/

void Renderer::updateLineBuffer()
{
	VkDeviceSize bufferSize = sizeof(LineInstanceData) * _lines.size();
	VmaAllocationInfo mapped_alloc_info{};

	auto& lineBuffer = m_context->getCurrentFrame().debugInstanceBuffer;
	auto& _lineBufferSize = m_context->getCurrentFrame()._lineBufferSize;

	if(lineBuffer._buffer == VK_NULL_HANDLE || bufferSize > _lineBufferSize) {
		// Create or recreate the buffer if it doesn't exist or is too small
		if(lineBuffer._buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(VulkanManager::Get().VkAllocator(), lineBuffer._buffer, lineBuffer._allocation);
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vmaCreateBuffer(VulkanManager::Get().VkAllocator(), &bufferInfo, &allocInfo, &lineBuffer._buffer, &lineBuffer._allocation, &mapped_alloc_info);

		_lineBufferSize = bufferSize;
	}
	else
	{
		vmaGetAllocationInfo(VulkanManager::Get().VkAllocator(), lineBuffer._allocation, &mapped_alloc_info);
	}
	if(mapped_alloc_info.pMappedData != nullptr)
	{
		memcpy(mapped_alloc_info.pMappedData, _lines.data(), bufferSize);//
	}
}
bool Renderer::isInViewport(const glm::vec2& position, const glm::vec2& size, float rotation = 0.0f) const {
    // Calculate buffer size
    float bufferX = m_viewport.width * 0.1f;
    float bufferY = std::abs(m_viewport.height) * 0.1f;

		rotation = glm::radians(rotation);
		// Check if the object is within the viewport, including the buffer zone
    // For non-rotated entities, use the original faster AABB method
    if (rotation == 0.0f || fmod(rotation, glm::two_pi<float>()) == 0.0f) {
        // Convert world coordinates to screen coordinates
        glm::vec2 screenPos = (position - m_cameraPosition) * m_zoom + glm::vec2(m_viewport.width / 2, std::abs(m_viewport.height) / 2);
        glm::vec2 screenSize = size * m_zoom;
        // Check if the object is within the viewport, including the buffer zone
        return (screenPos.x + screenSize.x / 2 >= -bufferX &&
                screenPos.x - screenSize.x / 2 <= m_viewport.width + bufferX &&
                screenPos.y + screenSize.y / 2 >= -bufferY &&
                screenPos.y - screenSize.y / 2 <= std::abs(m_viewport.height) + bufferY);
    }
    
    // For rotated entities, calculate a bounding box that encompasses the rotated entity
    
    // Calculate half-size
    float halfWidth = size.x / 2.0f;
    float halfHeight = size.y / 2.0f;
    
    // Calculate the absolute values of sine and cosine of rotation angle
    float cosA = std::abs(std::cos(rotation));
    float sinA = std::abs(std::sin(rotation));
    
    // Calculate the maximum extent of the rotated box (projected onto x and y axes)
    // This creates an AABB that fully contains the rotated box
    float maxExtentX = halfWidth * cosA + halfHeight * sinA;
    float maxExtentY = halfWidth * sinA + halfHeight * cosA;
    
    // Convert world coordinates to screen coordinates
    glm::vec2 screenPos = (position - m_cameraPosition) * m_zoom + glm::vec2(m_viewport.width / 2, std::abs(m_viewport.height) / 2);
    float screenExtentX = maxExtentX * m_zoom;
    float screenExtentY = maxExtentY * m_zoom;
    
    // Check if the enlarged AABB overlaps with the viewport including buffer
    return (screenPos.x + screenExtentX >= -bufferX &&
            screenPos.x - screenExtentX <= m_viewport.width + bufferX &&
            screenPos.y + screenExtentY >= -bufferY &&
            screenPos.y - screenExtentY <= std::abs(m_viewport.height) + bufferY);
}
