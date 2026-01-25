/**
 * @file gfx_renderer.h
 * @brief Graphics renderer backend for mobile-first rendering.
 *
 * This renderer implements the HypeHype-inspired architecture:
 * - CPU-driven rendering (no GPU-driven features)
 * - 3 descriptor sets (Android minimum)
 * - Deferred G-buffer (64-bit)
 * - CPU tile-binned lighting
 * - Octahedral shadow atlas
 *
 * Part of the MagicEngine -> hina-vk migration.
 */

#pragma once

#include "gfx_types.h"
#include "gfx_mesh_storage.h"
#include "gfx_material_system.h"
#include "i_renderer.h"
#include "frame_data.h"
#include "linear_color.h"
#include "renderer/ui/ui_primitives.h"
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <atomic>
#include <functional>

// Forward declarations
struct FrameData;
struct ImDrawData;
class RenderGraph;
class HinaContext;
class IRenderFeature;

// EnTT integration disabled - using internal ECS
// #include <entt/entity/fwd.hpp>

namespace Resource {
    struct Scene;
    class ResourceManager;
    class ResourceRegistry;
}

namespace ui {
    struct PrimitiveDrawList;
}

// ============================================================================
// GfxRenderer Configuration Constants
// ============================================================================

constexpr uint32_t GFX_MAX_FRAMES = 3;
constexpr uint32_t TILE_SIZE = 32;           // Light tile size in pixels
constexpr uint32_t MAX_LIGHTS = 64;          // Max lights per frame (64-bit mask)
constexpr uint32_t MAX_MATERIALS = 256;      // Max materials per frame
constexpr uint32_t MAX_TRANSFORMS = 4096;    // Max draw calls per frame (reduced for 256-byte alignment)
constexpr uint32_t TRANSFORM_ALIGNMENT = 256; // Dynamic UBO offset alignment (mobile requirement)
constexpr uint32_t TRANSFORM_UBO_SIZE = MAX_TRANSFORMS * TRANSFORM_ALIGNMENT;

// Skinning constants
constexpr uint32_t MAX_SKINNED_DRAWS = 256;  // Max skinned objects per frame
constexpr uint32_t BONE_MATRIX_UBO_SIZE = MAX_SKINNED_DRAWS * gfx::BONE_MATRICES_SIZE;  // 256 * 16KB = 4MB

// ============================================================================
// View Output System
// ============================================================================

/**
 * @brief View IDs for different rendering contexts.
 * Each view can have its own output texture that can be displayed in ImGui.
 */
enum class ViewId : uint32_t {
    Game = 0,      // Game view (what the player sees)
    Scene = 1,     // Scene view (editor view with gizmos)
    Preview = 2,   // Material/asset preview
    Count
};

/**
 * @brief Per-view output resources.
 */
struct ViewOutput {
    gfx::Texture texture;
    gfx::TextureView view;
    gfx::Sampler sampler;
    uint32_t width = 0;
    uint32_t height = 0;
    bool valid = false;
};

// ============================================================================
// Bind Group Layout Definitions
// ============================================================================

/**
 * @brief Bind group set indices (Android minimum is 4, we use 3 for safety)
 */
enum class BindGroupSet : uint32_t {
    Globals = 0,    // Per-pass: frame constants, lighting, shadow atlas
    Material = 1,   // Per-material: textures, material constants
    Dynamic = 2,    // Per-draw: transforms via dynamic offsets
};

/**
 * @brief Binding indices for Set 0 (Globals)
 */
namespace GlobalBindings {
    constexpr uint32_t FrameConstants = 0;   // UBO: view, proj, camera, time
    constexpr uint32_t LightingData = 1;     // UBO: tile light data
    constexpr uint32_t ShadowAtlas = 2;      // sampler2D: shadow maps
    constexpr uint32_t AmbientOctMap = 3;    // sampler2D: indirect lighting
}

/**
 * @brief Binding indices for Set 1 (Material)
 */
namespace MaterialBindings {
    constexpr uint32_t Constants = 0;        // UBO: packed material (16 bytes)
    constexpr uint32_t Albedo = 1;           // combined_image_sampler
    constexpr uint32_t Normal = 2;           // combined_image_sampler
    constexpr uint32_t MetallicRoughness = 3;// combined_image_sampler
    constexpr uint32_t Emissive = 4;         // combined_image_sampler
    constexpr uint32_t Occlusion = 5;        // combined_image_sampler
}

/**
 * @brief Binding indices for Set 2 (Dynamic)
 */
namespace DynamicBindings {
    constexpr uint32_t Transform = 0;        // UBO + DYNAMIC_OFFSET: instance data
    constexpr uint32_t BoneMatrices = 1;     // UBO + DYNAMIC_OFFSET: bone matrices (for skinning)
    constexpr uint32_t ObjectData = 2;       // UBO + DYNAMIC_OFFSET: tint, flags
}

// ============================================================================
// Per-Frame Resources (Triple Buffered)
// ============================================================================

/**
 * @brief Resources that are triple-buffered for each frame in flight.
 */
struct FrameResources {
    // Global UBOs
    gfx::Buffer frameConstantsUBO;
    gfx::Buffer lightingDataUBO;

    // Transform ring buffer (dynamic offsets) - persistently mapped
    gfx::Buffer transformRingUBO;
    void* transformRingMapped = nullptr;  // Persistently mapped pointer
    uint32_t transformRingOffset = 0;

    // Bone matrix ring buffer (for skinned meshes) - persistently mapped
    gfx::Buffer boneMatrixRingUBO;
    void* boneMatrixRingMapped = nullptr;  // Persistently mapped pointer
    uint32_t boneMatrixRingOffset = 0;

    // Object data ring buffer
    gfx::Buffer objectDataRingUBO;
    uint32_t objectDataRingOffset = 0;

    // Bind groups
    gfx::BindGroup globalBindGroup;
    gfx::BindGroup dynamicBindGroup;

    // Sync
    gfx::SyncPoint graphicsComplete;
    gfx::SyncPoint computeComplete;

    // Command buffer pool
    gfx::Cmd* cmd = nullptr;

    // Current frame's swapchain image (acquired in beginFrame)
    gfx::SwapchainImage swapchainImage;
    gfx::TextureView swapchainView;

    // Frame index for debugging
    uint32_t frameIndex = 0;
};

// ============================================================================
// Render Targets
// ============================================================================

/**
 * @brief G-Buffer render targets (64-bit color, fits Mali tile budget)
 */
struct GBuffer {
    gfx::Texture albedo;          // RGBA8_SRGB: base color (32 bits)
    gfx::Texture normal;          // R16G16_SFLOAT: octahedral normal (32 bits)
    gfx::Texture materialData;    // RGBA8: roughness, metallic, matID, flags (32 bits)
    gfx::Texture depth;           // D32F: depth buffer
    gfx::Texture visibilityID;    // R32UI: object picking

    // Cached default views for render pass setup
    gfx::TextureView albedoView;
    gfx::TextureView normalView;
    gfx::TextureView materialDataView;
    gfx::TextureView depthView;
    gfx::TextureView visibilityIDView;
};

/**
 * @brief WBOIT (Weighted Blended OIT) targets
 */
struct WBOITTargets {
    gfx::Texture accumulation;    // RGBA16F: weighted color accumulation
    gfx::Texture reveal;          // R8: reveal (product of 1-alpha)

    gfx::TextureView accumulationView;
    gfx::TextureView revealView;
};

// ============================================================================
// Light Tile Binner
// ============================================================================

/**
 * @brief CPU-side light tile binner (32x32 tiles, 64-bit masks)
 */
class LightTileBinner {
public:
    void resize(uint32_t screenWidth, uint32_t screenHeight);
    void clear();

    /**
     * @brief Add a light to the tile map
     * @param lightIndex Index in the light array (0-63)
     * @param screenBounds Screen-space AABB [minX, minY, maxX, maxY]
     * @param zMin Minimum depth of light influence
     * @param zMax Maximum depth of light influence
     */
    void addLight(uint32_t lightIndex, const glm::vec4& screenBounds, float zMin, float zMax);

    /**
     * @brief Get the light mask for a tile
     */
    uint64_t getTileMask(uint32_t tileX, uint32_t tileY) const;

    /**
     * @brief Get tile dimensions
     */
    uint32_t getTileCountX() const { return m_tileCountX; }
    uint32_t getTileCountY() const { return m_tileCountY; }

private:
    std::vector<gfx::LightTile> m_tiles;
    uint32_t m_tileCountX = 0;
    uint32_t m_tileCountY = 0;
    uint32_t m_screenWidth = 0;
    uint32_t m_screenHeight = 0;
};

// ============================================================================
// Shadow Atlas
// ============================================================================

/**
 * @brief Octahedral shadow atlas allocator
 */
class ShadowAtlasAllocator {
public:
    struct Region {
        uint32_t x, y;           // Position in atlas
        uint32_t size;           // Size (square)
        uint32_t lightIndex;     // Which light owns this
        bool isOctahedral;       // Point light = octahedral, spot = standard
    };

    void init(uint32_t atlasSize);
    Region allocate(uint32_t size, uint32_t lightIndex, bool octahedral);
    void free(uint32_t lightIndex);
    void reset();

private:
    std::vector<Region> m_regions;
    uint32_t m_atlasSize = 0;
    uint32_t m_nextY = 0;
};

// ============================================================================
// CPU Culler
// ============================================================================

/**
 * @brief CPU-based frustum culling and sorting
 */
class CPUCuller {
public:
    void setFrustum(const glm::mat4& viewProj);

    /**
     * @brief Test if an AABB is visible
     */
    bool isVisible(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const;

    /**
     * @brief Cull and sort objects
     * @param objects Input objects to cull
     * @param cameraPos Camera position for sorting
     * @param frontToBack True for opaque (front-to-back), false for transparent (back-to-front)
     * @return Sorted list of visible objects
     */
    std::vector<gfx::VisibleObject> cullAndSort(
        const std::vector<gfx::VisibleObject>& objects,
        const glm::vec3& cameraPos,
        bool frontToBack = true) const;

private:
    glm::vec4 m_frustumPlanes[6];
};

// ============================================================================
// GfxRenderer
// ============================================================================

/**
 * @brief Main renderer class using hina-vk.
 * Implements IRenderer for feature management.
 */
class GfxRenderer : public IRenderer {
public:
    GfxRenderer();
    ~GfxRenderer() override;

    // Lifecycle
    bool initialize(void* nativeWindow, uint32_t width, uint32_t height);
    void shutdown();

    // Frame loop
    bool beginFrame();
    void render(FrameData& frameData);  // Single view (legacy)
    void render(RenderFrameData& frameData);  // Multi-view (preferred)
    void endFrame();

    // Surface lifecycle management (for Android)
    void handleSurfaceCreated(void* nativeWindow);
    void handleSurfaceDestroyed();

    // Window management
    void onResize(uint32_t width, uint32_t height);

    // Accessors
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    uint32_t getCurrentFrame() const { return m_currentFrame; }
    bool isInitialized() const { return m_initialized; }

    // Camera - set from editor/game camera each frame
    void setViewMatrix(const glm::mat4& view) { m_viewMatrix = view; m_cameraMatricesSet = true; }
    void setProjMatrix(const glm::mat4& proj) { m_projMatrix = proj; m_cameraMatricesSet = true; }
    void setViewProjMatrices(const glm::mat4& view, const glm::mat4& proj)
    {
        m_viewMatrix = view;
        m_projMatrix = proj;
        m_cameraMatricesSet = true;
    }
    const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& getProjMatrix() const { return m_projMatrix; }

    // ImGui integration
    bool initImGui();
    void shutdownImGui();
    void renderImGui(ImDrawData* drawData);
    bool isImGuiInitialized() const { return m_imguiInitialized; }

    // 2D UI rendering (game UI overlay)
    bool initUI2D();
    void shutdownUI2D();
    void queueUI2D(const ui::PrimitiveDrawList& drawList, float viewportWidth, float viewportHeight);
    bool isUI2DInitialized() const { return m_ui2dInitialized; }

    // Register a texture for UI use, returns encoded texture ID
    // The ID encodes the TextureHandle for use with transient bind groups at render time
    uint32_t registerUITexture(gfx::TextureHandle texHandle);

    // Get the UI bind group layout (for external use, e.g., custom UI rendering)
    gfx::BindGroupLayout getUIBindGroupLayout() const { return m_uiBindGroupLayout; }

    // View output system - get texture IDs for viewport display
    uint64_t getViewOutputTextureId(ViewId viewId) const;
    void setActiveView(ViewId viewId) { m_activeViewId = viewId; }
    ViewId getActiveView() const { return m_activeViewId; }

    // Feature mask - controls which render features execute for the active view
    void setFeatureMask(uint64_t mask) { m_featureMask = mask; }
    uint64_t getFeatureMask() const { return m_featureMask; }

    // Feature flag constants (matching RenderFeatures in frame_data.h)
    static constexpr uint64_t FeatureScene         = 1ULL << 0;
    static constexpr uint64_t FeatureGrid          = 1ULL << 1;
    static constexpr uint64_t FeatureGizmos        = 1ULL << 2;
    static constexpr uint64_t FeatureDebugOverlays = 1ULL << 3;
    static constexpr uint64_t FeaturePostProcess   = 1ULL << 4;
    static constexpr uint64_t FeatureShadows       = 1ULL << 5;
    static constexpr uint64_t FeatureUI            = 1ULL << 6;

    // Resize a specific view's output (if different from main size)
    void resizeViewOutput(ViewId viewId, uint32_t width, uint32_t height);

    // Window ready state (after minimum frames rendered)
    bool isWindowReadyForShow() const { return m_windowReadyForShow; }

    // Tone mapping settings
    const ToneMappingSettings& GetToneMappingSettings() const;
    void UpdateToneMappingSettings(const ToneMappingSettings& newSettings);

    // ========================================================================
    // IRenderer Interface (Feature System)
    // ========================================================================

    /**
     * @brief Destroy a feature by handle.
     */
    void DestroyFeature(uint64_t feature_handle) override;

    /**
     * @brief Get the parameter block pointer for a feature.
     */
    void* GetFeatureParameterBlockPtr(uint64_t feature_handle) override;

    /**
     * @brief Get the feature mask for a feature handle.
     */
    FeatureMask GetFeatureMask(uint64_t feature_handle) const;

    /**
     * @brief Alias for GetFeatureMask (backward compatibility).
     */
    FeatureMask GetFeatureMaskForHandle(uint64_t feature_handle) const { return GetFeatureMask(feature_handle); }

    /**
     * @brief Get a feature by handle and cast to the expected type.
     */
    template <typename TFeature>
    TFeature* GetFeature(uint64_t feature_handle)
    {
        auto it = m_features.find(feature_handle);
        if (it == m_features.end() || !it->second.feature) {
            return nullptr;
        }
        return dynamic_cast<TFeature*>(it->second.feature.get());
    }

    // ========================================================================
    // Feature Registration (for RenderGraph integration)
    // ========================================================================

    /**
     * @brief Register a render feature with the renderer.
     * Features will have their passes executed during the render loop.
     */
    void registerFeature(IRenderFeature* feature);

    /**
     * @brief Unregister a render feature.
     */
    void unregisterFeature(IRenderFeature* feature);

    /**
     * @brief Get the feature mask for a registered feature pointer.
     * @return The feature's bit mask, or 0 if not registered.
     */
    FeatureMask getFeatureMask(IRenderFeature* feature) const;

    /**
     * @brief Get access to the HinaContext for features that need vk::IContext.
     */
    HinaContext* getHinaContext() const { return m_hinaContext.get(); }

    // Resource systems
    gfx::GfxMeshStorage& getMeshStorage() { return m_meshStorage; }
    const gfx::GfxMeshStorage& getMeshStorage() const { return m_meshStorage; }
    gfx::GfxMaterialSystem& getMaterialSystem() { return m_materialSystem; }
    const gfx::GfxMaterialSystem& getMaterialSystem() const { return m_materialSystem; }

    // Frustum culling interface
    void setFrustum(const glm::mat4& viewProj) { m_culler.setFrustum(viewProj); }
    bool isVisibleInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
        return m_culler.isVisible(aabbMin, aabbMax);
    }

    // Material bind group layout (Set 1: constants UBO + textures) - for features that need to create compatible pipelines
    gfx::BindGroupLayout getMaterialLayout() const { return m_materialLayout; }

    // Dynamic bind group layout (Set 2: transform UBO with dynamic offset) - for features that need to create compatible pipelines
    gfx::BindGroupLayout getDynamicLayout() const { return m_dynamicLayout; }

    // Get default sampler for external use
    gfx::Sampler getDefaultSampler() const { return m_defaultSampler; }

    // Skybox cubemap texture (set by GraphicsMain, used by scene feature)
    void setSkyboxTexture(gfx::Texture texture, gfx::TextureView view) {
        m_skyboxTexture = texture;
        m_skyboxTextureView = view;
    }
    gfx::Texture getSkyboxTexture() const { return m_skyboxTexture; }
    gfx::TextureView getSkyboxTextureView() const { return m_skyboxTextureView; }

    // Get ImGui font texture view for external rendering (e.g., ImGuiRenderFeature)
    gfx::TextureView getImGuiFontView() const { return m_imguiFontView; }

    // ========================================================================
    // Feature-invokable Render Passes
    // These methods can be called by IRenderFeature implementations
    // ========================================================================

    /**
     * @brief Execute grid pass.
     * Renders infinite grid for editor orientation.
     * @param cmd Command buffer to record into
     */
    void executeGridPass(gfx::Cmd* cmd);

    /**
     * @brief Get current frame resources.
     */
    FrameResources& getCurrentFrameResources() { return m_frames[m_currentFrame]; }

    /**
     * @brief Get G-buffer render targets.
     */
    const GBuffer& getGBuffer() const { return m_gbuffer; }

    /**
     * @brief Get WBOIT render targets.
     */
    const WBOITTargets& getWBOIT() const { return m_wboit; }

    /**
     * @brief Get view output for a specific view.
     */
    const ViewOutput& getViewOutput(ViewId viewId) const { return m_viewOutputs[static_cast<size_t>(viewId)]; }

private:
    // IRenderer interface implementation
    uint64_t DoCreateFeature(std::function<std::unique_ptr<IRenderFeature>()> factory) override;

    // Initialization helpers
    bool createBindGroupLayouts();
    bool createRenderTargets();
    bool createDefaultResources();
    bool createPipelines();
    bool createFrameResources();
    bool createViewOutput(ViewId viewId, uint32_t width, uint32_t height);

    // Per-frame helpers
    void updateFrameConstants(const FrameData& frameData);
    void cullScene(const FrameData& frameData);
    void binLights(const FrameData& frameData);
    void renderGrid();   // Infinite grid for editor orientation

    // Core state
    bool m_initialized = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_currentFrame = 0;
    FrameData* m_currentFrameData = nullptr;

    // Camera matrices (set externally via setViewProjMatrices)
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_projMatrix = glm::mat4(1.0f);
    bool m_cameraMatricesSet = false;

    // Bind group layouts (created once)
    gfx::BindGroupLayout m_globalLayout;
    gfx::BindGroupLayout m_materialLayout;
    gfx::BindGroupLayout m_dynamicLayout;

    // Per-frame resources (triple buffered)
    std::array<FrameResources, GFX_MAX_FRAMES> m_frames;

    // Render targets
    GBuffer m_gbuffer;
    WBOITTargets m_wboit;
    gfx::Texture m_hdrTarget;
    gfx::TextureView m_hdrTargetView;
    gfx::Texture m_shadowAtlas;
    gfx::TextureView m_shadowAtlasView;
    gfx::Texture m_ambientOctMap;

    // View output textures (multiple views: game, scene, preview)
    std::array<ViewOutput, static_cast<size_t>(ViewId::Count)> m_viewOutputs;
    ViewId m_activeViewId = ViewId::Scene;  // Which view is currently being rendered
    uint64_t m_featureMask = ~0ULL;         // All features enabled by default

    // CPU systems
    LightTileBinner m_lightBinner;
    ShadowAtlasAllocator m_shadowAllocator;
    CPUCuller m_culler;

    // Visible objects (populated by culling)
    std::vector<gfx::VisibleObject> m_visibleOpaque;
    std::vector<gfx::VisibleObject> m_visibleTransparent;

    // Pipelines (created at startup) - most scene rendering moved to SceneRenderFeature
    gfx::Pipeline m_gridPipeline;            // Infinite grid for editor

    // Grid rendering resources
    gfx::BindGroupLayout m_gridLayout;
    gfx::BindGroup m_gridBindGroup;
    gfx::Buffer m_gridUBO;
    void* m_gridUBOMapped = nullptr;
    gfx::Buffer m_fullscreenQuadVB;  // Shared fullscreen quad for grid and other fullscreen passes

    // Default resources
    gfx::Sampler m_defaultSampler;
    gfx::Texture m_defaultWhiteTexture;
    gfx::TextureView m_defaultWhiteTextureView;
    gfx::Texture m_defaultNormalTexture;
    gfx::TextureView m_defaultNormalTextureView;

    // Skybox cubemap texture (set by GraphicsMain after loading)
    gfx::Texture m_skyboxTexture;
    gfx::TextureView m_skyboxTextureView;

    // Shared UI bind group layout (used by ImGui, UI2D, and material system for UI textures)
    // Created early in createDefaultResources() and shared across all UI rendering
    gfx::BindGroupLayout m_uiBindGroupLayout;

    // ========================================================================
    // Resource Systems (mesh and material storage)
    // ========================================================================
    gfx::GfxMeshStorage m_meshStorage;
    gfx::GfxMaterialSystem m_materialSystem;

    // ========================================================================
    // ImGui Resources
    // ========================================================================
    static constexpr uint32_t IMGUI_MAX_VERTEX_COUNT = 65536 * 4;
    static constexpr uint32_t IMGUI_MAX_INDEX_COUNT = 65536 * 6;

    struct ImGuiFrameResources {
        gfx::Buffer vertexBuffer;
        gfx::Buffer indexBuffer;
        void* vertexMapped = nullptr;
        void* indexMapped = nullptr;
    };

    bool m_imguiInitialized = false;
    gfx::Pipeline m_imguiPipeline;
    gfx::Texture m_imguiFontTexture;
    gfx::TextureView m_imguiFontView;  // Font texture view for transient bind group
    gfx::Sampler m_imguiFontSampler;
    std::array<ImGuiFrameResources, GFX_MAX_FRAMES> m_imguiFrames;

    // ImTextureID encoding constants
    static constexpr uint64_t IMGUI_FONT_TEXTURE_ID = 0;  // Font texture uses ID 0 (ImGui default)
    static constexpr uint64_t IMGUI_VIEWOUTPUT_BIT = 1ULL << 63;  // High bit = ViewOutput pointer

    // ========================================================================
    // 2D UI Resources (game overlay)
    // ========================================================================
    static constexpr uint32_t UI2D_MAX_VERTEX_COUNT = 65536 * 4;
    static constexpr uint32_t UI2D_MAX_INDEX_COUNT = 65536 * 6;

    struct UI2DFrameResources {
        gfx::Buffer vertexBuffer;
        gfx::Buffer indexBuffer;
        void* vertexMapped = nullptr;
        void* indexMapped = nullptr;
    };

    bool m_ui2dInitialized = false;
    gfx::Pipeline m_ui2dPipeline;
    gfx::Sampler m_ui2dLinearSampler;
    gfx::Sampler m_ui2dNearestSampler;
    gfx::Sampler m_ui2dFontSampler;
    std::array<UI2DFrameResources, GFX_MAX_FRAMES> m_ui2dFrames;

    // Queued UI2D draw list (set during update, rendered during render phase)
    ui::PrimitiveDrawList m_queuedUI2DDrawList;
    float m_queuedUI2DViewportWidth = 0.0f;
    float m_queuedUI2DViewportHeight = 0.0f;
    bool m_hasQueuedUI2D = false;

    void renderUI2D();  // Called internally during render phase

    // ========================================================================
    // RenderGraph Integration
    // ========================================================================
    std::unique_ptr<HinaContext> m_hinaContext;
    std::unique_ptr<RenderGraph> m_renderGraph;
    std::vector<IRenderFeature*> m_registeredFeatures;

    // ========================================================================
    // Feature System (from IRenderer interface)
    // ========================================================================
    struct FeatureInfo {
        std::unique_ptr<IRenderFeature> feature;
        uint64_t handle;
        FeatureMask mask = 0;  // Cached feature mask from RenderGraph

        explicit FeatureInfo(std::unique_ptr<IRenderFeature> feat, uint64_t h, FeatureMask m = 0)
            : feature(std::move(feat)), handle(h), mask(m) {}
    };

    std::unordered_map<uint64_t, FeatureInfo> m_features;
    std::atomic<uint64_t> m_nextFeatureHandle{1};

    // ========================================================================
    // Frame Tracking
    // ========================================================================
    int m_framesRendered = 0;
    bool m_windowReadyForShow = false;
    static constexpr int MIN_FRAMES_BEFORE_SHOW = 3;

    // Tone mapping settings
    ToneMappingSettings m_toneMappingSettings{};
};
