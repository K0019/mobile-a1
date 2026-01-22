/**
 * @file hina_context.cpp
 * @brief Minimal hina-vk context implementation.
 */

#include "hina_context.h"
#include "logging/log.h"

HinaContext::HinaContext() = default;

HinaContext::~HinaContext() {
    shutdown();
}

bool HinaContext::initialize(void* nativeWindow, uint32_t width, uint32_t height) {
    if (m_initialized) return true;

    (void)nativeWindow;  // hina is already initialized by GfxRenderer

    m_width = width;
    m_height = height;

    // Query swapchain format from hina
    // For now, assume BGRA8 SRGB - this matches most desktop/mobile targets
    m_swapchainFormat = HINA_FORMAT_B8G8R8A8_SRGB;
    m_hasSwapchain = true;
    m_preTransform = gfx::SurfaceTransform::Identity;

    m_initialized = true;
    LOG_INFO("HinaContext initialized ({}x{})", width, height);
    return true;
}

void HinaContext::shutdown() {
    if (!m_initialized) return;
    m_initialized = false;
    LOG_INFO("HinaContext shutdown");
}

// Frame data is set by GfxRenderer via setFrameData()

gfx::Format HinaContext::getSwapchainFormatGfx() const {
    // gfx::Format values match hina_format, so just cast
    return static_cast<gfx::Format>(m_swapchainFormat);
}

gfx::ColorSpace HinaContext::getSwapchainColorSpace() const {
    // Assume sRGB non-linear (standard for most displays)
    return gfx::ColorSpace::SRGB_NONLINEAR;
}

gfx::Dimensions HinaContext::getDimensions(gfx::Texture tex) const {
    if (!gfx::isValid(tex)) return {0, 0, 0};
    uint32_t w = 0, h = 0, d = 0;
    hina_get_texture_extent(tex, &w, &h, &d);
    return {w, h, d};
}

gfx::Holder<gfx::Texture> HinaContext::createTextureHolder(const gfx::TextureDesc& desc, const char* debugName) {
    (void)debugName;  // hina doesn't support debug names yet

    hina_texture_desc hinaDesc = hina_texture_desc_default();
    hinaDesc.type = desc.type;
    hinaDesc.format = desc.format;
    hinaDesc.width = desc.width;
    hinaDesc.height = desc.height;
    hinaDesc.depth = desc.depth > 0 ? desc.depth : 1;
    hinaDesc.layers = desc.layers > 0 ? desc.layers : 1;
    hinaDesc.mip_levels = desc.mip_levels > 0 ? desc.mip_levels : 1;
    hinaDesc.usage = desc.usage;

    gfx::Texture texture = hina_make_texture(&hinaDesc);
    if (!hina_texture_is_valid(texture)) {
        LOG_ERROR("HinaContext::createTextureHolder failed");
        return {};
    }

    return gfx::Holder<gfx::Texture>(texture);
}

uint8_t* HinaContext::getMappedPtr(gfx::Buffer buf) const {
    if (!gfx::isValid(buf)) return nullptr;
    return static_cast<uint8_t*>(hina_map_buffer(buf));
}

void HinaContext::flushMappedMemory(gfx::Buffer buf, size_t offset, size_t size) const {
    // No-op: All buffers are HOST_COHERENT, flush is redundant
    (void)buf; (void)offset; (void)size;
}

gfx::Holder<gfx::Buffer> HinaContext::createBufferHolder(const gfx::BufferDesc& desc, const char* debugName) {
    (void)debugName;

    hina_buffer_desc hinaDesc{
        .size = desc.size,
        .flags = desc.flags
    };

    gfx::Buffer buffer = hina_make_buffer(&hinaDesc);
    if (!hina_buffer_is_valid(buffer)) {
        LOG_ERROR("HinaContext::createBufferHolder failed");
        return {};
    }

    return gfx::Holder<gfx::Buffer>(buffer);
}

void HinaContext::recreateSwapchain(int newWidth, int newHeight) {
    m_width = static_cast<uint32_t>(newWidth);
    m_height = static_cast<uint32_t>(newHeight);
    // hina handles swapchain recreation internally
    LOG_INFO("HinaContext::recreateSwapchain ({}x{})", newWidth, newHeight);
}
