#pragma once
/******************************************************************************/
/*!
\file   VulkanContext.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Vulkan Context class that manages the Vulkan Context and the frame in flight. Class that is close to godhood for vulkan.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Vk_Incl.h"

#include "Buffer.h"
#include "CommandManager.h"
#include "DescriptorSetManager.h"
#include "VulkanManager.h"
#include "Instance.h"

struct FrameInFlight
{
	CommandManager::CommandPoolHandle _commandPool;
	CommandManager::CommandBufferHandle _mainCommandBuffer;
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	AllocatedBuffer cameraBuffer; //SCENE UBO
	AllocatedBuffer instanceBuffers; //INSTANCE DATA
	VkDeviceSize _instanceBufferSize{};
	AllocatedBuffer non_lit_instanceBuffer;
	VkDeviceSize _non_lit_instanceBufferSize{};
	/*AllocatedBuffer textInstanceBuffer;
	VkDeviceSize _textBufferSize{}; //TEXT INSTANCE DATA*/
	AllocatedBuffer emissiveGlowBuffer;
	VkDeviceSize _emissiveGlowBufferSize{}; //EMISSIVE GLOW DATA
	AllocatedBuffer debugInstanceBuffer;
	VkDeviceSize _lineBufferSize{}; //DEBUG INSTANCE DATA
	void cleanup();
};

struct SceneUBO
{
	glm::mat4 projection;
};

enum RenderFlags {
    RENDER_FLAG_TEXT = 0x01,        // 0000 0001 - Indicates a text glyph
    RENDER_FLAG_SOLID_COLOR = 0x02, // 0000 0010 - Apply solid color while preserving alpha
		RENDER_FLAG_TRAIL = 0x04,       // 0000 0100 - Indicates a trail
    // Add other flags as needed
};

 struct SpriteInstanceData {
    glm::mat4 model;
    glm::vec4 color;
		glm::vec4 texCoords;
		uint32_t textureIndex;
		uint32_t flags;

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

/*
struct TextInstanceData {
		glm::mat4 model;
    glm::vec4 color;
		glm::vec4 texCoords;
		uint32_t textureIndex;
		//uint32_t effectFlags;

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};
*/

struct TextEffectData
{
	glm::vec4 outline_color;
	glm::vec4 shadow_color;
	glm::vec2 shadow_offset;
	float outline_width;
	float smoothing;
};

struct screenData
{
	glm::vec2 screenSize;
	float aspectRatio;
};

constexpr TextEffectData DEFAULT_EFFECT_DATA = {
    glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // outlineColor (transparent)
    glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // shadowColor (transparent)
    glm::vec2(0.0f, 0.0f),              // shadowOffset (no offset)
    0.0f,                               // outlineWidth (no outline)
    0.09f                                // smoothing (default antialiasing)
};

class VulkanContext {
	friend class Renderer;

	struct UploadContext
	{
		VkFence _uploadFence;
		CommandManager::CommandPoolHandle _commandPool;
		CommandManager::CommandBufferHandle _commandBuffer;
		void cleanup();
	};

	public:
	VulkanContext();
	~VulkanContext();

	
	void init();
	void shutdown() noexcept;

	void beginFrame();
	void endFrame();
	//
	//Interface
	FrameInFlight& getCurrentFrame();
	#ifdef IMGUI_ENABLED
	VkDescriptorSet getViewportDescriptorSet() const;
#endif

	uint32_t getCurrentFrameNumber() const;
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
	bool resized = false;

private:
	UploadContext _uploadContext{};
	bool isInitialized{ false };
	uint32_t _frameNumber{ 0 };
	std::array<FrameInFlight, Constant::FRAME_OVERLAP> _frames{};
	DescriptorSetManager::DescriptorSetHandle cameraDescriptor;
public:
	std::unique_ptr<Renderer> _renderer;
private:
#ifdef IMGUI_ENABLED
	public:
	std::array<VkDescriptorSet, Constant::FRAME_OVERLAP> m_viewportDescriptorSets;
	private:
	VkRenderPass m_imGuiRenderPass;
	std::vector<VkFramebuffer> m_imGuiFramebuffers;

	void initImGui();
	void initImgui_nonDynamic(); // IMGUI really does not like the idea of dynamic rendering, so ImGui windows will be using render passes.
	void recreateImGuiFramebuffers();
#endif
	//Vulkan initialization methods
	void initCommands();
	void initSyncObjs();
	//void initRenderPass();
	//void initFramebuffers();
	void setupSpritePipeline();
	//void setupTextPipeline();
	void setupDebugPipeline();
	void setupLightingPipelines();
	void setupCompositionPipeline();
	void setupPostProcessingPipeline();
	void initDescriptors();

	

	// Vulkan runtime methods
	//void recreateFramebuffers();
	void recreateSwapchain();
	
	//AllocatedBuffer allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;
	// Vulkan cleanup methods
	//void cleanupFramebuffers();
};



