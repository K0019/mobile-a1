/**
 * @file gfx_renderer.cpp
 * @brief hina-vk based renderer implementation.
 */

#include "gfx_renderer.h"
#include "hina_context.h"
#include "render_feature.h"
#include "render_graph.h"
#include "logging/log.h"
#include "frame_data.h"
#include "resource/processed_assets.h"
#include "resource/resource_manager.h"
#include "resource/resource_registry.h"
#include <hina_vk.h>  // For hina_recreate_surface
// EnTT integration disabled - using internal ECS
// #include "ecs/Components/Core.h"
// #include <entt/entt.hpp>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Use types from gfx namespace in this implementation file
using namespace gfx;

// ============================================================================
// CPUCuller Implementation
// ============================================================================

void CPUCuller::setFrustum(const glm::mat4& viewProj) {
    // Extract frustum planes from view-projection matrix
    // Using Gribb-Hartmann method
    glm::mat4 m = glm::transpose(viewProj);

    m_frustumPlanes[0] = m[3] + m[0]; // Left
    m_frustumPlanes[1] = m[3] - m[0]; // Right
    m_frustumPlanes[2] = m[3] + m[1]; // Bottom
    m_frustumPlanes[3] = m[3] - m[1]; // Top
    m_frustumPlanes[4] = m[3] + m[2]; // Near
    m_frustumPlanes[5] = m[3] - m[2]; // Far

    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(glm::vec3(m_frustumPlanes[i]));
        m_frustumPlanes[i] /= len;
    }
}

bool CPUCuller::isVisible(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
    for (int i = 0; i < 6; ++i) {
        glm::vec3 normal(m_frustumPlanes[i]);
        float d = m_frustumPlanes[i].w;

        // Find the positive vertex (furthest along plane normal)
        glm::vec3 pVertex;
        pVertex.x = (normal.x >= 0) ? aabbMax.x : aabbMin.x;
        pVertex.y = (normal.y >= 0) ? aabbMax.y : aabbMin.y;
        pVertex.z = (normal.z >= 0) ? aabbMax.z : aabbMin.z;

        if (glm::dot(normal, pVertex) + d < 0) {
            return false;
        }
    }
    return true;
}

CPUCuller::FrustumResult CPUCuller::classifyAABB(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
    bool allInside = true;
    for (int i = 0; i < 6; ++i) {
        glm::vec3 normal(m_frustumPlanes[i]);
        float d = m_frustumPlanes[i].w;

        // Positive vertex (furthest along normal)
        glm::vec3 pVertex;
        pVertex.x = (normal.x >= 0) ? aabbMax.x : aabbMin.x;
        pVertex.y = (normal.y >= 0) ? aabbMax.y : aabbMin.y;
        pVertex.z = (normal.z >= 0) ? aabbMax.z : aabbMin.z;

        if (glm::dot(normal, pVertex) + d < 0) {
            return FrustumResult::Outside;
        }

        // Negative vertex (closest along normal)
        glm::vec3 nVertex;
        nVertex.x = (normal.x >= 0) ? aabbMin.x : aabbMax.x;
        nVertex.y = (normal.y >= 0) ? aabbMin.y : aabbMax.y;
        nVertex.z = (normal.z >= 0) ? aabbMin.z : aabbMax.z;

        if (glm::dot(normal, nVertex) + d < 0) {
            allInside = false;
        }
    }
    return allInside ? FrustumResult::FullyInside : FrustumResult::Intersecting;
}

// ============================================================================
// hina-vk logging callback
// ============================================================================

static void hinaLogCallback(const char* msg) {
    LOG_INFO("[GFX] {}", msg);
}

// ============================================================================
// GfxRenderer Implementation
// ============================================================================

GfxRenderer::GfxRenderer() {
    LOG_INFO("[GfxRenderer] Constructor");
}

GfxRenderer::~GfxRenderer() {
    if (m_initialized) {
        shutdown();
    }
}

bool GfxRenderer::initialize(void* nativeWindow, uint32_t width, uint32_t height) {
    LOG_INFO("[GfxRenderer] Initializing with window={}, size={}x{}", nativeWindow, width, height);

    if (m_initialized) {
        LOG_WARNING("[GfxRenderer] Already initialized!");
        return true;
    }

    m_width = width;
    m_height = height;

    // Initialize hina-vk
    hina_desc desc = hina_desc_default();
    desc.native_window = nativeWindow;
    desc.flags = 0;
    desc.log_fn = hinaLogCallback;
    // Note: Window size is determined by native_window, not by desc fields

    if (!hina_init(&desc)) {
        LOG_ERROR("[GfxRenderer] hina_init failed!");
        return false;
    }
    LOG_INFO("[GfxRenderer] hina_init succeeded");

    // Initialize shader compiler module (separate from Vulkan init)
    hslc_config shader_config = {};
    shader_config.log_fn = [](hina_log_level level, const char* message, void* user_data) {
        (void)user_data;
        if (level == HINA_LOG_ERROR) LOG_ERROR("[hslc] {}", message);
        else if (level == HINA_LOG_WARN) LOG_WARNING("[hslc] {}", message);
        else LOG_INFO("[hslc] {}", message);
    };
    shader_config.log_user_data = nullptr;
    if (!hslc_init(&shader_config)) {
        LOG_ERROR("[GfxRenderer] hslc_init failed!");
        return false;
    }
    LOG_INFO("[GfxRenderer] Shader compiler initialized");

    // Create bind group layouts
    if (!createBindGroupLayouts()) {
        LOG_ERROR("[GfxRenderer] Failed to create bind group layouts");
        return false;
    }

    // Create render targets
    if (!createRenderTargets()) {
        LOG_ERROR("[GfxRenderer] Failed to create render targets");
        return false;
    }

    // Create frame resources
    if (!createFrameResources()) {
        LOG_ERROR("[GfxRenderer] Failed to create frame resources");
        return false;
    }

    // Create default resources (sampler, white texture, normal texture)
    if (!createDefaultResources()) {
        LOG_ERROR("[GfxRenderer] Failed to create default resources");
        return false;
    }

    // Initialize HinaContext for vk::IContext interface (used by RenderGraph/features)
    m_hinaContext = std::make_unique<HinaContext>();
    if (!m_hinaContext->initialize(nativeWindow, width, height)) {
        LOG_ERROR("[GfxRenderer] Failed to initialize HinaContext");
        return false;
    }
    LOG_INFO("[GfxRenderer] HinaContext initialized");

    // Create RenderGraph for pass management
    m_renderGraph = std::make_unique<RenderGraph>();
    m_renderGraph->SetGfxRenderer(this);  // Allow features to access renderer resources
    LOG_INFO("[GfxRenderer] RenderGraph created");

    m_initialized = true;
    LOG_INFO("[GfxRenderer] Initialization complete");
    return true;
}

void GfxRenderer::shutdown() {
    LOG_INFO("[GfxRenderer] Shutting down...");

    if (!m_initialized) return;

    // Note: hina_shutdown() handles GPU wait internally

    // Shutdown RenderGraph (before HinaContext)
    m_registeredFeatures.clear();
    m_renderGraph.reset();
    m_hinaContext.reset();

    // Shutdown resource systems (before destroying layouts they depend on)
    m_materialSystem.shutdown();
    m_meshStorage.clear();

    // Destroy frame resources
    for (auto& frame : m_frames) {
        if (hina_buffer_is_valid(frame.frameConstantsUBO)) {
            hina_destroy_buffer(frame.frameConstantsUBO);
        }
        if (hina_buffer_is_valid(frame.transformRingUBO)) {
            hina_destroy_buffer(frame.transformRingUBO);
        }
        if (hina_buffer_is_valid(frame.objectDataRingUBO)) {
            hina_destroy_buffer(frame.objectDataRingUBO);
        }
    }

    // Destroy default resources
    if (hina_sampler_is_valid(m_defaultSampler)) {
        hina_destroy_sampler(m_defaultSampler);
    }
    if (hina_texture_is_valid(m_defaultWhiteTexture)) {
        hina_destroy_texture(m_defaultWhiteTexture);
    }
    if (hina_texture_is_valid(m_defaultNormalTexture)) {
        hina_destroy_texture(m_defaultNormalTexture);
    }

    // Destroy render targets - G-buffer
    if (hina_texture_is_valid(m_gbuffer.albedo)) {
        hina_destroy_texture(m_gbuffer.albedo);
    }
    if (hina_texture_is_valid(m_gbuffer.normal)) {
        hina_destroy_texture(m_gbuffer.normal);
    }
    if (hina_texture_is_valid(m_gbuffer.materialData)) {
        hina_destroy_texture(m_gbuffer.materialData);
    }
    if (hina_texture_is_valid(m_gbuffer.depth)) {
        hina_destroy_texture(m_gbuffer.depth);
    }
    if (hina_texture_is_valid(m_gbuffer.visibilityID)) {
        hina_destroy_texture(m_gbuffer.visibilityID);
    }

    // Destroy render targets - WBOIT
    if (hina_texture_is_valid(m_wboit.accumulation)) {
        hina_destroy_texture(m_wboit.accumulation);
    }
    if (hina_texture_is_valid(m_wboit.reveal)) {
        hina_destroy_texture(m_wboit.reveal);
    }

    // Destroy bind group layouts
    if (hina_bind_group_layout_is_valid(m_globalLayout)) {
        hina_destroy_bind_group_layout(m_globalLayout);
    }
    if (hina_bind_group_layout_is_valid(m_materialLayout)) {
        hina_destroy_bind_group_layout(m_materialLayout);
    }
    if (hina_bind_group_layout_is_valid(m_dynamicLayout)) {
        hina_destroy_bind_group_layout(m_dynamicLayout);
    }
    if (hina_bind_group_layout_is_valid(m_uiBindGroupLayout)) {
        hina_destroy_bind_group_layout(m_uiBindGroupLayout);
    }

    // Shutdown shader compiler
    hslc_shutdown();

    // Shutdown HinaContext
    if (m_hinaContext) {
        m_hinaContext->shutdown();
        m_hinaContext.reset();
    }

    // Clear registered features
    m_registeredFeatures.clear();

    // Shutdown hina-vk
    hina_shutdown();

    m_initialized = false;
    LOG_INFO("[GfxRenderer] Shutdown complete");
}

bool GfxRenderer::createBindGroupLayouts() {
    LOG_INFO("[GfxRenderer] Creating bind group layouts...");

    // Set 0: Globals
    // Entry format: { binding, type, stage_flags, count, flags }
    {
        hina_bind_group_layout_entry entries[] = {
            { GlobalBindings::FrameConstants, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_ALL_GRAPHICS, 1, HINA_BINDING_FLAG_NONE },
            { GlobalBindings::LightingData, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { GlobalBindings::ShadowAtlas, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { GlobalBindings::AmbientOctMap, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
        };

        hina_bind_group_layout_desc desc = {};
        desc.entries = entries;
        desc.entry_count = sizeof(entries) / sizeof(entries[0]);
        desc.label = "GlobalLayout";

        m_globalLayout = hina_create_bind_group_layout(&desc);
        if (!hina_bind_group_layout_is_valid(m_globalLayout)) {
            LOG_ERROR("[GfxRenderer] Failed to create global bind group layout");
            return false;
        }
        LOG_DEBUG("[GfxRenderer] Created global layout (Set 0)");
    }

    // Set 1: Material (constants UBO + textures)
    {
        hina_bind_group_layout_entry entries[] = {
            { MaterialBindings::Constants, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { MaterialBindings::Albedo, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { MaterialBindings::Normal, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { MaterialBindings::MetallicRoughness, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { MaterialBindings::Emissive, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
            { MaterialBindings::Occlusion, HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER, HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_NONE },
        };

        hina_bind_group_layout_desc desc = {};
        desc.entries = entries;
        desc.entry_count = sizeof(entries) / sizeof(entries[0]);
        desc.label = "MaterialLayout";

        m_materialLayout = hina_create_bind_group_layout(&desc);
        if (!hina_bind_group_layout_is_valid(m_materialLayout)) {
            LOG_ERROR("[GfxRenderer] Failed to create material bind group layout");
            return false;
        }
        LOG_DEBUG("[GfxRenderer] Created material layout (Set 1) with constants UBO");
    }

    // Set 2: Dynamic (with dynamic offset flag for per-draw transforms and bone matrices)
    // Transform and BoneMatrices bindings - both use dynamic offsets
    {
        hina_bind_group_layout_entry entries[] = {
            { DynamicBindings::Transform, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_VERTEX, 1, HINA_BINDING_FLAG_DYNAMIC_OFFSET },
            { DynamicBindings::BoneMatrices, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_VERTEX, 1, HINA_BINDING_FLAG_DYNAMIC_OFFSET },
        };

        hina_bind_group_layout_desc desc = {};
        desc.entries = entries;
        desc.entry_count = sizeof(entries) / sizeof(entries[0]);
        desc.label = "DynamicLayout";

        m_dynamicLayout = hina_create_bind_group_layout(&desc);
        if (!hina_bind_group_layout_is_valid(m_dynamicLayout)) {
            LOG_ERROR("[GfxRenderer] Failed to create dynamic bind group layout");
            return false;
        }
        LOG_DEBUG("[GfxRenderer] Created dynamic layout (Set 2) with Transform and BoneMatrices");
    }

    LOG_INFO("[GfxRenderer] Bind group layouts created successfully");
    return true;
}

bool GfxRenderer::createRenderTargets() {
    // Use internal render resolution for G-buffer (must match SCENE_DEPTH from RenderGraph)
    const uint32_t gbufferWidth = RenderResources::INTERNAL_WIDTH;
    const uint32_t gbufferHeight = RenderResources::INTERNAL_HEIGHT;
    LOG_INFO("[GfxRenderer] Creating render targets at internal resolution {}x{}...", gbufferWidth, gbufferHeight);

    // Common render target usage flags (with INPUT_ATTACHMENT for tile pass support)
    constexpr auto rtUsage = static_cast<hina_texture_usage_flags>(
        HINA_TEXTURE_RENDER_TARGET_BIT | HINA_TEXTURE_SAMPLED_BIT | HINA_TEXTURE_INPUT_ATTACHMENT_BIT);

    // G-Buffer: Albedo (RGBA8_SRGB = 32 bits)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R8G8B8A8_SRGB;
        desc.usage = rtUsage;

        m_gbuffer.albedo = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_gbuffer.albedo)) {
            LOG_ERROR("[GfxRenderer] Failed to create G-buffer albedo texture");
            return false;
        }
        m_gbuffer.albedoView = hina_texture_get_default_view(m_gbuffer.albedo);
        LOG_DEBUG("[GfxRenderer] Created GBuffer_Albedo (R8G8B8A8_SRGB)");
    }

    // G-Buffer: Normal (octahedral-encoded, R16G16_SFLOAT = 32 bits)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R16G16_SFLOAT;
        desc.usage = rtUsage;

        m_gbuffer.normal = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_gbuffer.normal)) {
            LOG_ERROR("[GfxRenderer] Failed to create G-buffer normal texture");
            return false;
        }
        m_gbuffer.normalView = hina_texture_get_default_view(m_gbuffer.normal);
        LOG_DEBUG("[GfxRenderer] Created GBuffer_Normal (R16G16_SFLOAT)");
    }

    // G-Buffer: Material Data (RGBA8 = 32 bits)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R8G8B8A8_UNORM;
        desc.usage = rtUsage;

        m_gbuffer.materialData = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_gbuffer.materialData)) {
            LOG_ERROR("[GfxRenderer] Failed to create G-buffer material texture");
            return false;
        }
        m_gbuffer.materialDataView = hina_texture_get_default_view(m_gbuffer.materialData);
        LOG_DEBUG("[GfxRenderer] Created GBuffer_MaterialData (R8G8B8A8_UNORM)");
    }

    // G-Buffer: Depth (D32F)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_D32_SFLOAT;
        // Note: G-buffer depth is no longer used - SceneRenderFeature uses SCENE_DEPTH directly
        desc.usage = rtUsage;

        m_gbuffer.depth = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_gbuffer.depth)) {
            LOG_ERROR("[GfxRenderer] Failed to create G-buffer depth texture");
            return false;
        }
        m_gbuffer.depthView = hina_texture_get_default_view(m_gbuffer.depth);
        LOG_DEBUG("[GfxRenderer] Created GBuffer_Depth (D32_SFLOAT)");
    }

    // G-Buffer: Visibility ID (R32UI for picking)
    // Needs TRANSFER_SRC for GPU->CPU readback via hina_download_texture
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R32_UINT;
        desc.usage = static_cast<hina_texture_usage_flags>(rtUsage | HINA_TEXTURE_TRANSFER_SRC_BIT);

        m_gbuffer.visibilityID = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_gbuffer.visibilityID)) {
            LOG_ERROR("[GfxRenderer] Failed to create visibility ID texture");
            return false;
        }
        m_gbuffer.visibilityIDView = hina_texture_get_default_view(m_gbuffer.visibilityID);
        LOG_DEBUG("[GfxRenderer] Created GBuffer_VisibilityID (R32_UINT)");
    }

    // WBOIT Accumulation (RGBA16F)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R16G16B16A16_SFLOAT;
        desc.usage = rtUsage;

        m_wboit.accumulation = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_wboit.accumulation)) {
            LOG_ERROR("[GfxRenderer] Failed to create WBOIT accumulation texture");
            return false;
        }
        m_wboit.accumulationView = hina_texture_get_default_view(m_wboit.accumulation);
        LOG_DEBUG("[GfxRenderer] Created WBOIT_Accum (R16G16B16A16_SFLOAT)");
    }

    // WBOIT Reveal (R8)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R8_UNORM;
        desc.usage = rtUsage;

        m_wboit.reveal = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_wboit.reveal)) {
            LOG_ERROR("[GfxRenderer] Failed to create WBOIT reveal texture");
            return false;
        }
        m_wboit.revealView = hina_texture_get_default_view(m_wboit.reveal);
        LOG_DEBUG("[GfxRenderer] Created WBOIT_Reveal (R8_UNORM)");
    }

    // Create view output textures (Game, Scene, Preview)
    for (size_t i = 0; i < static_cast<size_t>(ViewId::Count); ++i) {
        if (!createViewOutput(static_cast<ViewId>(i), m_width, m_height)) {
            LOG_ERROR("[GfxRenderer] Failed to create view output {}", i);
            return false;
        }
    }

    LOG_INFO("[GfxRenderer] Render targets created successfully");
    return true;
}

bool GfxRenderer::createViewOutput(ViewId viewId, uint32_t width, uint32_t height) {
    auto& output = m_viewOutputs[static_cast<size_t>(viewId)];

    // Destroy existing if valid
    if (output.valid) {
        if (hina_sampler_is_valid(output.sampler)) {
            hina_destroy_sampler(output.sampler);
        }
        if (hina_texture_is_valid(output.texture)) {
            hina_destroy_texture(output.texture);
        }
        output.valid = false;
    }

    // Create texture
    hina_texture_desc desc = {};
    desc.width = width;
    desc.height = height;
    // Use swapchain format for compatibility with blit pipeline
    // Fallback to SRGB which is more common on Android
    desc.format = m_hinaContext ? static_cast<hina_format>(m_hinaContext->getSwapchainFormat())
                                : HINA_FORMAT_B8G8R8A8_SRGB;
    // ViewOutput is used both as render target (by FinalBlit) and as sampled texture (by ImGui)
    // Note: hina-vk doesn't expose TRANSFER_DST, so blit is only used for swapchain destination
    desc.usage = static_cast<hina_texture_usage_flags>(
        HINA_TEXTURE_RENDER_TARGET_BIT | HINA_TEXTURE_SAMPLED_BIT | HINA_TEXTURE_TRANSFER_SRC_BIT);

    output.texture = hina_make_texture(&desc);
    if (!hina_texture_is_valid(output.texture)) {
        LOG_ERROR("[GfxRenderer] Failed to create view output texture for view {}", static_cast<int>(viewId));
        return false;
    }
    output.view = hina_texture_get_default_view(output.texture);

    // Create sampler
    hina_sampler_desc samplerDesc = hina_sampler_desc_default();
    samplerDesc.min_filter = HINA_FILTER_LINEAR;
    samplerDesc.mag_filter = HINA_FILTER_LINEAR;
    samplerDesc.mipmap_filter = HINA_FILTER_NEAREST;
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_w = HINA_ADDRESS_CLAMP_TO_EDGE;

    output.sampler = hina_make_sampler(&samplerDesc);
    if (!hina_sampler_is_valid(output.sampler)) {
        LOG_ERROR("[GfxRenderer] Failed to create view output sampler for view {}", static_cast<int>(viewId));
        hina_destroy_texture(output.texture);
        return false;
    }

    output.width = width;
    output.height = height;
    output.valid = true;

    // Transition to SHADER_READ immediately so first-frame sampling is valid
    // (texture will contain garbage/zeros but won't cause validation errors)
    gfx::Cmd* initCmd = hina_cmd_begin();
    if (initCmd) {
        hina_cmd_transition_texture(initCmd, output.texture, HINA_TEXSTATE_SHADER_READ);
        hina_cmd_end(initCmd);
        hina_ticket ticket = hina_submit_immediate(initCmd);
        hina_wait_ticket(ticket);
    }

    const char* viewNames[] = {"Game", "Scene", "Preview"};
    LOG_DEBUG("[GfxRenderer] Created ViewOutput[{}] ({}x{})", viewNames[static_cast<int>(viewId)], width, height);

    return true;
}

void GfxRenderer::resizeViewOutput(ViewId viewId, uint32_t width, uint32_t height) {
    auto& output = m_viewOutputs[static_cast<size_t>(viewId)];
    if (output.width == width && output.height == height) {
        return;  // No change needed
    }

    // Recreate with new dimensions
    // ViewOutput pointer remains stable so ImGui texture IDs stay valid
    createViewOutput(viewId, width, height);
}

uint64_t GfxRenderer::getViewOutputTextureId(ViewId viewId) const {
    const auto& output = m_viewOutputs[static_cast<size_t>(viewId)];
    if (!output.valid) return 0;
    // Encode ViewOutput pointer with high bit set to distinguish from texture handles
    return IMGUI_VIEWOUTPUT_BIT | reinterpret_cast<uint64_t>(&output);
}

bool GfxRenderer::createFrameResources() {
    LOG_INFO("[GfxRenderer] Creating frame resources ({} frames in flight)...", GFX_MAX_FRAMES);

    for (uint32_t i = 0; i < GFX_MAX_FRAMES; ++i) {
        auto& frame = m_frames[i];
        frame.frameIndex = i;

        // Frame constants UBO
        {
            hina_buffer_desc desc = {};
            desc.size = sizeof(FrameConstants);
            desc.memory = HINA_BUFFER_CPU;
            desc.usage = HINA_BUFFER_UNIFORM;

            frame.frameConstantsUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.frameConstantsUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create frame constants UBO for frame {}", i);
                return false;
            }
        }

        // Transform ring buffer (dynamic offsets) - persistently mapped
        {
            hina_buffer_desc desc = {};
            desc.size = TRANSFORM_UBO_SIZE;
            desc.memory = HINA_BUFFER_CPU;
            desc.usage = HINA_BUFFER_UNIFORM;

            frame.transformRingUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.transformRingUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create transform ring UBO for frame {}", i);
                return false;
            }

            // Persistently map the buffer once - no per-frame mapping needed
            frame.transformRingMapped = hina_mapped_buffer_ptr(frame.transformRingUBO);
            if (!frame.transformRingMapped) {
                LOG_ERROR("[GfxRenderer] Failed to map transform ring UBO for frame {}", i);
                return false;
            }
        }

        // Bone matrix ring buffer (for skinned meshes) - persistently mapped
        {
            hina_buffer_desc desc = {};
            desc.size = BONE_MATRIX_UBO_SIZE;
            desc.memory = HINA_BUFFER_CPU;
            desc.usage = HINA_BUFFER_UNIFORM;

            frame.boneMatrixRingUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.boneMatrixRingUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create bone matrix ring UBO for frame {}", i);
                return false;
            }

            // Persistently map the buffer once - no per-frame mapping needed
            frame.boneMatrixRingMapped = hina_mapped_buffer_ptr(frame.boneMatrixRingUBO);
            if (!frame.boneMatrixRingMapped) {
                LOG_ERROR("[GfxRenderer] Failed to map bone matrix ring UBO for frame {}", i);
                return false;
            }
        }

        // Object data ring buffer
        {
            hina_buffer_desc desc = {};
            desc.size = MAX_TRANSFORMS * 64; // 64 bytes per object data
            desc.memory = HINA_BUFFER_CPU;
            desc.usage = HINA_BUFFER_UNIFORM;

            frame.objectDataRingUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.objectDataRingUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create object data ring UBO for frame {}", i);
                return false;
            }
        }

        // Create dynamic bind group (Set 2) with transform and bone matrix UBOs
        // This bind group is used with dynamic offsets for per-draw transforms and skinning
        {
            hina_bind_group_entry entries[2] = {};

            // Binding 0: Transform UBO (dynamic offset)
            entries[0].binding = DynamicBindings::Transform;
            entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
            entries[0].buffer.buffer = frame.transformRingUBO;
            entries[0].buffer.offset = 0;
            entries[0].buffer.size = TRANSFORM_ALIGNMENT;  // Single transform size

            // Binding 1: Bone Matrices UBO (dynamic offset)
            entries[1].binding = DynamicBindings::BoneMatrices;
            entries[1].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
            entries[1].buffer.buffer = frame.boneMatrixRingUBO;
            entries[1].buffer.offset = 0;
            entries[1].buffer.size = gfx::BONE_MATRICES_SIZE;  // 64 bones * 64 bytes = 4KB

            hina_bind_group_desc desc = {};
            desc.layout = m_dynamicLayout;
            desc.entries = entries;
            desc.entry_count = 2;

            frame.dynamicBindGroup = hina_create_bind_group(&desc);
            if (!hina_bind_group_is_valid(frame.dynamicBindGroup)) {
                LOG_ERROR("[GfxRenderer] Failed to create dynamic bind group for frame {}", i);
                return false;
            }
        }

        LOG_DEBUG("[GfxRenderer] Created frame resources for frame {}", i);
    }

    LOG_INFO("[GfxRenderer] Frame resources created successfully");
    return true;
}

bool GfxRenderer::beginFrame() {
    if (!m_initialized) {
        LOG_ERROR("[GfxRenderer] beginFrame called but not initialized!");
        return false;
    }

    SwapchainImage swap = hina_frame_begin();
    if (!swap.texture.id) {
        LOG_WARNING("[GfxRenderer] Failed to acquire swapchain image");
        return false;
    }

    // Reset frame resources
    auto& frame = m_frames[m_currentFrame];
    frame.transformRingOffset = 0;
    frame.boneMatrixRingOffset = 0;
    frame.objectDataRingOffset = 0;

    // Reset upload statistics for this frame
    m_uploadStats = {};

    // Save swapchain image for this frame
    frame.swapchainImage = swap;
    frame.swapchainView = hina_texture_get_default_view(swap.texture);

    // Note: Draw queue is cleared at the END of the frame (in endFrame()),
    // not here, because submitMesh/submitEcsEntities may be called
    // before beginFrame() in the engine update loop.

    return true;
}

void GfxRenderer::render(RenderFrameData& frameData) {
    if (!m_initialized) {
        return;
    }

    // Flush any pending mesh uploads before rendering
    // This batches buffer copies: maps each buffer once, copies all data, unmaps once
    flushPendingUploads();

    auto& frame = m_frames[m_currentFrame];

    // Begin command recording
    frame.cmd = hina_cmd_begin();
    if (!frame.cmd) {
        LOG_ERROR("[GfxRenderer] hina_cmd_begin() returned null!");
        return;
    }

    // Set the frame data in HinaContext for feature access
    if (m_hinaContext) {
        m_hinaContext->setFrameData(frame.cmd, frame.swapchainImage.texture);
    }

    // Execute RenderGraph if features are registered
    if (m_renderGraph && !m_registeredFeatures.empty()) {
        // Import swapchain texture into render graph
        gfx::TextureDesc swapchainDesc{
            .type = HINA_TEX_TYPE_2D,
            .format = HINA_FORMAT_B8G8R8A8_UNORM,
            .width = m_width,
            .height = m_height,
            .depth = 1,
            .layers = 1,
            .mip_levels = 1,
            .usage = static_cast<hina_texture_usage_flags>(gfx::TextureUsage::RenderTarget)
        };
        gfx::Texture swapchainTexture = m_hinaContext->getCurrentSwapchainHinaTexture();

        m_renderGraph->ImportExternalTexture(
            RenderResources::SWAPCHAIN_IMAGE,
            swapchainTexture,
            swapchainDesc
        );

        // NOTE: SCENE_COLOR and SCENE_DEPTH are TRANSIENT resources managed by the RenderGraph.
        // They are registered in RegisterBuiltInResources() during compilation.
        // Do NOT import them as external - that would overwrite the correctly allocated transient textures.

        // Import active mesh chunks for RenderGraph resource tracking
        m_meshStorage.importChunksToRenderGraph(*m_renderGraph);

        // Begin new frame for RenderGraph
        m_renderGraph->BeginFrame();

        // Process each view with its own feature mask
        for (size_t viewIndex = 0; viewIndex < frameData.views.size(); ++viewIndex) {
            FrameData& viewFrameData = frameData.views[viewIndex];
            ViewId viewId = static_cast<ViewId>(viewFrameData.viewId);

            // Get or create output texture for this view
            ViewOutput& viewOutput = m_viewOutputs[static_cast<size_t>(viewId)];
            if (!viewOutput.valid) {
                createViewOutput(viewId, RenderResources::INTERNAL_WIDTH, RenderResources::INTERNAL_HEIGHT);
            }

            // Set up ViewOutputConfig for this view
            // - resolvedColor: Always the per-view texture (scene rendered here, FinalBlit copies to swapchain)
            // - swapchainImage: Only set for the presented view (FinalBlit copies VIEW_OUTPUT to swapchain)
            bool isPresentedView = (viewFrameData.viewId == frameData.presentedViewId);
            ViewOutputConfig outputConfig{
                .resolvedColor = viewOutput.valid ? viewOutput.texture : gfx::Texture{},
                .swapchainImage = isPresentedView ? frame.swapchainImage.texture : gfx::Texture{}
            };

            // Update per-frame constants for this view
            m_currentFrameData = &viewFrameData;
            updateFrameConstants(viewFrameData);

            // Execute RenderGraph with this view's feature mask
            // The RenderGraph will filter passes based on viewFrameData.featureMask
            // SceneComposite clears SCENE_COLOR with sky color before rendering
            m_renderGraph->Execute(m_hinaContext->getCommandBuffer().getHinaCmd(), viewFrameData, outputConfig);

            // Store output info back to view for application to access
            if (viewOutput.valid) {
                viewFrameData.outputInfo.colorTextureId = getViewOutputTextureId(viewId);
            }
        }

        // Transition swapchain to PRESENT layout after all views are rendered
        // Note: For ImGui apps, ImGuiRenderFeature handles swapchain rendering
        // For non-ImGui apps (Game.exe), a PresentFeature should handle swapchain presentation
        hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_PRESENT);
    }

    // Update any dirty material bind groups before rendering
    m_materialSystem.updateDirtyMaterials();

    // Submit
    hina_frame_submit(frame.cmd);
}

void GfxRenderer::endFrame() {
    if (!m_initialized) return;

    hina_frame_end();
    m_currentFrame = (m_currentFrame + 1) % GFX_MAX_FRAMES;

    // Track frames for window ready state
    m_framesRendered++;
    if (!m_windowReadyForShow && m_framesRendered >= MIN_FRAMES_BEFORE_SHOW) {
        m_windowReadyForShow = true;
    }

    // Swap buffers for features (double-buffering parameter blocks)
    for (auto& [handle, info] : m_features) {
        if (info.feature) {
            info.feature->SwapBuffersForGT_RT();
        }
    }
}

// ============================================================================
// Upload Batching (Phase 3)
// ============================================================================

void GfxRenderer::recordMeshUpload(uint32_t vertexCount, uint32_t indexCount, uint32_t totalBytes) {
    m_uploadStats.meshUploadsThisFrame++;
    m_uploadStats.verticesUploadedThisFrame += vertexCount;
    m_uploadStats.indicesUploadedThisFrame += indexCount;
    m_uploadStats.bytesUploadedThisFrame += totalBytes;
}

void GfxRenderer::flushPendingUploads() {
    // Flush batched mesh uploads from ChunkedMeshStorage
    // This executes all pending buffer copies, grouping by buffer to minimize
    // map/unmap calls (map each buffer once, copy all data, unmap once)
    auto& chunkedStorage = m_meshStorage.getChunkedStorage();
    uint32_t copiesFlushed = chunkedStorage.flushPendingCopies();

    // Log statistics if there were uploads this frame
    if (copiesFlushed > 0 || m_uploadStats.meshUploadsThisFrame > 0) {
        LOG_DEBUG("[GfxRenderer] Frame: {} meshes, {} buffer copies flushed ({} vertices, {} indices, {} bytes)",
                  m_uploadStats.meshUploadsThisFrame,
                  copiesFlushed,
                  m_uploadStats.verticesUploadedThisFrame,
                  m_uploadStats.indicesUploadedThisFrame,
                  m_uploadStats.bytesUploadedThisFrame);
    }
}

void GfxRenderer::onResize(uint32_t width, uint32_t height) {
    if (width == m_width && height == m_height) return;

    LOG_INFO("[GfxRenderer] Resizing from {}x{} to {}x{}", m_width, m_height, width, height);

    m_width = width;
    m_height = height;

    // GBuffer, HDR, WBOIT, shadow atlas, and ambient oct map all use fixed internal
    // resolution (INTERNAL_WIDTH x INTERNAL_HEIGHT) — no need to recreate on window resize.
    // Only ViewOutputs need resizing since they match window dimensions for final blit.
    for (size_t i = 0; i < static_cast<size_t>(ViewId::Count); ++i) {
        resizeViewOutput(static_cast<ViewId>(i), width, height);
    }
}

void GfxRenderer::updateFrameConstants(const FrameData& frameData) {
    auto& frame = m_frames[m_currentFrame];

    FrameConstants constants = {};
    constants.view = frameData.viewMatrix;
    constants.proj = frameData.projMatrix;
    constants.viewProj = frameData.projMatrix * frameData.viewMatrix;
    constants.invViewProj = glm::inverse(constants.viewProj);
    constants.cameraPos = glm::vec4(frameData.cameraPos, 1.0f);
    constants.cameraDir = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);  // TODO: Extract from view matrix
    constants.screenSize = glm::vec2(static_cast<float>(frameData.screenWidth),
                                     static_cast<float>(frameData.screenHeight));
    constants.invScreenSize = glm::vec2(1.0f / constants.screenSize.x, 1.0f / constants.screenSize.y);
    constants.zNear = frameData.zNear;
    constants.zFar = frameData.zFar;
    constants.time = static_cast<float>(frameData.frameNumber) * 0.016f;  // Approximate time
    constants.deltaTime = frameData.deltaTime;

    // Upload to UBO - buffer is persistently mapped (HOST_VISIBLE + HOST_COHERENT)
    // No need to unmap - VMA persistently mapped buffers should stay mapped
    void* mapped = hina_mapped_buffer_ptr(frame.frameConstantsUBO);
    if (mapped) {
        std::memcpy(mapped, &constants, sizeof(constants));
        // Note: HOST_COHERENT means no flush needed, writes are visible to GPU
    } else {
        LOG_WARNING("[GfxRenderer] Failed to map frame constants buffer");
    }
}

bool GfxRenderer::createDefaultResources() {
    LOG_INFO("[GfxRenderer] Creating default resources...");

    // Create default sampler (linear filtering, repeat wrap)
    {
        hina_sampler_desc desc = hina_sampler_desc_default();
        m_defaultSampler = hina_make_sampler(&desc);
        if (!hina_sampler_is_valid(m_defaultSampler)) {
            LOG_ERROR("[GfxRenderer] Failed to create default sampler");
            return false;
        }
        LOG_DEBUG("[GfxRenderer] Created default sampler");
    }

    // Create 1x1 white texture for missing textures
    {
        constexpr auto texUsage = static_cast<hina_texture_usage_flags>(HINA_TEXTURE_SAMPLED_BIT);
        static const uint8_t whitePixel[] = { 255, 255, 255, 255 };

        hina_texture_desc desc = {};
        desc.width = 1;
        desc.height = 1;
        desc.format = HINA_FORMAT_R8G8B8A8_UNORM;
        desc.usage = texUsage;
        desc.initial_data = whitePixel;

        m_defaultWhiteTexture = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_defaultWhiteTexture)) {
            LOG_ERROR("[GfxRenderer] Failed to create default white texture");
            return false;
        }
        m_defaultWhiteTextureView = hina_texture_get_default_view(m_defaultWhiteTexture);
        LOG_DEBUG("[GfxRenderer] Created default white texture (1x1)");
    }

    // Create 1x1 default normal texture (flat normal pointing up)
    {
        constexpr auto texUsage = static_cast<hina_texture_usage_flags>(HINA_TEXTURE_SAMPLED_BIT);
        // Flat normal: (0.5, 0.5, 1.0) in tangent space = 128, 128, 255 in UNORM
        static const uint8_t normalPixel[] = { 128, 128, 255, 255 };

        hina_texture_desc desc = {};
        desc.width = 1;
        desc.height = 1;
        desc.format = HINA_FORMAT_R8G8B8A8_UNORM;
        desc.usage = texUsage;
        desc.initial_data = normalPixel;

        m_defaultNormalTexture = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_defaultNormalTexture)) {
            LOG_ERROR("[GfxRenderer] Failed to create default normal texture");
            return false;
        }
        m_defaultNormalTextureView = hina_texture_get_default_view(m_defaultNormalTexture);
        LOG_DEBUG("[GfxRenderer] Created default normal texture (1x1)");
    }

    // Initialize material system with material layout and default sampler
    if (!m_materialSystem.initialize(m_materialLayout, m_defaultSampler)) {
        LOG_ERROR("[GfxRenderer] Failed to initialize material system");
        return false;
    }
    LOG_DEBUG("[GfxRenderer] Initialized material system");

    // Create UI bind group layout (single combined image sampler)
    // This is used for all UI rendering (ImGui, UI2D) with transient bind groups
    // NOTE: Stage flags must be VERTEX|FRAGMENT to match what HSL shader compiler generates
    // for bindings() blocks, even though textures are only used in fragment stage
    {
        hina_bind_group_layout_entry uiLayoutEntry = {};
        uiLayoutEntry.binding = 0;
        uiLayoutEntry.type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
        uiLayoutEntry.stage_flags = HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT;
        uiLayoutEntry.count = 1;

        hina_bind_group_layout_desc uiLayoutDesc = {};
        uiLayoutDesc.entries = &uiLayoutEntry;
        uiLayoutDesc.entry_count = 1;

        m_uiBindGroupLayout = hina_create_bind_group_layout(&uiLayoutDesc);
        if (!hina_bind_group_layout_is_valid(m_uiBindGroupLayout)) {
            LOG_ERROR("[GfxRenderer] Failed to create UI bind group layout");
            return false;
        }
        LOG_DEBUG("[GfxRenderer] Created shared UI bind group layout");
    }

    LOG_INFO("[GfxRenderer] Default resources created successfully");
    return true;
}

// ============================================================================
// Feature Registration
// ============================================================================

void GfxRenderer::registerFeature(IRenderFeature* feature) {
    if (!feature) return;

    // Check if already registered
    auto it = std::find(m_registeredFeatures.begin(), m_registeredFeatures.end(), feature);
    if (it != m_registeredFeatures.end()) {
        return;
    }

    LOG_INFO("[GfxRenderer] Registering feature: {}", feature->GetName());
    m_registeredFeatures.push_back(feature);

    // Add to RenderGraph if available
    if (m_renderGraph) {
        // Register feature to get its mask BEFORE adding (so passes get the mask)
        m_renderGraph->RegisterFeature(feature);
        m_renderGraph->AddFeature(feature);
        m_renderGraph->Invalidate(); // Force recompilation
    }
}

void GfxRenderer::unregisterFeature(IRenderFeature* feature) {
    auto it = std::find(m_registeredFeatures.begin(), m_registeredFeatures.end(), feature);
    if (it != m_registeredFeatures.end()) {
        LOG_INFO("[GfxRenderer] Unregistered feature: {}", (*it)->GetName());

        // Remove from RenderGraph if available
        if (m_renderGraph) {
            m_renderGraph->UnregisterFeature(feature);
            m_renderGraph->RemoveFeature(feature);
            m_renderGraph->Invalidate();
        }

        m_registeredFeatures.erase(it);
    }
}

FeatureMask GfxRenderer::getFeatureMask(IRenderFeature* feature) const {
    if (!feature || !m_renderGraph) return 0;
    return m_renderGraph->GetFeatureMask(feature);
}

std::vector<GfxRenderer::RegisteredFeatureInfo> GfxRenderer::getRegisteredFeatures() const {
    std::vector<RegisteredFeatureInfo> result;
    result.reserve(m_registeredFeatures.size());
    for (auto* feature : m_registeredFeatures) {
        FeatureMask mask = m_renderGraph ? m_renderGraph->GetFeatureMask(feature) : 0;
        result.push_back({feature->GetName(), mask});
    }
    return result;
}

// ============================================================================
// IRenderer Interface Implementation
// ============================================================================

uint64_t GfxRenderer::DoCreateFeature(std::function<std::unique_ptr<IRenderFeature>()> factory) {
    uint64_t handle = m_nextFeatureHandle.fetch_add(1, std::memory_order_relaxed);
    auto feature = factory();
    if (!feature) {
        LOG_ERROR("[GfxRenderer] Feature factory returned null");
        return 0; // Invalid handle
    }
    LOG_INFO("[GfxRenderer] Creating feature '{}' with handle {}", feature->GetName(), handle);

    // Get raw pointer before moving into storage
    IRenderFeature* rawFeature = feature.get();

    // Register with RenderGraph - this will call SetupPasses during compilation
    registerFeature(rawFeature);

    // Get the feature mask after registration
    FeatureMask mask = getFeatureMask(rawFeature);

    m_features.emplace(std::piecewise_construct, std::forward_as_tuple(handle),
                       std::forward_as_tuple(std::move(feature), handle, mask));

    return handle;
}

void GfxRenderer::DestroyFeature(uint64_t feature_handle) {
    auto it = m_features.find(feature_handle);
    if (it == m_features.end()) {
        LOG_WARNING("[GfxRenderer] Attempted to destroy non-existent feature handle {}", feature_handle);
        return;
    }
    LOG_INFO("[GfxRenderer] Destroying feature '{}' with handle {}",
             it->second.feature ? it->second.feature->GetName() : "unknown", feature_handle);

    // Unregister from RenderGraph before destroying
    if (it->second.feature) {
        unregisterFeature(it->second.feature.get());
    }

    m_features.erase(it);
}

void* GfxRenderer::GetFeatureParameterBlockPtr(uint64_t feature_handle) {
    auto it = m_features.find(feature_handle);
    if (it == m_features.end()) {
        LOG_WARNING("[GfxRenderer] Requested parameter block for non-existent feature handle {}", feature_handle);
        return nullptr;
    }
    if (!it->second.feature) {
        LOG_WARNING("[GfxRenderer] Feature handle {} has null feature pointer", feature_handle);
        return nullptr;
    }
    return it->second.feature->GetGTParameterBlock_RT();
}

FeatureMask GfxRenderer::GetFeatureMask(uint64_t feature_handle) const {
    auto it = m_features.find(feature_handle);
    if (it == m_features.end()) return 0;
    // Return cached mask, or query from RenderGraph if not cached
    if (it->second.mask != 0) return it->second.mask;
    if (it->second.feature && m_renderGraph) {
        return m_renderGraph->GetFeatureMask(it->second.feature.get());
    }
    return 0;
}

const ToneMappingSettings& GfxRenderer::GetToneMappingSettings() const {
    return m_toneMappingSettings;
}

void GfxRenderer::UpdateToneMappingSettings(const ToneMappingSettings& newSettings) {
    m_toneMappingSettings = newSettings;
}

// ============================================================================
// Surface Lifecycle Management
// ============================================================================

void GfxRenderer::handleSurfaceCreated(void* nativeWindow) {
    if (!m_initialized) {
        LOG_WARNING("[GfxRenderer] handleSurfaceCreated called but renderer not initialized");
        return;
    }

    // Recreate surface after lifecycle event (Android resume from background)
    LOG_INFO("[GfxRenderer] Recreating surface");
    if (!hina_recreate_surface(nativeWindow, nullptr)) {
        LOG_ERROR("[GfxRenderer] Failed to recreate surface");
        return;
    }

    // Resize to current dimensions
    onResize(m_width, m_height);
    LOG_INFO("[GfxRenderer] Surface recreated: {}x{}", m_width, m_height);
}

void GfxRenderer::handleSurfaceDestroyed() {
    LOG_INFO("[GfxRenderer] Surface destroyed - cleaning up swapchain resources");
    // The hina-vk backend handles swapchain cleanup internally when surface is lost
    // No explicit cleanup needed here as the swapchain will be recreated on next surface
}
