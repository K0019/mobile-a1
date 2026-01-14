/**
 * @file hina_context.h
 * @brief Minimal hina-vk context wrapper for render graph integration.
 *
 * This is NOT a vk::IContext shim. It's a thin wrapper that provides:
 * - Swapchain info (format, color space, dimensions)
 * - Command buffer access
 * - Texture/buffer creation helpers
 *
 * Features that need more complex vk:: functionality should be migrated
 * to use gfx:: types directly or stubbed out.
 */

#pragma once

#include "gfx_interface.h"
#include "interface.h"  // For vk::Format, vk::ColorSpace
#include <cstdint>

/**
 * @brief Minimal command buffer wrapper.
 * Just holds a pointer to the current frame's hina_cmd*.
 */
class HinaCommandBuffer {
public:
    HinaCommandBuffer() = default;
    ~HinaCommandBuffer() = default;

    void setHinaCmd(gfx::Cmd* cmd) { m_cmd = cmd; }
    gfx::Cmd* getHinaCmd() const { return m_cmd; }

private:
    gfx::Cmd* m_cmd = nullptr;
};

/**
 * @brief Surface pre-transform for mobile optimization.
 */
enum class SurfaceTransform {
    Identity = 0,
    Rotate90 = 1,
    Rotate180 = 2,
    Rotate270 = 3,
};

/**
 * @brief Minimal hina-vk context wrapper.
 *
 * Provides access to swapchain info and resource creation.
 * Does NOT implement vk::IContext - that interface is deprecated.
 */
class HinaContext {
public:
    HinaContext();
    ~HinaContext();

    // Lifecycle
    bool initialize(void* nativeWindow, uint32_t width, uint32_t height);
    void shutdown();

    // Command buffer access
    HinaCommandBuffer& getCommandBuffer() { return m_commandBuffer; }

    // Frame management (called by GfxRenderer)
    // GfxRenderer manages hina_frame_begin/end and passes the results here
    void setFrameData(gfx::Cmd* cmd, gfx::Texture swapchainTexture) {
        m_commandBuffer.setHinaCmd(cmd);
        m_swapchainTexture = swapchainTexture;
    }

    // ========================================================================
    // Swapchain Info
    // ========================================================================

    bool hasSwapchain() const { return m_hasSwapchain; }

    /**
     * @brief Get current swapchain texture for this frame.
     */
    gfx::Texture getCurrentSwapchainHinaTexture() const { return m_swapchainTexture; }

    /**
     * @brief Get swapchain format as hina_format.
     */
    hina_format getSwapchainFormat() const { return m_swapchainFormat; }

    /**
     * @brief Get swapchain format as vk::Format (for backward compat).
     */
    vk::Format getSwapchainFormatVk() const;

    /**
     * @brief Get swapchain color space.
     */
    vk::ColorSpace getSwapchainColorSpace() const;

    /**
     * @brief Get pre-transform for mobile optimization.
     */
    SurfaceTransform getSwapchainPreTransform() const { return m_preTransform; }

    // ========================================================================
    // Texture Helpers
    // ========================================================================

    /**
     * @brief Get dimensions of a texture.
     */
    gfx::Dimensions getDimensions(gfx::Texture tex) const;

    /**
     * @brief Create a texture from gfx::TextureDesc.
     * Returns a gfx::Holder for RAII ownership.
     */
    gfx::Holder<gfx::Texture> createTextureHolder(const gfx::TextureDesc& desc, const char* debugName = nullptr);

    // ========================================================================
    // Buffer Helpers
    // ========================================================================

    /**
     * @brief Get mapped pointer of a host-visible buffer.
     */
    uint8_t* getMappedPtr(gfx::Buffer buf) const;

    /**
     * @brief Flush mapped memory range.
     */
    void flushMappedMemory(gfx::Buffer buf, size_t offset, size_t size) const;

    /**
     * @brief Create a buffer from gfx::BufferDesc.
     * Returns a gfx::Holder for RAII ownership.
     */
    gfx::Holder<gfx::Buffer> createBufferHolder(const gfx::BufferDesc& desc, const char* debugName = nullptr);

    // ========================================================================
    // Window Management
    // ========================================================================

    void recreateSwapchain(int newWidth, int newHeight);
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }

private:
    HinaCommandBuffer m_commandBuffer;
    bool m_initialized = false;
    bool m_hasSwapchain = true;  // Assume we have a swapchain
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // Swapchain state
    gfx::Texture m_swapchainTexture;
    hina_format m_swapchainFormat = HINA_FORMAT_B8G8R8A8_UNORM;
    SurfaceTransform m_preTransform = SurfaceTransform::Identity;
};
