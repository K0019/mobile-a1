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
#include "renderer/ui/ui_primitives.h"
// EnTT integration disabled - using internal ECS
// #include "ecs/Components/Core.h"
// #include <entt/entt.hpp>
#include <algorithm>
#include <cstring>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Use types from gfx namespace in this implementation file
using namespace gfx;

// ============================================================================
// LightTileBinner Implementation
// ============================================================================

void LightTileBinner::resize(uint32_t screenWidth, uint32_t screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_tileCountX = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
    m_tileCountY = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;
    m_tiles.resize(m_tileCountX * m_tileCountY);
    clear();

    LOG_INFO("[GfxRenderer] LightTileBinner resized: {}x{} tiles ({}x{} pixels)",
             m_tileCountX, m_tileCountY, screenWidth, screenHeight);
}

void LightTileBinner::clear() {
    for (auto& tile : m_tiles) {
        tile.lightMask = 0;
        tile.zMin = 1.0f;
        tile.zMax = 0.0f;
    }
}

void LightTileBinner::addLight(uint32_t lightIndex, const glm::vec4& screenBounds, float zMin, float zMax) {
    if (lightIndex >= MAX_LIGHTS) return;

    // Convert screen bounds to tile bounds
    uint32_t minTileX = static_cast<uint32_t>(glm::max(0.0f, screenBounds.x)) / TILE_SIZE;
    uint32_t minTileY = static_cast<uint32_t>(glm::max(0.0f, screenBounds.y)) / TILE_SIZE;
    uint32_t maxTileX = glm::min(m_tileCountX - 1, static_cast<uint32_t>(screenBounds.z) / TILE_SIZE);
    uint32_t maxTileY = glm::min(m_tileCountY - 1, static_cast<uint32_t>(screenBounds.w) / TILE_SIZE);

    uint64_t lightBit = 1ULL << lightIndex;

    for (uint32_t ty = minTileY; ty <= maxTileY; ++ty) {
        for (uint32_t tx = minTileX; tx <= maxTileX; ++tx) {
            auto& tile = m_tiles[ty * m_tileCountX + tx];
            tile.lightMask |= lightBit;
            tile.zMin = glm::min(tile.zMin, zMin);
            tile.zMax = glm::max(tile.zMax, zMax);
        }
    }
}

uint64_t LightTileBinner::getTileMask(uint32_t tileX, uint32_t tileY) const {
    if (tileX >= m_tileCountX || tileY >= m_tileCountY) return 0;
    return m_tiles[tileY * m_tileCountX + tileX].lightMask;
}

// ============================================================================
// ShadowAtlasAllocator Implementation
// ============================================================================

void ShadowAtlasAllocator::init(uint32_t atlasSize) {
    m_atlasSize = atlasSize;
    m_nextY = 0;
    m_regions.clear();
    LOG_INFO("[GfxRenderer] Shadow atlas initialized: {}x{}", atlasSize, atlasSize);
}

ShadowAtlasAllocator::Region ShadowAtlasAllocator::allocate(uint32_t size, uint32_t lightIndex, bool octahedral) {
    // Simple row-based allocation (can be improved with more sophisticated packing)
    Region region{};
    region.lightIndex = lightIndex;
    region.size = size;
    region.isOctahedral = octahedral;

    // Find space in current row or start new row
    uint32_t x = 0;
    for (const auto& existing : m_regions) {
        if (existing.y == m_nextY) {
            x = existing.x + existing.size;
        }
    }

    if (x + size > m_atlasSize) {
        // Start new row
        m_nextY += size;
        x = 0;
    }

    if (m_nextY + size > m_atlasSize) {
        LOG_WARNING("[GfxRenderer] Shadow atlas full!");
        return region;
    }

    region.x = x;
    region.y = m_nextY;
    m_regions.push_back(region);

    LOG_DEBUG("[GfxRenderer] Allocated shadow region: light={}, pos=({},{}), size={}, octahedral={}",
              lightIndex, region.x, region.y, size, octahedral);

    return region;
}

void ShadowAtlasAllocator::free(uint32_t lightIndex) {
    m_regions.erase(
        std::remove_if(m_regions.begin(), m_regions.end(),
            [lightIndex](const Region& r) { return r.lightIndex == lightIndex; }),
        m_regions.end());
}

void ShadowAtlasAllocator::reset() {
    m_regions.clear();
    m_nextY = 0;
}

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

std::vector<VisibleObject> CPUCuller::cullAndSort(
    const std::vector<VisibleObject>& objects,
    const glm::vec3& cameraPos,
    bool frontToBack) const
{
    std::vector<VisibleObject> result;
    result.reserve(objects.size());

    // For now, just copy all objects (actual culling would need AABB data)
    for (const auto& obj : objects) {
        result.push_back(obj);
    }

    // Sort by sort key
    if (frontToBack) {
        std::sort(result.begin(), result.end(),
            [](const VisibleObject& a, const VisibleObject& b) {
                // Sort by PSO first (minimize state changes), then by distance
                if (a.psoHandle != b.psoHandle) return a.psoHandle < b.psoHandle;
                if (a.materialBGHandle != b.materialBGHandle) return a.materialBGHandle < b.materialBGHandle;
                return a.sortKey < b.sortKey;
            });
    } else {
        std::sort(result.begin(), result.end(),
            [](const VisibleObject& a, const VisibleObject& b) {
                return a.sortKey > b.sortKey;
            });
    }

    return result;
}

// ============================================================================
// hina-vk logging callback
// ============================================================================

static void hinaLogCallback(const char* msg) {
    // Use both stderr (immediate) and LOG_INFO (file)
    fprintf(stderr, "[GFX] %s\n", msg);
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
    desc.flags = HINA_INIT_VALIDATION_BIT;
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

    // Create rendering pipelines
    if (!createPipelines()) {
        LOG_ERROR("[GfxRenderer] Failed to create pipelines");
        return false;
    }

    // Initialize CPU systems
    m_lightBinner.resize(width, height);
    m_shadowAllocator.init(2048); // 2K shadow atlas

    // Initialize HinaContext for vk::IContext interface (used by RenderGraph/features)
    m_hinaContext = std::make_unique<HinaContext>();
    if (!m_hinaContext->initialize(nativeWindow, width, height)) {
        LOG_ERROR("[GfxRenderer] Failed to initialize HinaContext");
        return false;
    }
    LOG_INFO("[GfxRenderer] HinaContext initialized");

    // Create RenderGraph for pass management
    // Note: We pass nullptr for GPUBuffers since we use GfxMeshStorage/GfxMaterialSystem
    // for mesh/material storage, which are already integrated with ResourceManager
    m_renderGraph = std::make_unique<RenderGraph>(nullptr);
    m_renderGraph->SetGfxRenderer(this);  // Allow features to access renderer resources
    LOG_INFO("[GfxRenderer] RenderGraph created");

    m_initialized = true;
    LOG_INFO("[GfxRenderer] Initialization complete");
    return true;
}

void GfxRenderer::shutdown() {
    LOG_INFO("[GfxRenderer] Shutting down...");

    if (!m_initialized) return;

    // Shutdown UI systems first
    shutdownImGui();
    shutdownUI2D();

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
        if (hina_buffer_is_valid(frame.lightingDataUBO)) {
            hina_destroy_buffer(frame.lightingDataUBO);
        }
        if (hina_buffer_is_valid(frame.transformRingUBO)) {
            hina_destroy_buffer(frame.transformRingUBO);
        }
        if (hina_buffer_is_valid(frame.objectDataRingUBO)) {
            hina_destroy_buffer(frame.objectDataRingUBO);
        }
    }

    // Destroy composite pass resources
    if (hina_bind_group_is_valid(m_compositeBindGroup)) {
        hina_destroy_bind_group(m_compositeBindGroup);
    }
    if (hina_buffer_is_valid(m_fullscreenQuadVB)) {
        hina_destroy_buffer(m_fullscreenQuadVB);
    }

    // Destroy G-buffer scene resources
    if (hina_bind_group_is_valid(m_gbufferSceneBindGroup)) {
        hina_destroy_bind_group(m_gbufferSceneBindGroup);
    }
    if (hina_buffer_is_valid(m_gbufferFrameUBO)) {
        hina_destroy_buffer(m_gbufferFrameUBO);
    }

    // Destroy pipelines
    if (hina_pipeline_is_valid(m_gbufferPipeline)) {
        hina_destroy_pipeline(m_gbufferPipeline);
    }
    if (hina_pipeline_is_valid(m_compositePassPipeline)) {
        hina_destroy_pipeline(m_compositePassPipeline);
    }
    if (hina_pipeline_is_valid(m_shadowPipeline)) {
        hina_destroy_pipeline(m_shadowPipeline);
    }
    if (hina_pipeline_is_valid(m_lightingPipeline)) {
        hina_destroy_pipeline(m_lightingPipeline);
    }
    if (hina_pipeline_is_valid(m_wboitAccumPipeline)) {
        hina_destroy_pipeline(m_wboitAccumPipeline);
    }
    if (hina_pipeline_is_valid(m_wboitResolvePipeline)) {
        hina_destroy_pipeline(m_wboitResolvePipeline);
    }
    if (hina_pipeline_is_valid(m_postProcessPipeline)) {
        hina_destroy_pipeline(m_postProcessPipeline);
    }
    if (hina_pipeline_is_valid(m_gridPipeline)) {
        hina_destroy_pipeline(m_gridPipeline);
    }

    // Destroy grid resources
    if (hina_bind_group_is_valid(m_gridBindGroup)) {
        hina_destroy_bind_group(m_gridBindGroup);
    }
    if (hina_bind_group_layout_is_valid(m_gridLayout)) {
        hina_destroy_bind_group_layout(m_gridLayout);
    }
    if (hina_buffer_is_valid(m_gridUBO)) {
        hina_destroy_buffer(m_gridUBO);
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

    // Destroy render targets - HDR and shadow
    if (hina_texture_is_valid(m_hdrTarget)) {
        hina_destroy_texture(m_hdrTarget);
    }
    if (hina_texture_is_valid(m_shadowAtlas)) {
        hina_destroy_texture(m_shadowAtlas);
    }
    if (hina_texture_is_valid(m_ambientOctMap)) {
        hina_destroy_texture(m_ambientOctMap);
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

    // Set 1: Material (textures only - material constants via push constants)
    {
        hina_bind_group_layout_entry entries[] = {
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
        LOG_DEBUG("[GfxRenderer] Created material layout (Set 1)");
    }

    // Set 2: Dynamic (with dynamic offset flag for per-draw transforms)
    {
        hina_bind_group_layout_entry entries[] = {
            { DynamicBindings::Transform, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_VERTEX, 1, HINA_BINDING_FLAG_DYNAMIC_OFFSET },
            { DynamicBindings::ObjectData, HINA_DESC_TYPE_UNIFORM_BUFFER, HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT, 1, HINA_BINDING_FLAG_DYNAMIC_OFFSET },
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
        LOG_DEBUG("[GfxRenderer] Created dynamic layout (Set 2)");
    }

    LOG_INFO("[GfxRenderer] Bind group layouts created successfully");
    return true;
}

bool GfxRenderer::createRenderTargets() {
    // Use internal render resolution for G-buffer (must match SCENE_DEPTH from RenderGraph)
    const uint32_t gbufferWidth = RenderResources::INTERNAL_WIDTH;
    const uint32_t gbufferHeight = RenderResources::INTERNAL_HEIGHT;
    LOG_INFO("[GfxRenderer] Creating render targets at internal resolution {}x{}...", gbufferWidth, gbufferHeight);

    // Common render target usage flags
    constexpr auto rtUsage = static_cast<hina_texture_usage_flags>(
        HINA_TEXTURE_RENDER_TARGET_BIT | HINA_TEXTURE_SAMPLED_BIT);

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
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R32_UINT;
        desc.usage = rtUsage;

        m_gbuffer.visibilityID = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_gbuffer.visibilityID)) {
            LOG_ERROR("[GfxRenderer] Failed to create visibility ID texture");
            return false;
        }
        m_gbuffer.visibilityIDView = hina_texture_get_default_view(m_gbuffer.visibilityID);
        LOG_DEBUG("[GfxRenderer] Created GBuffer_VisibilityID (R32_UINT)");
    }

    // HDR Target (R16G16B16A16_SFLOAT = 64 bits)
    {
        hina_texture_desc desc = {};
        desc.width = gbufferWidth;
        desc.height = gbufferHeight;
        desc.format = HINA_FORMAT_R16G16B16A16_SFLOAT;
        desc.usage = rtUsage;

        m_hdrTarget = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_hdrTarget)) {
            LOG_ERROR("[GfxRenderer] Failed to create HDR target");
            return false;
        }
        m_hdrTargetView = hina_texture_get_default_view(m_hdrTarget);
        LOG_DEBUG("[GfxRenderer] Created HDR_Target (R16G16B16A16_SFLOAT)");
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

    // Shadow Atlas (D16, 2048x2048)
    {
        hina_texture_desc desc = {};
        desc.width = 2048;
        desc.height = 2048;
        desc.format = HINA_FORMAT_D16_UNORM;
        desc.usage = rtUsage;

        m_shadowAtlas = hina_make_texture(&desc);
        if (!hina_texture_is_valid(m_shadowAtlas)) {
            LOG_ERROR("[GfxRenderer] Failed to create shadow atlas");
            return false;
        }
        m_shadowAtlasView = hina_texture_get_default_view(m_shadowAtlas);
        LOG_DEBUG("[GfxRenderer] Created ShadowAtlas (D16_UNORM, 2048x2048)");
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
    desc.format = HINA_FORMAT_B8G8R8A8_UNORM;  // UNORM for manual gamma control in tonemapping
    // ViewOutput is used both as render target (by legacy rendering code) and as sampled texture (by ImGui)
    // Note: hina-vk only adds TRANSFER_DST for non-render-targets, so blit to ViewOutput requires a render pass
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

        // Buffer flags for host-visible mapped UBOs
        constexpr hina_buffer_flags uboFlags = static_cast<hina_buffer_flags>(
            HINA_BUFFER_UNIFORM_BIT |
            HINA_BUFFER_HOST_VISIBLE_BIT |
            HINA_BUFFER_HOST_COHERENT_BIT);

        // Frame constants UBO
        {
            hina_buffer_desc desc = {};
            desc.size = sizeof(FrameConstants);
            desc.flags = uboFlags;

            frame.frameConstantsUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.frameConstantsUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create frame constants UBO for frame {}", i);
                return false;
            }
        }

        // Lighting data UBO (64 lights * sizeof(TileLight) + tile data)
        {
            hina_buffer_desc desc = {};
            desc.size = MAX_LIGHTS * sizeof(TileLight) + 4096; // Extra for tile data
            desc.flags = uboFlags;

            frame.lightingDataUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.lightingDataUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create lighting data UBO for frame {}", i);
                return false;
            }
        }

        // Transform ring buffer (dynamic offsets)
        {
            hina_buffer_desc desc = {};
            desc.size = TRANSFORM_UBO_SIZE;
            desc.flags = uboFlags;

            frame.transformRingUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.transformRingUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create transform ring UBO for frame {}", i);
                return false;
            }
        }

        // Object data ring buffer
        {
            hina_buffer_desc desc = {};
            desc.size = MAX_TRANSFORMS * 64; // 64 bytes per object data
            desc.flags = uboFlags;

            frame.objectDataRingUBO = hina_make_buffer(&desc);
            if (!hina_buffer_is_valid(frame.objectDataRingUBO)) {
                LOG_ERROR("[GfxRenderer] Failed to create object data ring UBO for frame {}", i);
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
    frame.objectDataRingOffset = 0;

    // Save swapchain image for this frame
    frame.swapchainImage = swap;
    frame.swapchainView = hina_texture_get_default_view(swap.texture);

    // Clear visible lists
    m_visibleOpaque.clear();
    m_visibleTransparent.clear();

    // Reset light binner
    m_lightBinner.clear();

    // Note: Draw queue is cleared at the END of the frame (in endFrame()),
    // not here, because submitMesh/submitEcsEntities may be called
    // before beginFrame() in the engine update loop.

    return true;
}

void GfxRenderer::render(FrameData& frameData) {
    if (!m_initialized) {
        return;
    }

    // Store frame data pointer for use in render passes
    m_currentFrameData = &frameData;

    auto& frame = m_frames[m_currentFrame];

    // Update frame constants
    updateFrameConstants(frameData);

    // CPU culling
    cullScene(frameData);

    // Light binning
    binLights(frameData);

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

        // Import active mesh chunks for RenderGraph resource tracking
        m_meshStorage.importChunksToRenderGraph(*m_renderGraph);

        // Begin new frame for RenderGraph (must be called once per frame, before Execute)
        m_renderGraph->BeginFrame();

        // Pass the active ViewOutput to the RenderGraph so it can copy SCENE_COLOR to it
        const auto& activeViewOutput = m_viewOutputs[static_cast<size_t>(m_activeViewId)];
        ViewOutputConfig outputConfig{
            .resolvedColor = activeViewOutput.valid ? activeViewOutput.texture : gfx::Texture{},
            .swapchainImage = frame.swapchainImage.texture
        };

        // Execute all registered features through RenderGraph
        // RenderGraph will copy SCENE_COLOR to VIEW_OUTPUT for ImGui viewports
        m_renderGraph->Execute(m_hinaContext->getCommandBuffer().getHinaCmd(), frameData, outputConfig);

        // Transition swapchain to PRESENT layout after all rendering is complete
        hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_PRESENT);
    }

    // Update any dirty material bind groups before rendering
    m_materialSystem.updateDirtyMaterials();

    // All rendering is now handled by the RenderGraph:
    // - SceneRenderFeature: G-buffer pass (ExecuteGBufferPass) + Composite pass to SCENE_COLOR
    // - GridFeature: Grid overlay on SCENE_COLOR
    // - UI2DRenderFeature: 2D UI overlay on SCENE_COLOR
    // - RenderGraph: ResolveViewOutput (SCENE_COLOR -> VIEW_OUTPUT), FinalBlit (SCENE_COLOR -> swapchain)
    // - ImGuiRenderFeature: ImGui overlay on swapchain (samples VIEW_OUTPUT for viewport)

    // Initialize UI systems if needed (deferred until first frame)
    if (!m_imguiInitialized && ImGui::GetCurrentContext() != nullptr) {
        initImGui();
    }
    if (!m_ui2dInitialized) {
        initUI2D();
    }

    // Submit
    hina_frame_submit(frame.cmd);
}

void GfxRenderer::render(RenderFrameData& frameData) {
    if (!m_initialized) {
        return;
    }

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
            // - resolvedColor: The view's output texture (for ImGui to sample)
            // - swapchainImage: Only set for the presented view
            bool isPresentedView = (viewFrameData.viewId == frameData.presentedViewId);
            ViewOutputConfig outputConfig{
                .resolvedColor = viewOutput.valid ? viewOutput.texture : gfx::Texture{},
                .swapchainImage = isPresentedView ? frame.swapchainImage.texture : gfx::Texture{}
            };

            // Update per-frame constants for this view
            m_currentFrameData = &viewFrameData;
            updateFrameConstants(viewFrameData);

            // CPU culling and light binning for this view
            cullScene(viewFrameData);
            binLights(viewFrameData);

            // Execute RenderGraph with this view's feature mask
            // The RenderGraph will filter passes based on viewFrameData.featureMask
            m_renderGraph->Execute(m_hinaContext->getCommandBuffer().getHinaCmd(), viewFrameData, outputConfig);

            // Store output info back to view for application to access
            if (viewOutput.valid) {
                viewFrameData.outputInfo.colorTextureId = getViewOutputTextureId(viewId);
            }
        }

        // Transition swapchain to PRESENT layout after all views are rendered
        hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_PRESENT);
    }

    // Update any dirty material bind groups before rendering
    m_materialSystem.updateDirtyMaterials();

    // Initialize UI systems if needed (deferred until first frame)
    if (!m_imguiInitialized && ImGui::GetCurrentContext() != nullptr) {
        initImGui();
    }
    if (!m_ui2dInitialized) {
        initUI2D();
    }

    // Submit
    hina_frame_submit(frame.cmd);
}

void GfxRenderer::endFrame() {
    if (!m_initialized) return;

    hina_frame_end();
    m_currentFrame = (m_currentFrame + 1) % GFX_MAX_FRAMES;

    // Clear draw queue at end of frame, ready for next frame's submissions
    clearDrawQueue();
}

void GfxRenderer::onResize(uint32_t width, uint32_t height) {
    if (width == m_width && height == m_height) return;

    LOG_INFO("[GfxRenderer] Resizing from {}x{} to {}x{}", m_width, m_height, width, height);

    // TODO: Need to wait for GPU - may need to call hina_frame_begin/end to sync
    // or expose a wait function in hina-vk

    m_width = width;
    m_height = height;

    // Recreate render targets
    // TODO: Destroy old targets first
    createRenderTargets();

    // Resize light binner
    m_lightBinner.resize(width, height);
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
    void* mapped = hina_map_buffer(frame.frameConstantsUBO);
    if (mapped) {
        std::memcpy(mapped, &constants, sizeof(constants));
        // Note: HOST_COHERENT means no flush needed, writes are visible to GPU
    } else {
        LOG_WARNING("[GfxRenderer] Failed to map frame constants buffer");
    }
}

void GfxRenderer::cullScene(const FrameData& frameData) {
    // TODO: Implement actual scene culling
    // For now, this is a stub
}

void GfxRenderer::binLights(const FrameData& frameData) {
    // TODO: Implement light binning
    // For now, this is a stub
}

bool GfxRenderer::createPipelines() {
    LOG_INFO("[GfxRenderer] Creating pipelines...");

    // G-Buffer pipeline shader - viewProj in UBO, per-draw data in push constants
    // Uses split vertex streams: Position (12 bytes) + Attributes (12 bytes packed)
    // Set 0: Frame UBO, Set 1: Material textures
    const char* gbuffer_shader = R"(
#hina
group Frame = 0;
group Material = 1;

bindings(Frame, start=0) {
  uniform(std140) FrameUBO {
    mat4 viewProj;
  } frame;
}

bindings(Material, start=1) {
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_metallicRoughness;
  texture sampler2D u_emissive;
  texture sampler2D u_occlusion;
}

push_constant PushConstants {
  mat4 model;
  vec4 baseColor;
} pc;

// Split vertex streams (24 bytes total):
// Buffer 0: Position stream (12 bytes)
// Buffer 1: Attribute stream (12 bytes packed)
struct VertexIn {
  vec3 a_position;    // location 0, buffer 0: position
  uint a_normalMatID; // location 1, buffer 1: RGB10A2 octahedral normal + 2-bit matID
  uint a_tangentSign; // location 2, buffer 1: RGB10A2 octahedral tangent + sign
  uint a_uv;          // location 3, buffer 1: RG16_UNORM UV
};

struct Varyings {
  vec3 worldPos;
  vec3 normal;
  vec3 tangent;
  float bitangentSign;
  vec2 uv;
};

struct FragOut {
  vec4 albedo;       // location 0
  vec4 normal;       // location 1
  vec4 materialData; // location 2
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;

    // Position from stream 0
    vec4 worldPos = pc.model * vec4(in.a_position, 1.0);
    out.worldPos = worldPos.xyz;

    // Inline octahedral decode helper
    // Unpack normal from RGB10A2 octahedral encoding
    float nx = float(in.a_normalMatID & 0x3FFu) / 1023.0;
    float ny = float((in.a_normalMatID >> 10u) & 0x3FFu) / 1023.0;
    vec2 nf = vec2(nx, ny) * 2.0 - 1.0;
    vec3 localNormal = vec3(nf.xy, 1.0 - abs(nf.x) - abs(nf.y));
    float nt = max(-localNormal.z, 0.0);
    localNormal.x += (localNormal.x >= 0.0) ? -nt : nt;
    localNormal.y += (localNormal.y >= 0.0) ? -nt : nt;
    localNormal = normalize(localNormal);

    // Unpack tangent from RGB10A2 octahedral encoding
    float tx = float(in.a_tangentSign & 0x3FFu) / 1023.0;
    float ty = float((in.a_tangentSign >> 10u) & 0x3FFu) / 1023.0;
    vec2 tf = vec2(tx, ty) * 2.0 - 1.0;
    vec3 localTangent = vec3(tf.xy, 1.0 - abs(tf.x) - abs(tf.y));
    float tt = max(-localTangent.z, 0.0);
    localTangent.x += (localTangent.x >= 0.0) ? -tt : tt;
    localTangent.y += (localTangent.y >= 0.0) ? -tt : tt;
    localTangent = normalize(localTangent);

    // Extract sign bit for bitangent
    uint signBit = (in.a_tangentSign >> 30u) & 0x3u;

    mat3 normalMatrix = mat3(pc.model);
    out.normal = normalMatrix * localNormal;
    out.tangent = normalMatrix * localTangent;
    out.bitangentSign = (signBit & 1u) != 0u ? -1.0 : 1.0;

    // Unpack UV from RG16_UNORM
    out.uv = unpackUnorm2x16(in.a_uv);

    gl_Position = frame.viewProj * worldPos;
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;

    // Sample albedo texture
    vec4 albedo = texture(u_albedo, in.uv) * pc.baseColor;

    // Sample normal map and construct TBN
    vec3 N = normalize(in.normal);
    vec3 T = normalize(in.tangent);
    vec3 B = cross(N, T) * in.bitangentSign;
    mat3 TBN = mat3(T, B, N);

    vec3 normalMap = texture(u_normal, in.uv).rgb * 2.0 - 1.0;
    N = normalize(TBN * normalMap);

    // Sample metallic/roughness (B=metallic, G=roughness in glTF convention)
    vec2 mr = texture(u_metallicRoughness, in.uv).bg;
    float metallic = mr.x;
    float roughness = mr.y;

    // Sample occlusion (R channel)
    float ao = texture(u_occlusion, in.uv).r;

    // Output albedo from material texture
    out.albedo = albedo;

    // Encode world-space normal to [0,1] range for storage
    out.normal = vec4(N.xy * 0.5 + 0.5, 0.0, 1.0);

    // Material data: roughness, metallic, ao, emissive intensity
    out.materialData = vec4(roughness, metallic, ao, 0.0);

    return out;
}
#hina_end
)";

    char* error = nullptr;
    hina_hsl_module* module = hslc_compile_hsl_source(gbuffer_shader, "gbuffer_shader", &error);
    if (!module) {
        LOG_ERROR("[GfxRenderer] G-buffer shader compilation failed: {}", error ? error : "Unknown");
        if (error) hslc_free_log(error);
        return false;
    }

    // Create EXPLICIT bind group layout for G-buffer scene (set 0: UBO at binding 0)
    // This bypasses reflection-based layout generation to avoid descriptor type mismatches
    hina_bind_group_layout_entry gbuffer_layout_entries[1] = {};
    gbuffer_layout_entries[0].binding = 0;
    gbuffer_layout_entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
    gbuffer_layout_entries[0].count = 1;
    gbuffer_layout_entries[0].stage_flags = HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT;
    gbuffer_layout_entries[0].flags = HINA_BINDING_FLAG_NONE;

    hina_bind_group_layout_desc gbuffer_layout_desc = {};
    gbuffer_layout_desc.entries = gbuffer_layout_entries;
    gbuffer_layout_desc.entry_count = 1;
    gbuffer_layout_desc.label = "gbuffer_scene_layout";

    m_gbufferSceneLayout = hina_create_bind_group_layout(&gbuffer_layout_desc);
    if (!hina_bind_group_layout_is_valid(m_gbufferSceneLayout)) {
        LOG_ERROR("[GfxRenderer] Failed to create G-buffer scene bind group layout");
        hslc_hsl_module_free(module);
        return false;
    }

    // Vertex layout for G-buffer pass - split streams (Position 12 bytes + Attributes 12 bytes)
    hina_vertex_layout vertex_layout = {};
    vertex_layout.buffer_count = 2;
    // Buffer 0: Position stream (12 bytes stride)
    vertex_layout.buffer_strides[0] = sizeof(gfx::VertexPosition); // 12 bytes
    vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
    // Buffer 1: Attribute stream (12 bytes stride)
    vertex_layout.buffer_strides[1] = sizeof(gfx::VertexAttributes); // 12 bytes
    vertex_layout.input_rates[1] = HINA_VERTEX_INPUT_RATE_VERTEX;
    vertex_layout.attr_count = 4;
    // Buffer 0 attributes
    vertex_layout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };  // position (vec3)
    // Buffer 1 attributes (packed uint32s)
    vertex_layout.attrs[1] = { HINA_FORMAT_R32_UINT, 0, 1, 1 };   // normalMatID (uint)
    vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, 4, 2, 1 };   // tangentSign (uint)
    vertex_layout.attrs[3] = { HINA_FORMAT_R32_UINT, 8, 3, 1 };   // uv (uint)

    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.layout = vertex_layout;
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;  // DEBUG: Disable culling to rule out winding issues
    pip_desc.depth.depth_test = true;
    pip_desc.depth.depth_write = true;
    pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
    pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
    pip_desc.color_attachment_count = 3;
    pip_desc.color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;   // albedo
    pip_desc.color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;   // normal
    pip_desc.color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;  // materialData
    // Use explicit bind group layouts (production path)
    // Set 0: Frame UBO, Set 1: Material textures
    pip_desc.bind_group_layouts[0] = m_gbufferSceneLayout;
    pip_desc.bind_group_layouts[1] = m_materialLayout;
    pip_desc.bind_group_layout_count = 2;

    m_gbufferPipeline = hina_make_pipeline_from_module(module, &pip_desc, nullptr);
    hslc_hsl_module_free(module);

    if (!hina_pipeline_is_valid(m_gbufferPipeline)) {
        LOG_ERROR("[GfxRenderer] G-buffer pipeline creation failed");
        return false;
    }

    // Create per-frame UBO for viewProj (64 bytes)
    hina_buffer_desc ubo_desc = {};
    ubo_desc.size = sizeof(glm::mat4);
    ubo_desc.flags = (hina_buffer_flags)(HINA_BUFFER_UNIFORM_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
    m_gbufferFrameUBO = hina_make_buffer(&ubo_desc);
    if (!hina_buffer_is_valid(m_gbufferFrameUBO)) {
        LOG_ERROR("[GfxRenderer] Failed to create G-buffer frame UBO");
        return false;
    }
    m_gbufferFrameUBOMapped = hina_map_buffer(m_gbufferFrameUBO);
    if (!m_gbufferFrameUBOMapped) {
        LOG_ERROR("[GfxRenderer] Failed to map G-buffer frame UBO");
        return false;
    }

    // Create persistent bind group for the frame UBO
    hina_bind_group_entry scene_entry = {};
    scene_entry.binding = 0;
    scene_entry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
    scene_entry.buffer.buffer = m_gbufferFrameUBO;
    scene_entry.buffer.offset = 0;
    scene_entry.buffer.size = sizeof(glm::mat4);

    hina_bind_group_desc scene_bg_desc = {};
    scene_bg_desc.layout = m_gbufferSceneLayout;
    scene_bg_desc.entries = &scene_entry;
    scene_bg_desc.entry_count = 1;
    scene_bg_desc.label = "gbuffer_scene";

    m_gbufferSceneBindGroup = hina_create_bind_group(&scene_bg_desc);
    if (!hina_bind_group_is_valid(m_gbufferSceneBindGroup)) {
        LOG_ERROR("[GfxRenderer] Failed to create G-buffer scene bind group");
        return false;
    }

    LOG_INFO("[GfxRenderer] G-buffer pipeline created successfully");

    // Deferred lighting/composite pass shader
    const char* composite_shader = R"(
#hina
group Composite = 0;

bindings(Composite, start=0) {
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_materialData;
  texture sampler2D u_depth;
}

push_constant LightingParams {
  vec4 lightDir;
  vec4 lightColor;
  vec4 ambientColor;
  vec4 cameraPos;
  mat4 invViewProj;
} pc;

struct VertexIn {
  vec2 a_position;
  vec2 a_uv;
};

struct Varyings {
  vec2 uv;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;
    out.uv = in.a_uv;
    gl_Position = vec4(in.a_position, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;

    // G-buffer is Y-flipped due to Vulkan projection flip - flip UV for all G-buffer samples
    vec2 gbufferUV = vec2(in.uv.x, 1.0 - in.uv.y);

    // Sample G-buffer with flipped UV
    vec4 albedoSample = texture(u_albedo, gbufferUV);
    vec2 encodedNormal = texture(u_normal, gbufferUV).xy;
    vec4 matData = texture(u_materialData, gbufferUV);
    float depth = texture(u_depth, gbufferUV).r;

    // Early out for sky/background (depth == 1.0)
    if (depth >= 1.0) {
        out.color = vec4(0.1, 0.1, 0.15, 1.0);  // Sky color
        return out;
    }

    // Decode normal from octahedral (simple encoding for now)
    vec3 normal = vec3(encodedNormal * 2.0 - 1.0, 0.0);
    normal.z = sqrt(max(0.0, 1.0 - dot(normal.xy, normal.xy)));
    normal = normalize(normal);

    // Extract material properties
    float roughness = matData.r;
    float metallic = matData.g;

    // Reconstruct world position from depth (use flipped UV, then convert to NDC)
    vec2 ndc = gbufferUV * 2.0 - 1.0;
    ndc.y = -ndc.y;  // Vulkan NDC flip
    vec4 clipPos = vec4(ndc, depth, 1.0);
    vec4 worldPos4 = pc.invViewProj * clipPos;
    vec3 worldPos = worldPos4.xyz / worldPos4.w;

    // View direction
    vec3 viewDir = normalize(pc.cameraPos.xyz - worldPos);

    // Directional light
    vec3 lightDir = normalize(pc.lightDir.xyz);
    float lightIntensity = pc.lightDir.w;

    // Diffuse lighting (Lambert)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = pc.lightColor.rgb * NdotL * lightIntensity;

    // Simple specular (Blinn-Phong)
    vec3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVec), 0.0);
    float specPower = mix(8.0, 128.0, 1.0 - roughness);
    float spec = pow(NdotH, specPower) * (1.0 - roughness) * 0.5;
    vec3 specular = pc.lightColor.rgb * spec * lightIntensity;

    // Combine lighting
    vec3 ambient = pc.ambientColor.rgb;
    vec3 baseColor = albedoSample.rgb;  // Use albedo from G-buffer

    vec3 finalColor = baseColor * (ambient + diffuse) + specular;

    // No tonemapping/gamma here - will be handled in dedicated pass later
    out.color = vec4(finalColor, 1.0);

    return out;
}
#hina_end
)";

    module = hslc_compile_hsl_source(composite_shader, "composite_shader", &error);
    if (!module) {
        LOG_ERROR("[GfxRenderer] Composite shader compilation failed: {}", error ? error : "Unknown");
        if (error) hslc_free_log(error);
        return false;
    }

    // Create EXPLICIT bind group layout for composite pass (set 0: 4 textures)
    hina_bind_group_layout_entry composite_layout_entries[4] = {};
    composite_layout_entries[0].binding = 0;  // albedo
    composite_layout_entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    composite_layout_entries[0].count = 1;
    composite_layout_entries[0].stage_flags = HINA_STAGE_FRAGMENT;
    composite_layout_entries[0].flags = HINA_BINDING_FLAG_NONE;

    composite_layout_entries[1].binding = 1;  // normal
    composite_layout_entries[1].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    composite_layout_entries[1].count = 1;
    composite_layout_entries[1].stage_flags = HINA_STAGE_FRAGMENT;
    composite_layout_entries[1].flags = HINA_BINDING_FLAG_NONE;

    composite_layout_entries[2].binding = 2;  // materialData
    composite_layout_entries[2].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    composite_layout_entries[2].count = 1;
    composite_layout_entries[2].stage_flags = HINA_STAGE_FRAGMENT;
    composite_layout_entries[2].flags = HINA_BINDING_FLAG_NONE;

    composite_layout_entries[3].binding = 3;  // depth
    composite_layout_entries[3].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    composite_layout_entries[3].count = 1;
    composite_layout_entries[3].stage_flags = HINA_STAGE_FRAGMENT;
    composite_layout_entries[3].flags = HINA_BINDING_FLAG_NONE;

    hina_bind_group_layout_desc composite_layout_desc = {};
    composite_layout_desc.entries = composite_layout_entries;
    composite_layout_desc.entry_count = 4;
    composite_layout_desc.label = "composite_layout";

    m_compositeLayout = hina_create_bind_group_layout(&composite_layout_desc);
    if (!hina_bind_group_layout_is_valid(m_compositeLayout)) {
        LOG_ERROR("[GfxRenderer] Failed to create composite bind group layout");
        hslc_hsl_module_free(module);
        return false;
    }

    // Fullscreen quad vertex layout
    hina_vertex_layout fs_vertex_layout = {};
    fs_vertex_layout.buffer_count = 1;
    fs_vertex_layout.buffer_strides[0] = sizeof(float) * 4; // pos(2) + uv(2)
    fs_vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
    fs_vertex_layout.attr_count = 2;
    fs_vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, 0, 0, 0 };  // position
    fs_vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, sizeof(float) * 2, 1, 0 };  // uv

    hina_hsl_pipeline_desc composite_pip = hina_hsl_pipeline_desc_default();
    composite_pip.layout = fs_vertex_layout;
    composite_pip.cull_mode = HINA_CULL_MODE_NONE;
    composite_pip.depth.depth_test = false;
    composite_pip.depth.depth_write = false;
    composite_pip.depth_format = HINA_FORMAT_UNDEFINED;
    // Explicitly set color format to match view output texture (BGRA UNORM for manual gamma)
    composite_pip.color_formats[0] = HINA_FORMAT_B8G8R8A8_UNORM;
    composite_pip.color_attachment_count = 1;
    // Use explicit bind group layout (production path)
    composite_pip.bind_group_layouts[0] = m_compositeLayout;
    composite_pip.bind_group_layout_count = 1;

    m_compositePassPipeline = hina_make_pipeline_from_module(module, &composite_pip, nullptr);
    hslc_hsl_module_free(module);

    if (!hina_pipeline_is_valid(m_compositePassPipeline)) {
        LOG_ERROR("[GfxRenderer] Composite pipeline creation failed");
        return false;
    }

    LOG_INFO("[GfxRenderer] Composite pipeline created successfully");

    // Create fullscreen quad vertex buffer
    // Format: pos(2) + uv(2) = 4 floats per vertex
    static const float quadVertices[] = {
        // pos.x, pos.y, uv.x, uv.y
        -1.0f, -1.0f,  0.0f, 1.0f,  // bottom-left
         1.0f, -1.0f,  1.0f, 1.0f,  // bottom-right
         1.0f,  1.0f,  1.0f, 0.0f,  // top-right
        -1.0f, -1.0f,  0.0f, 1.0f,  // bottom-left
         1.0f,  1.0f,  1.0f, 0.0f,  // top-right
        -1.0f,  1.0f,  0.0f, 0.0f,  // top-left
    };

    hina_buffer_desc quad_vb_desc = {};
    quad_vb_desc.size = sizeof(quadVertices);
    quad_vb_desc.flags = static_cast<hina_buffer_flags>(
        HINA_BUFFER_VERTEX_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
    m_fullscreenQuadVB = hina_make_buffer(&quad_vb_desc);
    if (!hina_buffer_is_valid(m_fullscreenQuadVB)) {
        LOG_ERROR("[GfxRenderer] Failed to create fullscreen quad vertex buffer");
        return false;
    }

    void* quadData = hina_map_buffer(m_fullscreenQuadVB);
    std::memcpy(quadData, quadVertices, sizeof(quadVertices));
    LOG_INFO("[GfxRenderer] Fullscreen quad created");

    // Create composite bind group with G-buffer textures (albedo, normal, materialData, depth)
    hina_bind_group_entry compositeEntries[4] = {};
    compositeEntries[0].binding = 0;  // albedo
    compositeEntries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    compositeEntries[0].combined.view = m_gbuffer.albedoView;
    compositeEntries[0].combined.sampler = m_defaultSampler;

    compositeEntries[1].binding = 1;  // normal
    compositeEntries[1].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    compositeEntries[1].combined.view = m_gbuffer.normalView;
    compositeEntries[1].combined.sampler = m_defaultSampler;

    compositeEntries[2].binding = 2;  // materialData
    compositeEntries[2].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    compositeEntries[2].combined.view = m_gbuffer.materialDataView;
    compositeEntries[2].combined.sampler = m_defaultSampler;

    compositeEntries[3].binding = 3;  // depth
    compositeEntries[3].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    compositeEntries[3].combined.view = m_gbuffer.depthView;
    compositeEntries[3].combined.sampler = m_defaultSampler;

    hina_bind_group_desc compositeBgDesc = {};
    compositeBgDesc.layout = m_compositeLayout;
    compositeBgDesc.entries = compositeEntries;
    compositeBgDesc.entry_count = 4;

    m_compositeBindGroup = hina_create_bind_group(&compositeBgDesc);
    if (!hina_bind_group_is_valid(m_compositeBindGroup)) {
        LOG_ERROR("[GfxRenderer] Failed to create composite bind group");
        return false;
    }
    LOG_INFO("[GfxRenderer] Composite bind group created");

    // ========================================================================
    // Grid Pipeline (infinite grid for editor)
    // ========================================================================
    const char* grid_shader = R"(
#hina
group Grid = 0;

bindings(Grid, start=0) {
  uniform(std140) GridUBO {
    mat4 view;
    mat4 proj;
    mat4 invViewProj;
    vec4 cameraPos;
  } grid;
}

struct VertexIn {
  vec2 a_position;
  vec2 a_uv;
};

struct Varyings {
  vec3 nearPoint;
  vec3 farPoint;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn vin) {
    Varyings out;
    vec4 clipNear = vec4(vin.a_position, 0.0, 1.0);
    vec4 clipFar = vec4(vin.a_position, 1.0, 1.0);
    vec4 nearWorld = grid.invViewProj * clipNear;
    vec4 farWorld = grid.invViewProj * clipFar;
    out.nearPoint = nearWorld.xyz / nearWorld.w;
    out.farPoint = farWorld.xyz / farWorld.w;
    gl_Position = vec4(vin.a_position, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
float getGrid(vec2 coord, float gridSize) {
    vec2 derivative = fwidth(coord);
    vec2 g = abs(fract(coord / gridSize - 0.5) - 0.5) / derivative * gridSize;
    float line = min(g.x, g.y);
    return 1.0 - min(line, 1.0);
}

FragOut FSMain(Varyings in) {
    FragOut out;
    // Calculate ray-plane intersection with y=0 (ground plane)
    float denom = in.farPoint.y - in.nearPoint.y;
    if (abs(denom) < 0.0001) { discard; }  // Ray parallel to ground
    float t = -in.nearPoint.y / denom;
    if (t < 0.0 || t > 1.0) { discard; }  // Behind camera or beyond far plane
    vec3 fragPos3D = in.nearPoint + t * (in.farPoint - in.nearPoint);

    // Compute depth and clamp to valid range
    vec4 clipPos = grid.proj * grid.view * vec4(fragPos3D, 1.0);
    float depth = clipPos.z / clipPos.w;
    gl_FragDepth = clamp(depth, 0.0, 1.0);

    float gridScale = grid.cameraPos.w;
    float minorGrid = getGrid(fragPos3D.xz, gridScale);
    float majorGrid = getGrid(fragPos3D.xz, gridScale * 10.0);
    float axisWidth = 0.02 * gridScale;
    float xAxis = 1.0 - smoothstep(0.0, axisWidth, abs(fragPos3D.z));
    float zAxis = 1.0 - smoothstep(0.0, axisWidth, abs(fragPos3D.x));
    float dist = length(fragPos3D.xz - grid.cameraPos.xz);
    float fade = 1.0 - smoothstep(gridScale * 20.0, gridScale * 50.0, dist);
    vec3 color = vec3(0.4);
    float alpha = max(minorGrid * 0.3, majorGrid * 0.5) * fade;
    if (xAxis > 0.0) { color = vec3(0.8, 0.2, 0.2); alpha = max(alpha, xAxis * fade); }
    if (zAxis > 0.0) { color = vec3(0.2, 0.2, 0.8); alpha = max(alpha, zAxis * fade); }
    out.color = vec4(color, alpha);
    return out;
}
#hina_end
)";

    // Compile grid shader
    char* grid_error = nullptr;
    hina_hsl_module* grid_module = hslc_compile_hsl_source(grid_shader, "grid_shader", &grid_error);
    if (!grid_module) {
        LOG_ERROR("[GfxRenderer] Grid shader compilation failed: {}", grid_error ? grid_error : "Unknown");
        if (grid_error) hslc_free_log(grid_error);
        // Grid is optional - continue without it
        LOG_WARNING("[GfxRenderer] Continuing without grid shader");
    } else {
        // Create explicit bind group layout for grid (set 0: UBO at binding 0)
        hina_bind_group_layout_entry grid_layout_entries[1] = {};
        grid_layout_entries[0].binding = 0;
        grid_layout_entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
        grid_layout_entries[0].count = 1;
        grid_layout_entries[0].stage_flags = HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT;
        grid_layout_entries[0].flags = HINA_BINDING_FLAG_NONE;

        hina_bind_group_layout_desc grid_layout_desc = {};
        grid_layout_desc.entries = grid_layout_entries;
        grid_layout_desc.entry_count = 1;
        grid_layout_desc.label = "grid_layout";

        m_gridLayout = hina_create_bind_group_layout(&grid_layout_desc);
        if (!hina_bind_group_layout_is_valid(m_gridLayout)) {
            LOG_ERROR("[GfxRenderer] Failed to create grid bind group layout");
            hslc_hsl_module_free(grid_module);
        } else {
            // Vertex layout for grid pass (fullscreen quad: pos(2) + uv(2) = 16 bytes)
            hina_vertex_layout grid_vertex_layout = {};
            grid_vertex_layout.buffer_count = 1;
            grid_vertex_layout.buffer_strides[0] = sizeof(float) * 4; // pos(2) + uv(2)
            grid_vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
            grid_vertex_layout.attr_count = 2;
            grid_vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, 0, 0, 0 };  // position (vec2)
            grid_vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, sizeof(float) * 2, 1, 0 };  // uv (vec2)

            hina_hsl_pipeline_desc grid_pip_desc = hina_hsl_pipeline_desc_default();
            grid_pip_desc.layout = grid_vertex_layout;
            grid_pip_desc.cull_mode = HINA_CULL_MODE_NONE;
            grid_pip_desc.depth.depth_test = true;
            grid_pip_desc.depth.depth_write = true;
            grid_pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS_OR_EQUAL;
            grid_pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
            grid_pip_desc.color_attachment_count = 1;
            grid_pip_desc.color_formats[0] = HINA_FORMAT_B8G8R8A8_UNORM;  // Match ViewOutput format (manual gamma)
            grid_pip_desc.blend.enable = true;
            grid_pip_desc.blend.src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
            grid_pip_desc.blend.dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            grid_pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
            grid_pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ONE;
            grid_pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ZERO;
            grid_pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;
            grid_pip_desc.bind_group_layouts[0] = m_gridLayout;
            grid_pip_desc.bind_group_layout_count = 1;

            m_gridPipeline = hina_make_pipeline_from_module(grid_module, &grid_pip_desc, nullptr);
            hslc_hsl_module_free(grid_module);

            if (!hina_pipeline_is_valid(m_gridPipeline)) {
                LOG_ERROR("[GfxRenderer] Grid pipeline creation failed");
            } else {
                LOG_INFO("[GfxRenderer] Grid pipeline created successfully");

                // Create grid UBO
                hina_buffer_desc grid_ubo_desc = {};
                grid_ubo_desc.size = sizeof(glm::mat4) * 3 + sizeof(glm::vec4);  // view, proj, invViewProj, cameraPos
                grid_ubo_desc.flags = static_cast<hina_buffer_flags>(
                    HINA_BUFFER_UNIFORM_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
                m_gridUBO = hina_make_buffer(&grid_ubo_desc);
                m_gridUBOMapped = hina_map_buffer(m_gridUBO);

                // Create grid bind group
                hina_bind_group_entry grid_entry = {};
                grid_entry.binding = 0;
                grid_entry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
                grid_entry.buffer.buffer = m_gridUBO;
                grid_entry.buffer.offset = 0;
                grid_entry.buffer.size = grid_ubo_desc.size;

                hina_bind_group_desc grid_bg_desc = {};
                grid_bg_desc.layout = m_gridLayout;
                grid_bg_desc.entries = &grid_entry;
                grid_bg_desc.entry_count = 1;

                m_gridBindGroup = hina_create_bind_group(&grid_bg_desc);
                if (!hina_bind_group_is_valid(m_gridBindGroup)) {
                    LOG_ERROR("[GfxRenderer] Failed to create grid bind group");
                } else {
                    LOG_INFO("[GfxRenderer] Grid bind group created");
                }
            }
        }
    }

    LOG_INFO("[GfxRenderer] All pipelines created successfully");
    return true;
}

void GfxRenderer::executeGBufferPass(gfx::Cmd* cmd) {
    if (!cmd || !hina_pipeline_is_valid(m_gbufferPipeline)) {
        return;
    }

    // G-buffer pass
    // Note: width/height = 0 lets hina-vk infer from attachment dimensions
    hina_pass_action pass = {};

    // Color attachments: albedo, normal, materialData
    pass.colors[0].image = m_gbuffer.albedoView;
    pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[0].store_op = HINA_STORE_OP_STORE;
    pass.colors[0].clear_color[0] = 0.0f;
    pass.colors[0].clear_color[1] = 0.0f;
    pass.colors[0].clear_color[2] = 0.0f;
    pass.colors[0].clear_color[3] = 0.0f;

    pass.colors[1].image = m_gbuffer.normalView;
    pass.colors[1].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[1].store_op = HINA_STORE_OP_STORE;
    pass.colors[1].clear_color[0] = 0.5f;  // neutral normal X
    pass.colors[1].clear_color[1] = 0.5f;  // neutral normal Y
    pass.colors[1].clear_color[2] = 0.0f;
    pass.colors[1].clear_color[3] = 0.0f;

    pass.colors[2].image = m_gbuffer.materialDataView;
    pass.colors[2].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[2].store_op = HINA_STORE_OP_STORE;

    // Depth attachment
    pass.depth.image = m_gbuffer.depthView;
    pass.depth.load_op = HINA_LOAD_OP_CLEAR;
    pass.depth.store_op = HINA_STORE_OP_STORE;
    pass.depth.depth_clear = 1.0f;

    hina_cmd_begin_pass(cmd, &pass);

    // Note: hina-vk sets default viewport/scissor based on pass dimensions
    // (hina-vk deferred example doesn't call set_viewport/set_scissor)

    // Bind pipeline
    hina_cmd_bind_pipeline(cmd, m_gbufferPipeline);

    // Bind frame UBO at set 0
    hina_cmd_bind_group(cmd, 0, m_gbufferSceneBindGroup);

    // Push constants: model + baseColor only (80 bytes, fits in 128 byte limit)
    struct {
        glm::mat4 model;
        glm::vec4 baseColor;
    } pushConstants;

    // Draw submitted meshes from the queue
    for (const auto& drawCall : m_drawQueue) {
        const gfx::GpuMesh* gpuMesh = m_meshStorage.get(drawCall.mesh);
        if (!gpuMesh || !gpuMesh->valid) {
            continue;
        }

        // Bind split vertex streams with offsets (chunked storage shares buffers between meshes)
        hina_vertex_input meshInput = {};
        // Buffer 0: Position stream
        meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
        meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
        // Buffer 1: Attribute stream
        meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
        meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
        // Index buffer
        meshInput.index_buffer = gpuMesh->indexBuffer;
        meshInput.index_type = HINA_INDEX_UINT32;

        hina_cmd_apply_vertex_input(cmd, &meshInput);

        // Bind material at Set 1
        gfx::BindGroup materialBG;
        if (drawCall.useMaterial && m_materialSystem.isMaterialValid(drawCall.material)) {
            materialBG = m_materialSystem.getMaterialBindGroup(drawCall.material);
        } else {
            // Use default material for objects without explicit materials
            materialBG = m_materialSystem.getMaterialBindGroup(m_materialSystem.getDefaultMaterial());
        }
        if (hina_bind_group_is_valid(materialBG)) {
            hina_cmd_bind_group(cmd, 1, materialBG);
        }

        // Update push constants for this draw
        pushConstants.model = drawCall.transform;
        if (drawCall.useMaterial && m_materialSystem.isMaterialValid(drawCall.material)) {
            // Get base color from material
            const gfx::MaterialEntry* matEntry = m_materialSystem.getMaterial(drawCall.material);
            if (matEntry) {
                pushConstants.baseColor = matEntry->material.baseColor;
            } else {
                pushConstants.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
            }
        } else {
            pushConstants.baseColor = drawCall.baseColor;
        }
        hina_cmd_push_constants(cmd, 0, sizeof(pushConstants), &pushConstants);

        // Draw with index offset (convert byte offset to index count)
        uint32_t firstIndex = gpuMesh->indexOffset / sizeof(uint32_t);
        hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, firstIndex, 0, 0);
    }

    hina_cmd_end_pass(cmd);
}

void GfxRenderer::renderGBuffer() {
    if (!hina_pipeline_is_valid(m_gbufferPipeline)) {
        return;
    }

    auto& frame = m_frames[m_currentFrame];

    // Calculate viewProj matrix for this frame
    glm::mat4 view, proj;

    if (m_cameraMatricesSet) {
        // Use externally-set camera matrices (from editor/game camera)
        view = m_viewMatrix;
        proj = m_projMatrix;
    } else {
        // Fallback test camera if no camera has been set
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        view = glm::lookAt(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        proj[1][1] *= -1.0f;  // Vulkan clip space correction
    }

    glm::mat4 viewProj = proj * view;

    // Write viewProj to the per-frame UBO (persistently mapped, HOST_COHERENT)
    std::memcpy(m_gbufferFrameUBOMapped, &viewProj, sizeof(glm::mat4));

    // Execute the actual G-buffer rendering
    executeGBufferPass(frame.cmd);
}

void GfxRenderer::executeCompositePass(gfx::Cmd* cmd) {
    if (!cmd || !hina_pipeline_is_valid(m_compositePassPipeline)) {
        return;
    }
    if (!hina_bind_group_is_valid(m_compositeBindGroup)) {
        return;
    }

    // Get the active view output
    auto& viewOutput = m_viewOutputs[static_cast<size_t>(m_activeViewId)];
    if (!viewOutput.valid) {
        return;
    }

    // Composite pass - render G-buffer to view output texture with deferred lighting
    // Note: width/height = 0 lets hina-vk infer from attachment dimensions
    hina_pass_action pass = {};
    pass.colors[0].image = viewOutput.view;
    pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[0].store_op = HINA_STORE_OP_STORE;
    pass.colors[0].clear_color[0] = 0.1f;  // Dark gray background for visibility
    pass.colors[0].clear_color[1] = 0.1f;
    pass.colors[0].clear_color[2] = 0.1f;
    pass.colors[0].clear_color[3] = 1.0f;

    hina_cmd_begin_pass(cmd, &pass);

    // Set viewport and scissor to match view output dimensions
    hina_viewport viewport = {};
    viewport.width = static_cast<float>(viewOutput.width);
    viewport.height = static_cast<float>(viewOutput.height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    hina_cmd_set_viewport(cmd, &viewport);

    hina_scissor scissor = {};
    scissor.width = viewOutput.width;
    scissor.height = viewOutput.height;
    hina_cmd_set_scissor(cmd, &scissor);

    // Bind pipeline
    hina_cmd_bind_pipeline(cmd, m_compositePassPipeline);

    // Bind G-buffer textures
    hina_cmd_bind_group(cmd, 0, m_compositeBindGroup);

    // Prepare lighting push constants
    struct {
        glm::vec4 lightDir;      // xyz = direction, w = intensity
        glm::vec4 lightColor;    // rgb = color, a = unused
        glm::vec4 ambientColor;  // rgb = ambient, a = unused
        glm::vec4 cameraPos;     // xyz = camera position, w = unused
        glm::mat4 invViewProj;   // For world position reconstruction
    } lightingParams;

    // Set up directional light (sun-like, coming from upper-right-front)
    glm::vec3 lightDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f));
    lightingParams.lightDir = glm::vec4(lightDirection, 1.5f);  // intensity = 1.5
    lightingParams.lightColor = glm::vec4(1.0f, 0.98f, 0.95f, 1.0f);  // warm white
    lightingParams.ambientColor = glm::vec4(0.15f, 0.17f, 0.2f, 1.0f);  // cool ambient

    // Camera position and inverse view-projection
    if (m_currentFrameData) {
        lightingParams.cameraPos = glm::vec4(m_currentFrameData->cameraPos, 1.0f);
        glm::mat4 viewProj = m_currentFrameData->projMatrix * m_currentFrameData->viewMatrix;
        lightingParams.invViewProj = glm::inverse(viewProj);
    } else {
        // Fallback for test mode
        lightingParams.cameraPos = glm::vec4(0.0f, 1.0f, 3.0f, 1.0f);
        float aspect = static_cast<float>(viewOutput.width) / static_cast<float>(viewOutput.height);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        proj[1][1] *= -1.0f;  // Vulkan clip space correction
        lightingParams.invViewProj = glm::inverse(proj * view);
    }

    hina_cmd_push_constants(cmd, 0, sizeof(lightingParams), &lightingParams);

    // Bind fullscreen quad vertex buffer
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = m_fullscreenQuadVB;
    vertInput.vertex_offsets[0] = 0;
    hina_cmd_apply_vertex_input(cmd, &vertInput);

    // Draw fullscreen quad (6 vertices)
    hina_cmd_draw(cmd, 6, 1, 0, 0);

    hina_cmd_end_pass(cmd);
}

void GfxRenderer::renderComposite() {
    executeCompositePass(m_frames[m_currentFrame].cmd);
}

void GfxRenderer::renderShadows() {
    // TODO: Implement shadow rendering
}

void GfxRenderer::renderLighting() {
    // TODO: Implement deferred lighting
}

void GfxRenderer::renderTransparency() {
    auto& frame = m_frames[m_currentFrame];
    if (!frame.cmd) return;

    // Execute WBOIT if there are transparent objects in the queue
    bool hasTransparent = false;
    for (const auto& draw : m_drawQueue) {
        if (draw.useMaterial && m_materialSystem.isMaterialValid(draw.material)) {
            const auto* matEntry = m_materialSystem.getMaterial(draw.material);
            if (matEntry && matEntry->material.alphaMode == gfx::GfxMaterial::AlphaMode::Blend) {
                hasTransparent = true;
                break;
            }
        }
    }

    if (hasTransparent) {
        executeWBOITAccumulationPass(frame.cmd);
    }
}

void GfxRenderer::executeWBOITAccumulationPass(gfx::Cmd* cmd) {
    if (!cmd) return;

    // WBOIT uses additive blending for accumulation, multiplicative for reveal
    // Pipeline should be created lazily - for now just clear the targets

    hina_pass_action pass = {};

    // Accumulation target: RGBA16F, additive blend
    pass.colors[0].image = m_wboit.accumulationView;
    pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[0].store_op = HINA_STORE_OP_STORE;
    pass.colors[0].clear_color[0] = 0.0f;
    pass.colors[0].clear_color[1] = 0.0f;
    pass.colors[0].clear_color[2] = 0.0f;
    pass.colors[0].clear_color[3] = 0.0f;

    // Reveal target: R8, initial = 1.0 (no objects)
    pass.colors[1].image = m_wboit.revealView;
    pass.colors[1].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[1].store_op = HINA_STORE_OP_STORE;
    pass.colors[1].clear_color[0] = 1.0f;

    // Use G-buffer depth for depth testing (no write)
    pass.depth.image = m_gbuffer.depthView;
    pass.depth.load_op = HINA_LOAD_OP_LOAD;  // Keep G-buffer depth
    pass.depth.store_op = HINA_STORE_OP_DONT_CARE;

    hina_cmd_begin_pass(cmd, &pass);

    // TODO: Bind WBOIT pipeline and render transparent objects
    // For now, this is a placeholder that just clears the WBOIT targets
    // The actual transparent object rendering will use a special shader that outputs:
    //   accum.rgb = premultiplied_color * weight
    //   accum.a = alpha * weight
    //   reveal = 1.0 - alpha (multiplicative)

    hina_cmd_end_pass(cmd);

    LOG_DEBUG("[GfxRenderer] WBOIT accumulation pass executed (placeholder)");
}

void GfxRenderer::executeWBOITResolvePass(gfx::Cmd* cmd) {
    if (!cmd) return;

    auto& viewOutput = m_viewOutputs[static_cast<size_t>(m_activeViewId)];
    if (!viewOutput.valid) return;

    // TODO: Create WBOIT resolve pipeline that:
    // 1. Samples accumulation and reveal buffers
    // 2. Computes: color = accum.rgb / max(accum.a, epsilon)
    // 3. Alpha = 1.0 - reveal
    // 4. Blends over the view output with alpha blending

    // For now, this is a placeholder
    LOG_DEBUG("[GfxRenderer] WBOIT resolve pass executed (placeholder)");
}

void GfxRenderer::renderPostProcess() {
    // TODO: Implement post-processing
}

void GfxRenderer::executeGridPass(gfx::Cmd* cmd) {
    auto& viewOutput = m_viewOutputs[static_cast<size_t>(m_activeViewId)];

    if (!cmd || !hina_pipeline_is_valid(m_gridPipeline) || !viewOutput.valid) {
        return;
    }
    if (!hina_bind_group_is_valid(m_gridBindGroup)) {
        return;
    }

    // Update grid UBO with camera data
    struct GridUBO {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 invViewProj;
        glm::vec4 cameraPos;  // xyz = position, w = grid scale
    } gridData;

    gridData.view = m_viewMatrix;
    gridData.proj = m_projMatrix;
    gridData.invViewProj = glm::inverse(m_projMatrix * m_viewMatrix);

    // Extract camera position from view matrix inverse
    glm::mat4 invView = glm::inverse(m_viewMatrix);
    gridData.cameraPos = glm::vec4(invView[3].x, invView[3].y, invView[3].z, 1.0f);  // Grid scale = 1.0

    if (m_gridUBOMapped) {
        std::memcpy(m_gridUBOMapped, &gridData, sizeof(gridData));
    }

    // Transition view output to color attachment state
    hina_cmd_transition_texture(cmd, viewOutput.texture, HINA_TEXSTATE_COLOR_ATTACHMENT);

    // Begin render pass - render to view output with depth from G-buffer
    hina_pass_action pass = {};
    pass.colors[0].image = viewOutput.view;
    pass.colors[0].load_op = HINA_LOAD_OP_LOAD;  // Keep existing scene content
    pass.colors[0].store_op = HINA_STORE_OP_STORE;
    pass.depth.image = m_gbuffer.depthView;
    pass.depth.load_op = HINA_LOAD_OP_LOAD;  // Use existing depth
    pass.depth.store_op = HINA_STORE_OP_STORE;

    hina_cmd_begin_pass(cmd, &pass);

    // Set viewport and scissor to match view output dimensions
    hina_viewport viewport = {};
    viewport.width = static_cast<float>(viewOutput.width);
    viewport.height = static_cast<float>(viewOutput.height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    hina_cmd_set_viewport(cmd, &viewport);

    hina_scissor scissor = {};
    scissor.width = viewOutput.width;
    scissor.height = viewOutput.height;
    hina_cmd_set_scissor(cmd, &scissor);

    // Bind pipeline and resources
    hina_cmd_bind_pipeline(cmd, m_gridPipeline);
    hina_cmd_bind_group(cmd, 0, m_gridBindGroup);

    // Use the same fullscreen quad as composite pass
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = m_fullscreenQuadVB;
    vertInput.vertex_offsets[0] = 0;
    hina_cmd_apply_vertex_input(cmd, &vertInput);
    hina_cmd_draw(cmd, 6, 1, 0, 0);

    hina_cmd_end_pass(cmd);
}

void GfxRenderer::renderGrid() {
    executeGridPass(m_frames[m_currentFrame].cmd);
}

void GfxRenderer::blitToSwapchain() {
    auto& frame = m_frames[m_currentFrame];
    auto& viewOutput = m_viewOutputs[static_cast<size_t>(m_activeViewId)];

    if (!viewOutput.valid) {
        return;
    }

    // Transition textures for blit
    hina_cmd_transition_texture(frame.cmd, viewOutput.texture, HINA_TEXSTATE_SHADER_READ);
    hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_COLOR_ATTACHMENT);

    // Blit viewOutput to swapchain
    hina_cmd_blit_texture(frame.cmd, viewOutput.texture, frame.swapchainImage.texture, 0, 0, HINA_FILTER_LINEAR);

    // Transition swapchain for presentation
    hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_PRESENT);
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

void GfxRenderer::renderClear() {
    auto& frame = m_frames[m_currentFrame];

    // Use swapchain view acquired in beginFrame
    if (!hina_texture_view_is_valid(frame.swapchainView)) {
        LOG_WARNING("[GfxRenderer] No swapchain view available");
        return;
    }

    // Clear pass - just clear the swapchain to a solid color for testing
    hina_pass_action pass = {};
    pass.width = m_width;
    pass.height = m_height;

    // Color attachment: swapchain image, clear to cornflower blue
    pass.colors[0].image = frame.swapchainView;
    pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;
    pass.colors[0].store_op = HINA_STORE_OP_STORE;
    pass.colors[0].clear_color[0] = 0.392f;  // R
    pass.colors[0].clear_color[1] = 0.584f;  // G
    pass.colors[0].clear_color[2] = 0.929f;  // B
    pass.colors[0].clear_color[3] = 1.0f;    // A

    // No depth for simple clear test
    pass.depth = {};

    hina_cmd_begin_pass(frame.cmd, &pass);
    // No draw calls - just clearing
    hina_cmd_end_pass(frame.cmd);
}

// ============================================================================
// ImGui Integration
// ============================================================================

bool GfxRenderer::initImGui() {
    if (m_imguiInitialized) return true;

    LOG_INFO("[GfxRenderer] Initializing ImGui...");

    // Build font atlas (ImGui context should already be created by ImGuiContext)
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create font texture
    hina_texture_desc tex_desc = hina_texture_desc_default();
    tex_desc.format = HINA_FORMAT_R8G8B8A8_UNORM;
    tex_desc.width = static_cast<uint32_t>(width);
    tex_desc.height = static_cast<uint32_t>(height);
    tex_desc.initial_data = pixels;
    tex_desc.usage = HINA_TEXTURE_SAMPLED_BIT;

    m_imguiFontTexture = hina_make_texture(&tex_desc);
    if (!hina_texture_is_valid(m_imguiFontTexture)) {
        LOG_ERROR("[GfxRenderer] Failed to create ImGui font texture");
        return false;
    }

    // Create sampler
    hina_sampler_desc sampler_desc = hina_sampler_desc_default();
    sampler_desc.min_filter = HINA_FILTER_LINEAR;
    sampler_desc.mag_filter = HINA_FILTER_LINEAR;
    sampler_desc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    sampler_desc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_imguiFontSampler = hina_make_sampler(&sampler_desc);
    if (!hina_sampler_is_valid(m_imguiFontSampler)) {
        LOG_ERROR("[GfxRenderer] Failed to create ImGui sampler");
        return false;
    }

    // Store font texture view for use with transient bind groups
    m_imguiFontView = hina_texture_get_default_view(m_imguiFontTexture);

    // Set ImGui font texture ID to 0 (special marker for font texture)
    io.Fonts->SetTexID(static_cast<ImTextureID>(IMGUI_FONT_TEXTURE_ID));

    // Create pipeline with embedded HSL shader
    const char* shader_source = R"(
#hina
group ImGui = 0;

bindings(ImGui, start=0) {
  texture sampler2D u_texture;
}

push_constant PushConstants {
  vec2 scale;
  vec2 translate;
} pc;

struct VertexIn {
  vec2 a_position;
  vec2 a_uv;
  vec4 a_color;
};

struct Varyings {
  vec2 uv;
  vec4 color;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;
    out.uv = in.a_uv;
    out.color = in.a_color;
    gl_Position = vec4(in.a_position * pc.scale + pc.translate, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;
    out.color = in.color * texture(u_texture, in.uv);
    return out;
}
#hina_end
)";

    char* error = nullptr;
    hina_hsl_module* module = hslc_compile_hsl_source(shader_source, "imgui_shader", &error);
    if (!module) {
        LOG_ERROR("[GfxRenderer] ImGui shader compilation failed: {}", error ? error : "Unknown");
        if (error) hslc_free_log(error);
        return false;
    }

    hina_vertex_layout vertex_layout = {};
    vertex_layout.buffer_count = 1;
    vertex_layout.buffer_strides[0] = sizeof(ImDrawVert);
    vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
    vertex_layout.attr_count = 3;
    vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos), 0, 0 };
    vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv), 1, 0 };
    vertex_layout.attrs[2] = { HINA_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col), 2, 0 };

    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.layout = vertex_layout;
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;
    pip_desc.depth.depth_test = false;
    pip_desc.depth.depth_write = false;
    pip_desc.blend.enable = true;
    pip_desc.blend.src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
    pip_desc.blend.dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
    pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ONE;
    pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;
    pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

    // Use the shared UI bind group layout - same layout for pipeline AND transient bind groups
    if (hina_bind_group_layout_is_valid(m_uiBindGroupLayout)) {
        pip_desc.bind_group_layouts[0] = m_uiBindGroupLayout;
        pip_desc.bind_group_layout_count = 1;
    }

    m_imguiPipeline = hina_make_pipeline_from_module(module, &pip_desc, nullptr);
    hslc_hsl_module_free(module);

    if (!hina_pipeline_is_valid(m_imguiPipeline)) {
        LOG_ERROR("[GfxRenderer] ImGui pipeline creation failed");
        return false;
    }

    // Create per-frame buffers
    for (uint32_t i = 0; i < GFX_MAX_FRAMES; ++i) {
        hina_buffer_desc vb_desc = {};
        vb_desc.size = IMGUI_MAX_VERTEX_COUNT * sizeof(ImDrawVert);
        vb_desc.flags = static_cast<hina_buffer_flags>(
            HINA_BUFFER_VERTEX_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
        m_imguiFrames[i].vertexBuffer = hina_make_buffer(&vb_desc);
        m_imguiFrames[i].vertexMapped = hina_map_buffer(m_imguiFrames[i].vertexBuffer);

        hina_buffer_desc ib_desc = {};
        ib_desc.size = IMGUI_MAX_INDEX_COUNT * sizeof(ImDrawIdx);
        ib_desc.flags = static_cast<hina_buffer_flags>(
            HINA_BUFFER_INDEX_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
        m_imguiFrames[i].indexBuffer = hina_make_buffer(&ib_desc);
        m_imguiFrames[i].indexMapped = hina_map_buffer(m_imguiFrames[i].indexBuffer);

        if (!m_imguiFrames[i].vertexMapped || !m_imguiFrames[i].indexMapped) {
            LOG_ERROR("[GfxRenderer] Failed to create/map ImGui frame buffers for frame {}", i);
            return false;
        }
    }

    m_imguiInitialized = true;

    LOG_INFO("[GfxRenderer] ImGui initialized successfully");
    return true;
}

void GfxRenderer::shutdownImGui() {
    if (!m_imguiInitialized) return;

    LOG_INFO("[GfxRenderer] Shutting down ImGui...");

    // Destroy per-frame buffers
    for (uint32_t i = 0; i < GFX_MAX_FRAMES; ++i) {
        if (hina_buffer_is_valid(m_imguiFrames[i].vertexBuffer))
            hina_destroy_buffer(m_imguiFrames[i].vertexBuffer);
        if (hina_buffer_is_valid(m_imguiFrames[i].indexBuffer))
            hina_destroy_buffer(m_imguiFrames[i].indexBuffer);
    }

    if (hina_pipeline_is_valid(m_imguiPipeline))
        hina_destroy_pipeline(m_imguiPipeline);
    if (hina_sampler_is_valid(m_imguiFontSampler))
        hina_destroy_sampler(m_imguiFontSampler);
    if (hina_texture_is_valid(m_imguiFontTexture))
        hina_destroy_texture(m_imguiFontTexture);

    m_imguiFontView = {};

    m_imguiInitialized = false;
    LOG_INFO("[GfxRenderer] ImGui shutdown complete");
}

void GfxRenderer::renderImGui(ImDrawData* drawData) {
    if (!m_imguiInitialized || !drawData) return;
    if (drawData->TotalVtxCount == 0 || drawData->TotalIdxCount == 0) return;

    auto& frame = m_frames[m_currentFrame];
    auto& imguiFrame = m_imguiFrames[m_currentFrame];

    // Check buffer sizes
    if (drawData->TotalVtxCount > static_cast<int>(IMGUI_MAX_VERTEX_COUNT) ||
        drawData->TotalIdxCount > static_cast<int>(IMGUI_MAX_INDEX_COUNT)) {
        LOG_WARNING("[GfxRenderer] ImGui draw data exceeds buffer size");
        return;
    }

    // Upload vertex/index data
    ImDrawVert* vtx = static_cast<ImDrawVert*>(imguiFrame.vertexMapped);
    ImDrawIdx* idx = static_cast<ImDrawIdx*>(imguiFrame.indexMapped);
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        std::memcpy(vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idx, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx += cmdList->VtxBuffer.Size;
        idx += cmdList->IdxBuffer.Size;
    }

    // Transition swapchain to color attachment before rendering
    hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_COLOR_ATTACHMENT);

    // Begin ImGui pass - clear swapchain to dark gray (editor background)
    hina_pass_action pass = {};
    pass.width = m_width;
    pass.height = m_height;
    pass.colors[0].image = frame.swapchainView;
    pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;  // Clear before drawing UI
    pass.colors[0].store_op = HINA_STORE_OP_STORE;
    pass.colors[0].clear_color[0] = 0.118f;  // Dark gray (30/255)
    pass.colors[0].clear_color[1] = 0.118f;
    pass.colors[0].clear_color[2] = 0.118f;
    pass.colors[0].clear_color[3] = 1.0f;

    hina_cmd_begin_pass(frame.cmd, &pass);

    // Set viewport
    hina_viewport viewport = {};
    viewport.width = static_cast<float>(m_width);
    viewport.height = static_cast<float>(m_height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    hina_cmd_set_viewport(frame.cmd, &viewport);

    // Bind pipeline
    hina_cmd_bind_pipeline(frame.cmd, m_imguiPipeline);

    // Bind vertex and index buffers
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = imguiFrame.vertexBuffer;
    vertInput.vertex_offsets[0] = 0;
    vertInput.index_buffer = imguiFrame.indexBuffer;
    vertInput.index_type = HINA_INDEX_UINT16;
    hina_cmd_apply_vertex_input(frame.cmd, &vertInput);

    // Push constants for projection
    struct {
        float scale[2];
        float translate[2];
    } pushConstants;
    pushConstants.scale[0] = 2.0f / drawData->DisplaySize.x;
    pushConstants.scale[1] = 2.0f / drawData->DisplaySize.y;
    pushConstants.translate[0] = -1.0f - drawData->DisplayPos.x * pushConstants.scale[0];
    pushConstants.translate[1] = -1.0f - drawData->DisplayPos.y * pushConstants.scale[1];
    hina_cmd_push_constants(frame.cmd, 0, sizeof(pushConstants), &pushConstants);

    // Draw commands
    uint32_t idxOffset = 0;
    uint32_t vtxOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;
    ImVec2 clipScale = drawData->FramebufferScale;

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
            const ImDrawCmd& drawCmd = cmdList->CmdBuffer[cmdIdx];
            if (drawCmd.UserCallback) continue;

            // Clip rect
            float clipMinX = (drawCmd.ClipRect.x - clipOff.x) * clipScale.x;
            float clipMinY = (drawCmd.ClipRect.y - clipOff.y) * clipScale.y;
            float clipMaxX = (drawCmd.ClipRect.z - clipOff.x) * clipScale.x;
            float clipMaxY = (drawCmd.ClipRect.w - clipOff.y) * clipScale.y;
            clipMinX = std::max(clipMinX, 0.0f);
            clipMinY = std::max(clipMinY, 0.0f);
            clipMaxX = std::min(clipMaxX, static_cast<float>(m_width));
            clipMaxY = std::min(clipMaxY, static_cast<float>(m_height));
            if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) continue;

            hina_scissor scissor = {};
            scissor.x = static_cast<int32_t>(clipMinX);
            scissor.y = static_cast<int32_t>(clipMinY);
            scissor.width = static_cast<uint32_t>(clipMaxX - clipMinX);
            scissor.height = static_cast<uint32_t>(clipMaxY - clipMinY);
            hina_cmd_set_scissor(frame.cmd, &scissor);

            // Determine texture view and sampler based on texture ID encoding
            uint64_t texId = static_cast<uint64_t>(drawCmd.TextureId);
            gfx::TextureView texView = {};
            gfx::Sampler sampler = m_imguiFontSampler;

            if (texId == IMGUI_FONT_TEXTURE_ID) {
                // Font texture (ImGui default ID 0)
                texView = m_imguiFontView;
            } else if (texId & IMGUI_VIEWOUTPUT_BIT) {
                // ViewOutput pointer (high bit set)
                const ViewOutput* vo = reinterpret_cast<const ViewOutput*>(texId & ~IMGUI_VIEWOUTPUT_BIT);
                if (vo && vo->valid) {
                    texView = vo->view;
                    sampler = vo->sampler;
                }
            } else {
                // Encoded TextureHandle from material system
                gfx::TextureHandle h;
                h.index = static_cast<uint16_t>(texId & 0xFFFF);
                h.generation = static_cast<uint16_t>((texId >> 16) & 0xFFFF);
                texView = m_materialSystem.getTextureView(h);
            }

            // Fallback to default white texture if invalid
            if (!hina_texture_view_is_valid(texView)) {
                texView = m_defaultWhiteTextureView;
            }

            // Create transient bind group using the shared UI layout
            hina_transient_bind_group tbg = hina_alloc_transient_bind_group(m_uiBindGroupLayout);
            hina_transient_write_combined_image(&tbg, 0, texView, sampler);
            hina_cmd_bind_transient_group(frame.cmd, 0, tbg);

            // Draw
            hina_cmd_draw_indexed(frame.cmd, drawCmd.ElemCount, 1,
                                  idxOffset + drawCmd.IdxOffset,
                                  static_cast<int32_t>(vtxOffset + drawCmd.VtxOffset), 0);
        }
        idxOffset += cmdList->IdxBuffer.Size;
        vtxOffset += cmdList->VtxBuffer.Size;
    }

    hina_cmd_end_pass(frame.cmd);

    // Transition swapchain to present state
    hina_cmd_transition_texture(frame.cmd, frame.swapchainImage.texture, HINA_TEXSTATE_PRESENT);
}

uint32_t GfxRenderer::registerUITexture(gfx::TextureHandle texHandle) {
    // Simply encode the TextureHandle as a uint32_t
    // The texture view will be retrieved at render time using transient bind groups
    if (!m_materialSystem.isTextureValid(texHandle)) return 0;

    // Encode: index in low 16 bits, generation in high 16 bits
    return static_cast<uint32_t>(texHandle.index) | (static_cast<uint32_t>(texHandle.generation) << 16);
}

// ============================================================================
// 2D UI Rendering (Game Overlay)
// ============================================================================

bool GfxRenderer::initUI2D() {
    if (m_ui2dInitialized) return true;

    LOG_INFO("[GfxRenderer] Initializing 2D UI...");

    // Create samplers
    hina_sampler_desc sampler_desc = hina_sampler_desc_default();
    sampler_desc.min_filter = HINA_FILTER_LINEAR;
    sampler_desc.mag_filter = HINA_FILTER_LINEAR;
    sampler_desc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    sampler_desc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_ui2dLinearSampler = hina_make_sampler(&sampler_desc);

    sampler_desc.min_filter = HINA_FILTER_NEAREST;
    sampler_desc.mag_filter = HINA_FILTER_NEAREST;
    m_ui2dNearestSampler = hina_make_sampler(&sampler_desc);

    sampler_desc.min_filter = HINA_FILTER_LINEAR;
    sampler_desc.mag_filter = HINA_FILTER_LINEAR;
    m_ui2dFontSampler = hina_make_sampler(&sampler_desc);

    // Create per-frame vertex/index buffers
    for (uint32_t i = 0; i < GFX_MAX_FRAMES; ++i) {
        auto& frame = m_ui2dFrames[i];

        hina_buffer_desc vb_desc = {};
        vb_desc.size = UI2D_MAX_VERTEX_COUNT * sizeof(ui::PrimitiveVertex);
        vb_desc.flags = static_cast<hina_buffer_flags>(HINA_BUFFER_VERTEX_BIT | HINA_BUFFER_HOST_VISIBLE_BIT);

        frame.vertexBuffer = hina_make_buffer(&vb_desc);
        if (!hina_buffer_is_valid(frame.vertexBuffer)) {
            LOG_ERROR("[GfxRenderer] Failed to create UI2D vertex buffer");
            return false;
        }
        frame.vertexMapped = hina_map_buffer(frame.vertexBuffer);

        hina_buffer_desc ib_desc = {};
        ib_desc.size = UI2D_MAX_INDEX_COUNT * sizeof(uint32_t);
        ib_desc.flags = static_cast<hina_buffer_flags>(HINA_BUFFER_INDEX_BIT | HINA_BUFFER_HOST_VISIBLE_BIT);

        frame.indexBuffer = hina_make_buffer(&ib_desc);
        if (!hina_buffer_is_valid(frame.indexBuffer)) {
            LOG_ERROR("[GfxRenderer] Failed to create UI2D index buffer");
            return false;
        }
        frame.indexMapped = hina_map_buffer(frame.indexBuffer);
    }

    // Create pipeline with embedded HSL shader
    const char* shader_source = R"(
#hina
group UI2D = 0;

bindings(UI2D, start=0) {
  texture sampler2D u_texture;
}

push_constant PushConstants {
  vec4 LRTB;  // left, right, top, bottom
  uint textureId;
  uint samplerId;
} pc;

struct VertexIn {
  vec2 a_pos;
  vec2 a_uv;
  uint a_color;
};

struct Varyings {
  vec2 uv;
  vec4 color;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;

    float L = pc.LRTB.x;
    float R = pc.LRTB.y;
    float T = pc.LRTB.z;
    float B = pc.LRTB.w;

    mat4 proj = mat4(
        2.0 / (R - L), 0.0, 0.0, 0.0,
        0.0, 2.0 / (B - T), 0.0, 0.0,
        0.0, 0.0, -1.0, 0.0,
        (R + L) / (L - R), (T + B) / (T - B), 0.0, 1.0
    );

    out.color = unpackUnorm4x8(in.a_color);
    out.uv = in.a_uv;
    gl_Position = proj * vec4(in.a_pos, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;
    out.color = in.color * texture(u_texture, in.uv);
    return out;
}
#hina_end
)";

    char* error = nullptr;
    hina_hsl_module* module = hslc_compile_hsl_source(shader_source, "ui2d_shader", &error);
    if (!module) {
        LOG_ERROR("[GfxRenderer] UI2D shader compilation failed: {}", error ? error : "Unknown");
        if (error) hslc_free_log(error);
        return false;
    }

    // Vertex layout for PrimitiveVertex
    hina_vertex_layout vertex_layout = {};
    vertex_layout.buffer_count = 1;
    vertex_layout.buffer_strides[0] = sizeof(ui::PrimitiveVertex);
    vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
    vertex_layout.attr_count = 3;
    vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ui::PrimitiveVertex, x), 0, 0 };
    vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ui::PrimitiveVertex, u), 1, 0 };
    vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, offsetof(ui::PrimitiveVertex, color), 2, 0 };

    // Create pipeline
    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.layout = vertex_layout;
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;
    pip_desc.depth.depth_test = false;
    pip_desc.depth.depth_write = false;
    pip_desc.blend.enable = true;
    pip_desc.blend.src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
    pip_desc.blend.dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
    pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ONE;
    pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;
    pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

    // Use the shared UI bind group layout - same layout for pipeline AND transient bind groups
    if (hina_bind_group_layout_is_valid(m_uiBindGroupLayout)) {
        pip_desc.bind_group_layouts[0] = m_uiBindGroupLayout;
        pip_desc.bind_group_layout_count = 1;
    }

    m_ui2dPipeline = hina_make_pipeline_from_module(module, &pip_desc, nullptr);
    hslc_hsl_module_free(module);

    if (!hina_pipeline_is_valid(m_ui2dPipeline)) {
        LOG_ERROR("[GfxRenderer] Failed to create UI2D pipeline");
        return false;
    }

    m_ui2dInitialized = true;
    LOG_INFO("[GfxRenderer] 2D UI initialized successfully");
    return true;
}

void GfxRenderer::shutdownUI2D() {
    if (!m_ui2dInitialized) return;

    for (auto& frame : m_ui2dFrames) {
        if (hina_buffer_is_valid(frame.vertexBuffer)) {
            hina_destroy_buffer(frame.vertexBuffer);
        }
        if (hina_buffer_is_valid(frame.indexBuffer)) {
            hina_destroy_buffer(frame.indexBuffer);
        }
        frame = {};
    }

    if (hina_pipeline_is_valid(m_ui2dPipeline)) {
        hina_destroy_pipeline(m_ui2dPipeline);
    }
    if (hina_sampler_is_valid(m_ui2dLinearSampler)) {
        hina_destroy_sampler(m_ui2dLinearSampler);
    }
    if (hina_sampler_is_valid(m_ui2dNearestSampler)) {
        hina_destroy_sampler(m_ui2dNearestSampler);
    }
    if (hina_sampler_is_valid(m_ui2dFontSampler)) {
        hina_destroy_sampler(m_ui2dFontSampler);
    }

    m_ui2dInitialized = false;
    LOG_INFO("[GfxRenderer] 2D UI shutdown complete");
}

void GfxRenderer::queueUI2D(const ui::PrimitiveDrawList& drawList, float viewportWidth, float viewportHeight) {
    // Copy the draw list to be rendered later when command buffer is recording
    m_queuedUI2DDrawList = drawList;
    m_queuedUI2DViewportWidth = viewportWidth;
    m_queuedUI2DViewportHeight = viewportHeight;
    m_hasQueuedUI2D = !drawList.commands.empty();
}

void GfxRenderer::renderUI2D() {
    if (!m_ui2dInitialized || !m_hasQueuedUI2D) return;

    const ui::PrimitiveDrawList& drawList = m_queuedUI2DDrawList;
    float viewportWidth = m_queuedUI2DViewportWidth;
    float viewportHeight = m_queuedUI2DViewportHeight;

    // Clear queued flag
    m_hasQueuedUI2D = false;

    if (drawList.commands.empty()) return;

    auto& frame = m_frames[m_currentFrame];
    auto& uiFrame = m_ui2dFrames[m_currentFrame];

    if (!frame.cmd) return;

    const size_t vertexCount = drawList.vertices.size();
    const size_t indexCount = drawList.indices.size();
    if (vertexCount == 0 || indexCount == 0) return;

    // Check capacity
    if (vertexCount > UI2D_MAX_VERTEX_COUNT || indexCount > UI2D_MAX_INDEX_COUNT) {
        LOG_WARNING("[GfxRenderer] UI2D draw list too large: {} vertices, {} indices",
                    vertexCount, indexCount);
        return;
    }

    // Upload vertex/index data
    std::memcpy(uiFrame.vertexMapped, drawList.vertices.data(), vertexCount * sizeof(ui::PrimitiveVertex));
    std::memcpy(uiFrame.indexMapped, drawList.indices.data(), indexCount * sizeof(uint32_t));

    // Get view output for rendering
    auto& viewOutput = m_viewOutputs[static_cast<size_t>(m_activeViewId)];
    if (!viewOutput.valid) return;

    // Transition view output to color attachment state before rendering to it
    hina_cmd_transition_texture(frame.cmd, viewOutput.texture, HINA_TEXSTATE_COLOR_ATTACHMENT);

    // Begin render pass - render to view output
    hina_pass_action pass = {};
    pass.colors[0].image = viewOutput.view;
    pass.colors[0].load_op = HINA_LOAD_OP_LOAD;  // Load existing content
    pass.colors[0].store_op = HINA_STORE_OP_STORE;

    hina_cmd_begin_pass(frame.cmd, &pass);

    // Set viewport and scissor
    hina_viewport viewport = {};
    viewport.width = viewportWidth;
    viewport.height = viewportHeight;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    hina_cmd_set_viewport(frame.cmd, &viewport);

    // Bind pipeline
    hina_cmd_bind_pipeline(frame.cmd, m_ui2dPipeline);

    // Bind vertex/index buffers
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = uiFrame.vertexBuffer;
    vertInput.vertex_offsets[0] = 0;
    vertInput.index_buffer = uiFrame.indexBuffer;
    vertInput.index_type = HINA_INDEX_UINT32;
    hina_cmd_apply_vertex_input(frame.cmd, &vertInput);

    // Set frame-level push constants (projection)
    struct {
        float LRTB[4];
        uint32_t textureId;
        uint32_t samplerId;
    } pushConstants;
    pushConstants.LRTB[0] = 0.0f;           // Left
    pushConstants.LRTB[1] = viewportWidth;  // Right
    pushConstants.LRTB[2] = 0.0f;           // Top
    pushConstants.LRTB[3] = viewportHeight; // Bottom
    pushConstants.textureId = 0;
    pushConstants.samplerId = 0;

    // Sort commands by layer, then by sortOrder
    std::vector<const ui::PrimitiveDrawCommand*> sortedCommands;
    sortedCommands.reserve(drawList.commands.size());
    for (const auto& cmd : drawList.commands) {
        sortedCommands.push_back(&cmd);
    }
    std::sort(sortedCommands.begin(), sortedCommands.end(),
        [](const ui::PrimitiveDrawCommand* a, const ui::PrimitiveDrawCommand* b) {
            if (a->layer != b->layer) return a->layer < b->layer;
            return a->sortOrder < b->sortOrder;
        });

    // Draw each command
    for (const ui::PrimitiveDrawCommand* cmdPtr : sortedCommands) {
        const ui::PrimitiveDrawCommand& drawCmd = *cmdPtr;

        // Decode texture handle and get texture view
        uint32_t texId = static_cast<uint32_t>(drawCmd.textureId);
        gfx::TextureView texView = m_defaultWhiteTextureView;

        if (texId != 0) {
            // Decode TextureHandle from ID
            gfx::TextureHandle h;
            h.index = static_cast<uint16_t>(texId & 0xFFFF);
            h.generation = static_cast<uint16_t>((texId >> 16) & 0xFFFF);
            gfx::TextureView decoded = m_materialSystem.getTextureView(h);
            if (hina_texture_view_is_valid(decoded)) {
                texView = decoded;
            }
        }

        // Select sampler based on mode
        gfx::Sampler sampler = m_ui2dLinearSampler;
        if (drawCmd.samplerIndex == static_cast<uint32_t>(ui::SamplerMode::Nearest)) {
            sampler = m_ui2dNearestSampler;
        } else if (drawCmd.samplerIndex == static_cast<uint32_t>(ui::SamplerMode::Font)) {
            sampler = m_ui2dFontSampler;
        }

        // Create transient bind group
        hina_transient_bind_group tbg = hina_alloc_transient_bind_group(m_uiBindGroupLayout);
        hina_transient_write_combined_image(&tbg, 0, texView, sampler);
        hina_cmd_bind_transient_group(frame.cmd, 0, tbg);

        // Push constants
        hina_cmd_push_constants(frame.cmd, 0, sizeof(pushConstants), &pushConstants);

        // Set scissor rect
        float clipMinX = std::clamp(drawCmd.clipRect.x, 0.0f, viewportWidth);
        float clipMinY = std::clamp(drawCmd.clipRect.y, 0.0f, viewportHeight);
        float clipMaxX = std::clamp(drawCmd.clipRect.z, 0.0f, viewportWidth);
        float clipMaxY = std::clamp(drawCmd.clipRect.w, 0.0f, viewportHeight);

        if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) continue;

        hina_scissor scissor = {};
        scissor.x = static_cast<uint32_t>(clipMinX);
        scissor.y = static_cast<uint32_t>(clipMinY);
        scissor.width = static_cast<uint32_t>(clipMaxX - clipMinX);
        scissor.height = static_cast<uint32_t>(clipMaxY - clipMinY);
        hina_cmd_set_scissor(frame.cmd, &scissor);

        // Draw
        hina_cmd_draw_indexed(frame.cmd, drawCmd.indexCount, 1, drawCmd.indexOffset, 0, 0);
    }

    hina_cmd_end_pass(frame.cmd);
}

// ============================================================================
// Draw Queue Implementation
// ============================================================================

void GfxRenderer::submitMesh(gfx::MeshHandle mesh, const glm::mat4& transform, gfx::MaterialHandle material) {
    if (!m_meshStorage.isValid(mesh)) return;

    DrawCall call;
    call.mesh = mesh;
    call.material = material;
    call.transform = transform;
    call.useMaterial = true;
    m_drawQueue.push_back(call);
}

void GfxRenderer::submitMesh(gfx::MeshHandle mesh, const glm::mat4& transform, const glm::vec4& baseColor) {
    if (!m_meshStorage.isValid(mesh)) {
        LOG_WARNING("[GfxRenderer] submitMesh: invalid mesh handle {}:{}", mesh.index, mesh.generation);
        return;
    }

    DrawCall call;
    call.mesh = mesh;
    call.transform = transform;
    call.baseColor = baseColor;
    call.useMaterial = false;
    m_drawQueue.push_back(call);
}

void GfxRenderer::clearDrawQueue() {
    m_drawQueue.clear();
}

void GfxRenderer::submitScene(const Resource::Scene& scene,
                              const Resource::ResourceManager& resourceMngr) {
    for (const auto& obj : scene.objects) {
        // Only render mesh objects with valid handles
        if (!obj.isRenderable()) continue;

        // Get mesh GPU data to retrieve the gfx::MeshHandle
        const ::MeshGPUData* meshData = resourceMngr.getMesh(obj.mesh);
        if (!meshData || !meshData->hasGfxMesh) continue;

        // Reconstruct gfx::MeshHandle from stored index/generation
        gfx::MeshHandle gfxMesh;
        gfxMesh.index = meshData->gfxMeshIndex;
        gfxMesh.generation = meshData->gfxMeshGeneration;

        // Try to get material handle
        gfx::MaterialHandle gfxMaterial;
        if (obj.material.isValid()) {
            const auto* matHotData = resourceMngr.getMaterialHotData(obj.material);
            if (matHotData && matHotData->hasGfxMaterial) {
                gfxMaterial.index = matHotData->gfxMaterialIndex;
                gfxMaterial.generation = matHotData->gfxMaterialGeneration;
            }
        }

        // Submit with material if available, otherwise use default gray
        if (gfxMaterial.isValid()) {
            submitMesh(gfxMesh, obj.transform, gfxMaterial);
        } else {
            submitMesh(gfxMesh, obj.transform, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
        }
    }
}

// EnTT integration disabled - using internal ECS
// void GfxRenderer::submitEcsEntities(const entt::registry& registry,
//                                     const Resource::ResourceManager& resourceMngr,
//                                     const Resource::ResourceRegistry& resourceRegistry) {
//     // Iterate all entities with both TransformComponent and MeshRendererComponent
//     auto view = registry.view<TransformComponent, MeshRendererComponent>();
//
//     for (auto entity : view) {
//         const auto& transform = view.get<TransformComponent>(entity);
//         const auto& meshRenderer = view.get<MeshRendererComponent>(entity);
//
//         // Skip invisible entities
//         if (!meshRenderer.IsVisible()) continue;
//
//         // Compute world transform (includes parent hierarchy) - shared for all entries
//         glm::mat4 worldMatrix = transform.GetWorldMatrix(registry, entity);
//
//         // Iterate over all mesh entries in this component
//         for (const auto& entry : meshRenderer.GetEntries()) {
//             // Get mesh name and look up handle
//             const std::string& meshName = entry.meshName;
//             if (meshName.empty()) continue;
//
//             // Look up mesh handle from resource registry
//             ::MeshHandle resourceMesh = resourceRegistry.GetMeshByPath(meshName);
//             if (!resourceMesh.isValid()) continue;
//
//             // Get mesh GPU data to retrieve the gfx::MeshHandle
//             const ::MeshGPUData* meshData = resourceMngr.getMesh(resourceMesh);
//             if (!meshData || !meshData->hasGfxMesh) continue;
//
//             // Reconstruct gfx::MeshHandle from stored index/generation
//             gfx::MeshHandle gfxMesh;
//             gfxMesh.index = meshData->gfxMeshIndex;
//             gfxMesh.generation = meshData->gfxMeshGeneration;
//
//             // Try to get material handle
//             gfx::MaterialHandle gfxMaterial;
//             const std::string& materialName = entry.materialName;
//             if (!materialName.empty()) {
//                 ::MaterialHandle resourceMaterial = resourceRegistry.GetMaterialByPath(materialName);
//                 if (resourceMaterial.isValid()) {
//                     const auto* matHotData = resourceMngr.getMaterialHotData(resourceMaterial);
//                     if (matHotData && matHotData->hasGfxMaterial) {
//                         gfxMaterial.index = matHotData->gfxMaterialIndex;
//                         gfxMaterial.generation = matHotData->gfxMaterialGeneration;
//                     }
//                 }
//             }
//
//             // Submit with material if available, otherwise use base color from component
//             if (gfxMaterial.isValid()) {
//                 submitMesh(gfxMesh, worldMatrix, gfxMaterial);
//             } else {
//                 submitMesh(gfxMesh, worldMatrix, meshRenderer.GetBaseColor());
//             }
//         }
//     }
// }

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
