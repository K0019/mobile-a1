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

#include <graphics/pool.h>
#include "vk_util.h"
#include <future>
#include <memory>
#include <vector>

namespace vk
{
  class VulkanContext;

  struct DeviceQueues final
  {
    const static uint32_t INVALID = 0xFFFFFFFF;
    uint32_t graphicsQueueFamilyIndex = INVALID;
    uint32_t computeQueueFamilyIndex = INVALID;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
  };

  struct VulkanBuffer final
  {
    // clang-format off
    [[nodiscard]] uint8_t* getMappedPtr() const { return static_cast<uint8_t*>(mappedPtr_); }

    [[nodiscard]] bool isMapped() const { return mappedPtr_ != nullptr; }

    // clang-format on

    void bufferSubData(const VulkanContext& ctx, size_t offset, size_t size, const void* data);

    void getBufferSubData(const VulkanContext& ctx, size_t offset, size_t size, void* data);

    void flushMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size) const;

    void invalidateMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size) const;

    VkBuffer vkBuffer_ = VK_NULL_HANDLE;
    VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
    VkDeviceAddress vkDeviceAddress_ = 0;
    VkDeviceSize bufferSize_ = 0;
    VkBufferUsageFlags vkUsageFlags_ = 0;
    VkMemoryPropertyFlags vkMemFlags_ = 0;
    void* mappedPtr_ = nullptr;
    bool isCoherentMemory_ = false;
  };

  struct VulkanImage final
  {
    // clang-format off
    [[nodiscard]] bool isSampledImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_SAMPLED_BIT) > 0; }

    [[nodiscard]] bool isStorageImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) > 0; }

    [[nodiscard]] bool isColorAttachment() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) > 0; }

    [[nodiscard]] bool isDepthAttachment() const
    {
      return (vkUsageFlags_ & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) > 0;
    }

    [[nodiscard]] bool isAttachment() const
    {
      return (vkUsageFlags_ & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0;
    }

    // clang-format on

    /*
     * Setting `numLevels` to a non-zero value will override `mipLevels_` value from the original Vulkan image, and can be used to create
     * image views with different number of levels.
     */
    [[nodiscard]] VkImageView createImageView(VkDevice device, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseLevel, uint32_t numLevels = VK_REMAINING_MIP_LEVELS, uint32_t baseLayer = 0, uint32_t numLayers = 1, VkComponentMapping mapping = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY}, const VkSamplerYcbcrConversionInfo* ycbcr = nullptr, const char* debugName = nullptr) const;

    void generateMipmap(VkCommandBuffer commandBuffer) const;

    void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newImageLayout, const VkImageSubresourceRange& subresourceRange) const;

    [[nodiscard]] VkImageAspectFlags getImageAspectFlags() const;

    // framebuffers can render only into one level/layer
    [[nodiscard]] VkImageView getOrCreateVkImageViewForFramebuffer(VulkanContext& ctx, uint8_t level, uint16_t layer);

    [[nodiscard]] static bool isDepthFormat(VkFormat format);

    [[nodiscard]] static bool isStencilFormat(VkFormat format);

    VkImage vkImage_ = VK_NULL_HANDLE;
    VkImageUsageFlags vkUsageFlags_ = 0;
    VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
    VkFormatProperties vkFormatProperties_ = {};
    VkExtent3D vkExtent_ = {0, 0, 0};
    VkImageType vkType_ = VK_IMAGE_TYPE_MAX_ENUM;
    VkFormat vkImageFormat_ = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits vkSamples_ = VK_SAMPLE_COUNT_1_BIT;
    void* mappedPtr_ = nullptr;
    bool isSwapchainImage_ = false;
    bool isOwningVkImage_ = true;
    bool isResolveAttachment = false; // autoset by cmdBeginRendering() for extra synchronization
    uint32_t numLevels_ = 1u;
    uint32_t numLayers_ = 1u;
    bool isDepthFormat_ = false;
    bool isStencilFormat_ = false;
    char debugName_[256] = {0};
    // current image layout
    mutable VkImageLayout vkImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    // precached image views - owned by this VulkanImage
    VkImageView imageView_ = VK_NULL_HANDLE;                      // default view with all mip-levels
    VkImageView imageViewStorage_ = VK_NULL_HANDLE;               // default view with identity swizzle (all mip-levels)
    VkImageView imageViewForFramebuffer_[MAX_MIP_LEVELS][6] = {}; // max 6 faces for cubemap rendering
  };

  struct FrameResources final
  {
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkFence presentFence = VK_NULL_HANDLE;
    uint64_t timelineWaitValue = 0;
  };

  struct SwapchainImage final
  {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    TextureHandle handle = {}; // Handle for the texture pool
  };

  class VulkanSwapchain final
  {
    static constexpr int MAX_SWAPCHAIN_IMAGES = 16;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    public:
      struct InitInfo
      {
        VulkanContext& ctx;
        uint32_t width;
        uint32_t height;
        bool vSync = true;
        VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
      };

      VulkanSwapchain() = default;

      ~VulkanSwapchain();

      // Disable copy/move to prevent resource management issues
      VulkanSwapchain(const VulkanSwapchain&) = delete;

      VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

      VulkanSwapchain(VulkanSwapchain&&) = delete;

      VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

      Result init(const InitInfo& info);

      void deinit();

      // Resource management
      Result initResources(uint32_t& outWidth, uint32_t& outHeight, bool vSync);

      Result reinitResources(uint32_t& outWidth, uint32_t& outHeight, bool vSync);

      void deinitResources();

      // Frame operations
      Result acquireNextImage();

      Result present(VkSemaphore waitSemaphore);

      // Getters
      bool needsRebuild() const { return needRebuild_; }

      VkImage getCurrentVkImage() const;

      VkImageView getCurrentVkImageView() const;

      TextureHandle getCurrentTexture();

      VkSemaphore getCurrentImageAvailableSemaphore() const;

      const VkSurfaceFormatKHR& getSurfaceFormat() const { return surfaceFormat_; }

      uint32_t getSwapchainCurrentImageIndex() const { return currentImageIndex_; }

      uint32_t getNumSwapchainImages() const { return numSwapchainImages_; }

      uint32_t getCurrentFrameIndex() const { return currentFrameIndex_; }

      VkSwapchainKHR getVkSwapchain() const { return swapchain_; }

      // For recreation only
      VkSwapchainKHR releaseNativeHandle();

      VkSurfaceTransformFlagBitsKHR getPreTransform() const { return preTransform_; }

    private:
      // Helper methods
      Result querySwapchainSupport();

      VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;

      VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vSync) const;

      VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t requestedWidth, uint32_t requestedHeight) const;

      VkImageUsageFlags selectUsageFlags() const;

      VkCompositeAlphaFlagBitsKHR selectCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const;

      void createSynchronizationObjects();

      void destroySynchronizationObjects();

      void transitionImagesToPresentLayout();

      // Core Vulkan objects
      VulkanContext* ctx_ = nullptr;
      VkDevice device_ = VK_NULL_HANDLE;
      VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
      VkSurfaceKHR surface_ = VK_NULL_HANDLE;
      VkQueue graphicsQueue_ = VK_NULL_HANDLE;
      uint32_t queueFamilyIndex_ = 0;

      // Swapchain state
      VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
      VkSurfaceFormatKHR surfaceFormat_ = {.format = VK_FORMAT_UNDEFINED};
      VkPresentModeKHR presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
      VkExtent2D extent_ = {0, 0};
      uint32_t acquiredImageIndex_ = UINT32_MAX; // Track which image we actually acquired
      bool hasAcquiredImage_ = false;            // Track if we have a valid acquired image
      bool needRebuild_ = false;
      bool nativeHandleReleased_ = false;

      // Images and synchronization
      std::vector<SwapchainImage> images_;
      std::vector<FrameResources> frameResources_;
      uint32_t numSwapchainImages_ = 0;
      uint32_t currentImageIndex_ = 0; // Current swapchain image being rendered to
      uint32_t currentFrameIndex_ = 0; // Current frame resource index
      uint64_t frameCounter_ = 0;      // Total frames rendered

      // Surface capabilities cache
      VkSurfaceCapabilitiesKHR surfaceCapabilities_ = {};
      std::vector<VkSurfaceFormatKHR> surfaceFormats_;
      std::vector<VkPresentModeKHR> presentModes_;
      bool surfaceDataValid_ = false;

      // Android pre-rotation
      VkExtent2D identityExtent_ = { 0, 0 };
      VkSurfaceTransformFlagBitsKHR preTransform_ = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  };

  class VulkanImmediateCommands final
  {
    public:
      // the maximum number of command buffers which can similtaneously exist in the system; when we run out of buffers, we stall and wait until
      // an existing buffer becomes available
      static constexpr uint32_t kMaxCommandBuffers = 8;

      VulkanImmediateCommands(VkDevice device, uint32_t queueFamilyIndex, const char* debugName);

      ~VulkanImmediateCommands();

      VulkanImmediateCommands(const VulkanImmediateCommands&) = delete;

      VulkanImmediateCommands& operator=(const VulkanImmediateCommands&) = delete;

      struct CommandBufferWrapper
      {
        VkCommandBuffer cmdBuf_ = VK_NULL_HANDLE;
        VkCommandBuffer cmdBufAllocated_ = VK_NULL_HANDLE;
        SubmitHandle handle_ = {};
        VkFence fence_ = VK_NULL_HANDLE;
        VkSemaphore semaphore_ = VK_NULL_HANDLE;
        bool isEncoding_ = false;
      };

      // returns the current command buffer (creates one if it does not exist)
      const CommandBufferWrapper& acquire();

      SubmitHandle submit(const CommandBufferWrapper& wrapper);

      void waitSemaphore(VkSemaphore semaphore);

      void signalSemaphore(VkSemaphore semaphore, uint64_t signalValue);

      VkSemaphore acquireLastSubmitSemaphore();

      VkFence getVkFence(SubmitHandle handle) const;

      SubmitHandle getLastSubmitHandle() const;

      SubmitHandle getNextSubmitHandle() const;

      bool isReady(SubmitHandle handle, bool fastCheckNoVulkan = false) const;

      void wait(SubmitHandle handle);

      void waitAll();

    private:
      void purge();

      VkDevice device_ = VK_NULL_HANDLE;
      VkQueue queue_ = VK_NULL_HANDLE;
      VkCommandPool commandPool_ = VK_NULL_HANDLE;
      uint32_t queueFamilyIndex_ = 0;
      const char* debugName_ = "";
      CommandBufferWrapper buffers_[kMaxCommandBuffers];
      SubmitHandle lastSubmitHandle_ = SubmitHandle();
      SubmitHandle nextSubmitHandle_ = SubmitHandle();
      VkSemaphoreSubmitInfo lastSubmitSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
      VkSemaphoreSubmitInfo waitSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};   // extra "wait" semaphore
      VkSemaphoreSubmitInfo signalSemaphore_ = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}; // extra "signal" semaphore
      uint32_t numAvailableCommandBuffers_ = kMaxCommandBuffers;
      uint32_t submitCounter_ = 1;
  };

  struct RenderPipelineState final
  {
    RenderPipelineDesc desc_;

    uint32_t numBindings_ = 0;
    uint32_t numAttributes_ = 0;
    VkVertexInputBindingDescription vkBindings_[VertexInput::VERTEX_BUFFER_MAX] = {};
    VkVertexInputAttributeDescription vkAttributes_[VertexInput::VERTEX_ATTRIBUTES_MAX] = {};

    // non-owning, the last seen VkDescriptorSetLayout from VulkanContext::vkDSL_ (if the context has a new layout, invalidate all VkPipeline
    // objects)
    VkDescriptorSetLayout lastVkDescriptorSetLayout_ = VK_NULL_HANDLE;

    VkShaderStageFlags shaderStageFlags_ = 0;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    void* specConstantDataStorage_ = nullptr;

    uint32_t viewMask_ = 0;
  };

  class VulkanPipelineBuilder final
  {
    public:
      VulkanPipelineBuilder();

      ~VulkanPipelineBuilder() = default;

      VulkanPipelineBuilder& dynamicState(VkDynamicState state);

      VulkanPipelineBuilder& primitiveTopology(VkPrimitiveTopology topology);

      VulkanPipelineBuilder& rasterizationSamples(VkSampleCountFlagBits samples, float minSampleShading);

      VulkanPipelineBuilder& shaderStage(VkPipelineShaderStageCreateInfo stage);

      VulkanPipelineBuilder& stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp);

      VulkanPipelineBuilder& stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask, uint32_t writeMask, uint32_t reference);

      VulkanPipelineBuilder& cullMode(VkCullModeFlags mode);

      VulkanPipelineBuilder& frontFace(VkFrontFace mode);

      VulkanPipelineBuilder& polygonMode(VkPolygonMode mode);

      VulkanPipelineBuilder& vertexInputState(const VkPipelineVertexInputStateCreateInfo& state);

      VulkanPipelineBuilder& viewMask(uint32_t mask);

      VulkanPipelineBuilder& colorAttachments(const VkPipelineColorBlendAttachmentState* states, const VkFormat* formats, uint32_t numColorAttachments);

      VulkanPipelineBuilder& depthAttachmentFormat(VkFormat format);

      VulkanPipelineBuilder& stencilAttachmentFormat(VkFormat format);

      VulkanPipelineBuilder& patchControlPoints(uint32_t numPoints);

      VkResult build(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout, VkPipeline* outPipeline, const char* debugName = nullptr) noexcept;

      static uint32_t getNumPipelinesCreated() { return numPipelinesCreated_; }

    private:
      static constexpr int MAX_DYNAMIC_STATES = 128;
      uint32_t numDynamicStates_ = 0;
      VkDynamicState dynamicStates_[MAX_DYNAMIC_STATES] = {};

      uint32_t numShaderStages_ = 0;
      VkPipelineShaderStageCreateInfo shaderStages_[(uint8_t)ShaderStage::Frag + 1] = {};

      VkPipelineVertexInputStateCreateInfo vertexInputState_;
      VkPipelineInputAssemblyStateCreateInfo inputAssembly_;
      VkPipelineRasterizationStateCreateInfo rasterizationState_;
      VkPipelineMultisampleStateCreateInfo multisampleState_;
      VkPipelineDepthStencilStateCreateInfo depthStencilState_;
      VkPipelineTessellationStateCreateInfo tessellationState_;

      uint32_t viewMask_ = 0;
      uint32_t numColorAttachments_ = 0;
      VkPipelineColorBlendAttachmentState colorBlendAttachmentStates_[MAX_COLOR_ATTACHMENTS] = {};
      VkFormat colorAttachmentFormats_[MAX_COLOR_ATTACHMENTS] = {};

      VkFormat depthAttachmentFormat_ = VK_FORMAT_UNDEFINED;
      VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

      static uint32_t numPipelinesCreated_;
  };

  struct ComputePipelineState final
  {
    ComputePipelineDesc desc_;

    // non-owning, the last seen VkDescriptorSetLayout from VulkanContext::vkDSL_ (invalidate all VkPipeline objects on new layout)
    VkDescriptorSetLayout lastVkDescriptorSetLayout_ = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    void* specConstantDataStorage_ = nullptr;
  };

  struct RayTracingPipelineState final
  {
    RayTracingPipelineDesc desc_;

    // non-owning, the last seen VkDescriptorSetLayout from VulkanContext::vkDSL_ (invalidate all VkPipeline objects on new layout)
    VkDescriptorSetLayout lastVkDescriptorSetLayout_ = VK_NULL_HANDLE;

    VkShaderStageFlags shaderStageFlags_ = 0;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    void* specConstantDataStorage_ = nullptr;

    Holder<BufferHandle> sbt;

    VkStridedDeviceAddressRegionKHR sbtEntryRayGen = {};
    VkStridedDeviceAddressRegionKHR sbtEntryMiss = {};
    VkStridedDeviceAddressRegionKHR sbtEntryHit = {};
    VkStridedDeviceAddressRegionKHR sbtEntryCallable = {};
  };

  struct ShaderModuleState final
  {
    VkShaderModule sm = VK_NULL_HANDLE;
    uint32_t pushConstantsSize = 0;
  };

  struct AccelerationStructure
  {
    bool isTLAS = false;
    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {};
    VkAccelerationStructureKHR vkHandle = VK_NULL_HANDLE;
    uint64_t deviceAddress = 0;
    Holder<BufferHandle> buffer;
    Holder<BufferHandle> scratchBuffer; // Store only for TLAS
  };

  class CommandBuffer final : public ICommandBuffer
  {
    public:
      CommandBuffer() = default;

      explicit CommandBuffer(VulkanContext* ctx);

      ~CommandBuffer() override;

      CommandBuffer& operator=(CommandBuffer&& other) = default;

      explicit operator VkCommandBuffer() const { return getVkCommandBuffer(); }

      void transitionToShaderReadOnly(TextureHandle handle) const override;

      void cmdBindRayTracingPipeline(RayTracingPipelineHandle handle) override;

      void cmdBindComputePipeline(ComputePipelineHandle handle) override;

      void cmdDispatchThreadGroups(const Dimensions& threadgroupCount, const Dependencies& deps) override;

      void cmdPushDebugGroupLabel(const char* label, uint32_t colorRGBA) const override;

      void cmdInsertDebugEventLabel(const char* label, uint32_t colorRGBA) const override;

      void cmdPopDebugGroupLabel() const override;

      void cmdBeginRendering(const RenderPass& renderPass, const Framebuffer& fb, const Dependencies& deps) override;

      void cmdEndRendering() override;

      void cmdBindViewport(const Viewport& viewport) override;

      void cmdBindScissorRect(const ScissorRect& rect) override;

      void cmdBindRenderPipeline(RenderPipelineHandle handle) override;

      void cmdBindDepthState(const DepthState& desc) override;

      void cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset) override;

      void cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat indexFormat, uint64_t indexBufferOffset) override;

      void cmdPushConstants(const void* data, size_t size, size_t offset) override;

      void cmdFillBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, uint32_t data) override;

      void cmdUpdateBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, const void* data) override;

      void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance) override;

      void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t baseInstance) override;

      void cmdDrawIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) override;

      void cmdDrawIndexedIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) override;

      void cmdDrawIndexedIndirectCount(BufferHandle indirectBuffer, size_t indirectBufferOffset, BufferHandle countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride = 0) override;

      void cmdDrawMeshTasks(const Dimensions& threadgroupCount) override;

      void cmdDrawMeshTasksIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) override;

      void cmdDrawMeshTasksIndirectCount(BufferHandle indirectBuffer, size_t indirectBufferOffset, BufferHandle countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride = 0) override;

      void cmdTraceRays(uint32_t width, uint32_t height, uint32_t depth, const Dependencies& deps) override;

      void cmdSetBlendColor(const float color[4]) override;

      void cmdSetDepthBias(float constantFactor, float slopeFactor, float clamp) override;

      void cmdSetDepthBiasEnable(bool enable) override;

      void cmdResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) override;

      void cmdWriteTimestamp(QueryPoolHandle pool, uint32_t query) override;

      void cmdClearColorImage(TextureHandle tex, const ClearColorValue& value, const TextureLayers& layers) override;

      void cmdCopyImage(TextureHandle src, TextureHandle dst, const Dimensions& extent, const Offset3D& srcOffset, const Offset3D& dstOffset, const TextureLayers& srcLayers, const TextureLayers& dstLayers) override;

      void cmdGenerateMipmap(TextureHandle handle) override;

      void cmdUpdateTLAS(AccelStructHandle handle, BufferHandle instancesBuffer) override;

      VkCommandBuffer getVkCommandBuffer() const { return wrapper_ ? wrapper_->cmdBuf_ : VK_NULL_HANDLE; }

    private:
      void useComputeTexture(TextureHandle handle, VkPipelineStageFlags2 dstStage);

      void bufferBarrier(BufferHandle handle, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);

      friend class VulkanContext;

      VulkanContext* ctx_ = nullptr;
      const VulkanImmediateCommands::CommandBufferWrapper* wrapper_ = nullptr;

      Framebuffer framebuffer_ = {};
      SubmitHandle lastSubmitHandle_ = {};

      VkPipeline lastPipelineBound_ = VK_NULL_HANDLE;

      bool isRendering_ = false;
      uint32_t viewMask_ = 0;

      RenderPipelineHandle currentPipelineGraphics_ = {};
      ComputePipelineHandle currentPipelineCompute_ = {};
      RayTracingPipelineHandle currentPipelineRayTracing_ = {};
  };

  class VulkanStagingDevice final
  {
    public:
      explicit VulkanStagingDevice(VulkanContext& ctx);

      ~VulkanStagingDevice() = default;

      VulkanStagingDevice(const VulkanStagingDevice&) = delete;

      VulkanStagingDevice& operator=(const VulkanStagingDevice&) = delete;

      void bufferSubData(VulkanBuffer& buffer, size_t dstOffset, size_t size, const void* data);

      void imageData2D(VulkanImage& image, const VkRect2D& imageRegion, uint32_t baseMipLevel, uint32_t numMipLevels, uint32_t layer, uint32_t numLayers, VkFormat format, const void* data);

      void imageData3D(VulkanImage& image, const VkOffset3D& offset, const VkExtent3D& extent, VkFormat format, const void* data);

      void getImageData(VulkanImage& image, const VkOffset3D& offset, const VkExtent3D& extent, VkImageSubresourceRange range, VkFormat format, void* outData);

    private:
      static constexpr int kStagingBufferAlignment = 16; // updated to support BC7 compressed image

      struct MemoryRegionDesc
      {
        uint32_t offset_ = 0;
        uint32_t size_ = 0;
        SubmitHandle handle_ = {};
      };

      MemoryRegionDesc getNextFreeOffset(uint32_t size);

      void ensureStagingBufferSize(uint32_t sizeNeeded);

      void waitAndReset();

      VulkanContext& ctx_;
      Holder<BufferHandle> stagingBuffer_;
      uint32_t stagingBufferSize_ = 0;
      uint32_t stagingBufferCounter_ = 0;
      uint32_t maxBufferSize_ = 0;
      const uint32_t minBufferSize_ = 4u * 2048u * 2048u;
      std::vector<MemoryRegionDesc> regions_;
  };

  class VulkanContext final : public IContext
  {
    public:
      VulkanContext(const ContextConfig& config);

      ~VulkanContext() override;

      ICommandBuffer& acquireCommandBuffer() override;

      SubmitHandle submit(ICommandBuffer& commandBuffer, TextureHandle present) override;

      void wait(SubmitHandle handle) override;

      Holder<BufferHandle> createBuffer(const BufferDesc& desc, const char* debugName, Result* outResult) override;

      Holder<SamplerHandle> createSampler(const SamplerStateDesc& desc, Result* outResult) override;

      Holder<TextureHandle> createTexture(const TextureDesc& desc, const char* debugName, Result* outResult) override;

      Holder<TextureHandle> createTextureView(TextureHandle texture, const TextureViewDesc& desc, const char* debugName, Result* outResult) override;

      Holder<ComputePipelineHandle> createComputePipeline(const ComputePipelineDesc& desc, Result* outResult) override;

      Holder<RenderPipelineHandle> createRenderPipeline(const RenderPipelineDesc& desc, Result* outResult) override;

      Holder<RayTracingPipelineHandle> createRayTracingPipeline(const RayTracingPipelineDesc& desc, Result* outResult = nullptr) override;

      Holder<ShaderModuleHandle> createShaderModule(const ShaderModuleDesc& desc, Result* outResult) override;

      Holder<QueryPoolHandle> createQueryPool(uint32_t numQueries, const char* debugName, Result* outResult) override;

      Holder<AccelStructHandle> createAccelerationStructure(const AccelStructDesc& desc, Result* outResult) override;

      void destroy(ComputePipelineHandle handle) override;

      void destroy(RenderPipelineHandle handle) override;

      void destroy(RayTracingPipelineHandle handle) override;

      void destroy(ShaderModuleHandle handle) override;

      void destroy(SamplerHandle handle) override;

      void destroy(BufferHandle handle) override;

      void destroy(TextureHandle handle) override;

      void destroy(QueryPoolHandle handle) override;

      void destroy(AccelStructHandle handle) override;

      void destroy(Framebuffer& fb) override;

      uint64_t gpuAddress(AccelStructHandle handle) const override;

      Result upload(BufferHandle handle, const void* data, size_t size, size_t offset) override;

      Result download(BufferHandle handle, void* data, size_t size, size_t offset) override;

      uint8_t* getMappedPtr(BufferHandle handle) const override;

      uint64_t gpuAddress(BufferHandle handle, size_t offset = 0) const override;

      void flushMappedMemory(BufferHandle handle, size_t offset, size_t size) const override;

      Result upload(TextureHandle handle, const TextureRangeDesc& range, const void* data) override;

      Result download(TextureHandle handle, const TextureRangeDesc& range, void* outData) override;

      Dimensions getDimensions(TextureHandle handle) const override;

      float getAspectRatio(TextureHandle handle) const override;

      Format getFormat(TextureHandle handle) const override;

      TextureHandle getCurrentSwapchainTexture() override;

      Format getSwapchainFormat() const override;

      ColorSpace getSwapchainColorSpace() const override;

      uint32_t getSwapchainCurrentImageIndex() const override;

      uint32_t getNumSwapchainImages() const override;

      void recreateSwapchain(int newWidth, int newHeight) override;

      uint32_t getFramebufferMSAABitMask() const override;

      double getTimestampPeriodToMs() const override;

      bool getQueryPoolResults(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* outData, size_t stride) const override;

      [[nodiscard]] AccelStructSizes getAccelStructSizes(const AccelStructDesc& desc, Result* outResult) const override;

      ///////////////

      VkPipeline getVkPipeline(ComputePipelineHandle handle);

      VkPipeline getVkPipeline(RenderPipelineHandle handle, uint32_t viewMask);

      VkPipeline getVkPipeline(RayTracingPipelineHandle handle);

      uint32_t queryDevices(HWDeviceType deviceType, HWDeviceDesc* outDevices, uint32_t maxOutDevices = 1);

      Result initContext(const HWDeviceDesc& desc);

      Result initSwapchain(uint32_t width, uint32_t height);

      BufferHandle createBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memFlags, Result* outResult, const char* debugName = nullptr);

      SamplerHandle createSampler(const VkSamplerCreateInfo& ci, Result* outResult, Format yuvFormat = Format::Invalid, const char* debugName = nullptr);

      AccelStructHandle createBLAS(const AccelStructDesc& desc, Result* outResult);

      AccelStructHandle createTLAS(const AccelStructDesc& desc, Result* outResult);

      bool hasSwapchain() const noexcept override
      { return swapchain_ != nullptr; }
      // Surface management
      void createSurface(void* nativeWindow, void* display = nullptr) override;
      void destroySurface() override;
      bool hasSurface() const override
      { return vkSurface_ != VK_NULL_HANDLE; }
#if defined(__ANDROID__)
      SurfaceTransform getSwapchainPreTransform() const override;
#endif
      const VkPhysicalDeviceProperties& getVkPhysicalDeviceProperties() const
      {
        return vkPhysicalDeviceProperties2_.properties;
      }

      VkInstance getVkInstance() const { return vkInstance_; }

      VkDevice getVkDevice() const { return vkDevice_; }

      VkPhysicalDevice getVkPhysicalDevice() const { return vkPhysicalDevice_; }

      std::vector<uint8_t> getPipelineCacheData() const;

      // execute a task some time in the future after the submit handle finished processing
      void deferredTask(std::packaged_task<void()>&& task, SubmitHandle handle = SubmitHandle()) const;

      void* getVmaAllocator() const;

      void checkAndUpdateDescriptorSets();

      void bindDefaultDescriptorSets(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint, VkPipelineLayout layout) const;

      // for shaders debugging
      void invokeShaderModuleErrorCallback(int line, int col, const char* debugName, VkShaderModule sm);

      [[nodiscard]] uint32_t getMaxStorageBufferRange() const override;

    private:
      void createInstance();

      void createHeadlessSurface();

      void querySurfaceCapabilities();

      void processDeferredTasks() const;

      void waitDeferredTasks();

      void generateMipmap(TextureHandle handle) const;

      Result growDescriptorPool(uint32_t maxTextures, uint32_t maxSamplers, uint32_t maxAccelStructs);

      ShaderModuleState createShaderModuleFromSPIRV(const void* spirv, size_t numBytes, const char* debugName, Result* outResult) const;

      ShaderModuleState createShaderModuleFromGLSL(ShaderStage stage, const char* source, const char* debugName, Result* outResult) const;

      const VkSamplerYcbcrConversionInfo* getOrCreateYcbcrConversionInfo(Format format);

      VkSampler getOrCreateYcbcrSampler(Format format);

      void addNextPhysicalDeviceProperties(void* properties);

      void getBuildInfoBLAS(const AccelStructDesc& desc, VkAccelerationStructureGeometryKHR& outGeometry, VkAccelerationStructureBuildSizesInfoKHR& outSizesInfo) const;

      void getBuildInfoTLAS(const AccelStructDesc& desc, VkAccelerationStructureGeometryKHR& outGeometry, VkAccelerationStructureBuildSizesInfoKHR& outSizesInfo) const;

      friend class VulkanSwapchain;
      friend class VulkanStagingDevice;

      VkInstance vkInstance_ = VK_NULL_HANDLE;
      VkDebugUtilsMessengerEXT vkDebugUtilsMessenger_ = VK_NULL_HANDLE;
      VkSurfaceKHR vkSurface_ = VK_NULL_HANDLE;
      VkPhysicalDevice vkPhysicalDevice_ = VK_NULL_HANDLE;
      VkDevice vkDevice_ = VK_NULL_HANDLE;

      uint32_t khronosValidationVersion_ = 0;

#if defined(VK_API_VERSION_1_4)
      VkPhysicalDeviceVulkan14Features vkFeatures14_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};
#endif // VK_API_VERSION_1_4
      VkPhysicalDeviceVulkan13Features vkFeatures13_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
      VkPhysicalDeviceVulkan12Features vkFeatures12_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &vkFeatures13_};
      VkPhysicalDeviceVulkan11Features vkFeatures11_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &vkFeatures12_};
      VkPhysicalDeviceFeatures2 vkFeatures10_ = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &vkFeatures11_};

    public:
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
      VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
      VkPhysicalDeviceDepthStencilResolveProperties vkPhysicalDeviceDepthStencilResolveProperties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES, nullptr,};
      VkPhysicalDeviceDriverProperties vkPhysicalDeviceDriverProperties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES, nullptr};
      VkPhysicalDeviceRobustness2FeaturesEXT vkRobustness2Features_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT, nullptr};
      VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT vkSwapchainMaintenance1Features_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT, nullptr};
#if defined(VK_API_VERSION_1_4)
      // provided by Vulkan 1.4
      VkPhysicalDeviceVulkan14Properties vkPhysicalDeviceVulkan14Properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, &vkPhysicalDeviceDriverProperties_,};
#endif // VK_API_VERSION_1_4
      // provided by Vulkan 1.3
      VkPhysicalDeviceVulkan13Properties vkPhysicalDeviceVulkan13Properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, &vkPhysicalDeviceDriverProperties_,};
      // provided by Vulkan 1.2
      VkPhysicalDeviceVulkan12Properties vkPhysicalDeviceVulkan12Properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &vkPhysicalDeviceVulkan13Properties_,};
      // provided by Vulkan 1.1
      VkPhysicalDeviceVulkan11Properties vkPhysicalDeviceVulkan11Properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, &vkPhysicalDeviceVulkan12Properties_,};
      VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &vkPhysicalDeviceVulkan11Properties_, VkPhysicalDeviceProperties{},};

      std::vector<VkFormat> deviceDepthFormats_;
      std::vector<VkSurfaceFormatKHR> deviceSurfaceFormats_;
      VkSurfaceCapabilitiesKHR deviceSurfaceCaps_;
      std::vector<VkPresentModeKHR> devicePresentModes_;

      VkResolveModeFlagBits depthResolveMode_ = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;

      DeviceQueues deviceQueues_;
      std::unique_ptr<VulkanSwapchain> swapchain_;
      VkSemaphore timelineSemaphore_ = VK_NULL_HANDLE;
      std::unique_ptr<VulkanImmediateCommands> immediate_;
      std::unique_ptr<VulkanStagingDevice> stagingDevice_;
      uint32_t currentMaxTextures_ = 16;
      uint32_t currentMaxSamplers_ = 16;
      uint32_t currentMaxAccelStructs_ = 1;
      VkDescriptorSetLayout vkDSL_ = VK_NULL_HANDLE;
      VkDescriptorPool vkDPool_ = VK_NULL_HANDLE;
      VkDescriptorSet vkDSet_ = VK_NULL_HANDLE;
      // don't use staging on devices with shared host-visible memory
      bool useStaging_ = true;
      bool shouldRecreate_ = false;
      int newWidth_ = 0, newHeight_ = 0;
      std::unique_ptr<struct VulkanContextImpl> pimpl_;

      VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;

      // a texture/sampler was created since the last descriptor set update
      mutable bool awaitingCreation_ = false;
      mutable bool awaitingNewImmutableSamplers_ = false;

      ContextConfig config_;
      bool hasRobustness2_ = false;
      bool hasAccelerationStructure_ = false;
      bool hasRayQuery_ = false;
      bool hasRayTracingPipeline_ = false;
      bool has8BitIndices_ = false;
      bool hasCalibratedTimestamps_ = false;
      bool has_EXT_swapchain_maintenance1_ = false;

      TextureHandle dummyTexture_;

      Pool<ShaderModule, ShaderModuleState> shaderModulesPool_;
      Pool<RenderPipeline, RenderPipelineState> renderPipelinesPool_;
      Pool<ComputePipeline, ComputePipelineState> computePipelinesPool_;
      Pool<RayTracingPipeline, RayTracingPipelineState> rayTracingPipelinesPool_;
      Pool<Sampler, VkSampler> samplersPool_;
      Pool<Buffer, VulkanBuffer> buffersPool_;
      Pool<Texture, VulkanImage> texturesPool_;
      Pool<QueryPool, VkQueryPool> queriesPool_;
      Pool<AccelerationStructure, AccelerationStructure> accelStructuresPool_;
  };
} // namespace vk
