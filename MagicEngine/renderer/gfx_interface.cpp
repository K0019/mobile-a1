/**
 * @file gfx_interface.cpp
 * @brief Implementation of gfx interface methods.
 */

#include "gfx_interface.h"

namespace gfx {

void CommandBuffer::beginRendering(const RenderPass& rp, const Framebuffer& fb, const Dependencies& deps) {
    (void)deps; // Dependencies handled by hina internally via transitions

    hina_pass_action action = {};

    // Get dimensions from first color attachment or depth
    uint32_t width = 0, height = 0;
    if (isValid(fb.color[0].texture)) {
        hina_get_texture_size(fb.color[0].texture, &width, &height);
    } else if (isValid(fb.depthStencil.texture)) {
        hina_get_texture_size(fb.depthStencil.texture, &width, &height);
    }
    action.width = width;
    action.height = height;

    // Set up color attachments
    uint32_t colorCount = fb.getNumColorAttachments();
    for (uint32_t i = 0; i < colorCount; ++i) {
        action.colors[i].image = hina_texture_get_default_view(fb.color[i].texture);
        action.colors[i].load_op = static_cast<hina_load_op>(rp.color[i].loadOp);
        action.colors[i].store_op = static_cast<hina_store_op>(rp.color[i].storeOp);
        action.colors[i].clear_color[0] = rp.color[i].clearColor[0];
        action.colors[i].clear_color[1] = rp.color[i].clearColor[1];
        action.colors[i].clear_color[2] = rp.color[i].clearColor[2];
        action.colors[i].clear_color[3] = rp.color[i].clearColor[3];

        // Handle resolve texture for MSAA
        if (isValid(fb.color[i].resolveTexture)) {
            action.colors[i].resolve = hina_texture_get_default_view(fb.color[i].resolveTexture);
        }
    }

    // Set up depth attachment
    if (isValid(fb.depthStencil.texture)) {
        action.depth.image = hina_texture_get_default_view(fb.depthStencil.texture);
        action.depth.load_op = static_cast<hina_load_op>(rp.depth.loadOp);
        action.depth.store_op = static_cast<hina_store_op>(rp.depth.storeOp);
        action.depth.depth_clear = rp.depth.clearDepth;
        action.depth.stencil_clear = static_cast<uint8_t>(rp.stencil.clearStencil);
    }

    hina_cmd_begin_pass(m_cmd, &action);
}

} // namespace gfx
