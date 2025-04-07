#pragma once
/******************************************************************************/
/*!
\file   Renderer.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
a class that handles rendering of objects, text and debug lines. The only class that bridges the game logic and the rendering pipeline.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/


#include "CameraController.h"
#include "LightingSystem.h"
#include "Mesh3D.h"
#include "RenderComponent.h"
#include "LightComponent.h"
#include "PostProcessingComponent.h"
#include "TextComponent.h"
#include "TextureManager.h"
#include "TrailComponent.h"
#include "VulkanContext.h"
#include "VulkanManager.h"
#include "VkInit.h"

class Renderer {
    friend class VulkanContext;
    friend class LightResources;
    public:
    explicit Renderer(VulkanContext* context);
    ~Renderer();
    void drawFrame();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept;
    Renderer& operator=(Renderer&&) noexcept;

    void initialize();

    VkClearValue getClearColor() const;
    void setClearColor(float r, float g, float b, float a);
    void setClearColor(const VkClearValue& color);
    void updateCameraData(const CameraData& camera_data);
    void updateProjectionMatrix(const glm::mat4& projection);
    void updateCameraPosition(const glm::vec2& position);
    void updateCameraZoom(float zoom);
    void updateCameraRotation(float rotation);
    void resize(uint32_t width, uint32_t height);
    void resizeViewport(uint32_t width, uint32_t height);
    VkExtent2D getViewport() const;

    void updatePostProcessing(const PostProcessingComponent& post_processing);
    void ResetPostProcessing();

    void renderDebugBounds(const Transform& transform);
    void AddRenderInstance(const RenderComponent& render_component);
    void AddTextInstance(const TextComponent& text_component);
    void AddTrailInstance(const TrailRendererComponent& trailComp);
    void AddLineInstance(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color);
    void AddLightInstance(const LightComponent& light_component);
    private:
    void renderSprites(std::vector<SpriteInstanceData>& sprites, AllocatedBuffer& buffer, VkDeviceSize& size) const;
    //void renderText();
    void renderDebug();
    void renderShadows();
    void renderEmissiveGlow();
    void composePasses();
    void renderPostProcessing();
    //
#ifdef IMGUI_ENABLED
    void renderImGui(VkCommandBuffer cmd, uint32_t index);
#endif
    struct GlowTrailInstanceData {
        // Trail geometry (packed into 3 vec4s)
        glm::vec4 points;       // (startPoint.x, startPoint.y, endPoint.x, endPoint.y)
        glm::vec4 perps;        // (startPerp.x, startPerp.y, endPerp.x, endPerp.y)
        glm::vec4 widthsAges;   // (startWidth, endWidth, startAge, endAge)
        
        // Color and glow parameters (2 vec4s)
        glm::vec4 glowColor;    // (r, g, b, a)
        glm::vec4 glowParams;   // (intensity, decay, smoothing, unused)
        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct SpriteBatches {
        std::vector<SpriteInstanceData> lit;        // sprites that receieve light
        std::vector<SpriteInstanceData> non_lit;         // the latter
        std::vector<GlowTrailInstanceData> glow;
        void clear();
    };
    SpriteBatches m_spriteBatches;
    //std::vector<SpriteInstanceData> m_finalSpriteBuffer; // Buffer for GPU upload
    //std::vector<TextInstanceData> _glyphs;
    std::vector<LineInstanceData> _lines;
    VulkanContext* m_context;
    glm::mat4 m_projectionMatrix{};
    float m_zoom{ 1.0f };
    float m_rotation{ 0.0f };
    glm::vec2 m_cameraPosition{};
    VkViewport m_viewport{};
    VkRect2D m_scissor{};
    screenData m_screenData;
    uint32_t imageIndex{};
    VkClearValue clearColor = { {0.1f, 0.1f, 0.1f, 1.0f} };
    bool projection_dirty{ false };

    struct RenderTarget {
        AllocatedImage image;
        VkImageView RenderView;
#ifdef IMGUI_ENABLED
        VkImageView ImGuiView;
#endif
        VkSampler sampler;
        VkFormat format;
        DescriptorSetManager::DescriptorSetHandle descriptorHandle;
        void cleanup();
    };

    struct Target
    {
        AllocatedImage image;
        VkImageView view;
        VkSampler sampler;
        VkFormat format;
        DescriptorSetManager::DescriptorSetHandle descriptorHandle;
        void cleanup();
    };

    VkDescriptorSetLayout m_renderTargetLayout{};
    std::array<RenderTarget, Constant::FRAME_OVERLAP> m_renderTargets{};
    std::array<Target, Constant::FRAME_OVERLAP> m_lightingTargets;
    Target createBloomTarget(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage);

    bool m_bloomEnabled = false;
    float m_bloomThreshold = 0.01f;
    float m_blurSize = 1.0f;
    float m_bloomIntensity = 1.0f;

    bool m_vignetteEnabled = false;
    struct VignetteSettings {
        glm::vec4 vignetteColor = {0.0f,0.0f,0.0f,1.0f};    // Color of the vignette
        float vignetteIntensity{2.0f};     // Overall intensity
        float vignetteRadius{0.5f};        // How far it extends from corners
        float vignetteSmoothness{0.33f};    // Edge softness
    } m_vignetteSettings;

    struct BloomResources
    {
        Target bloomTextureA; // Bright-pass output
        Target bloomTextureB; // Horizontal blur output
        Target bloomTextureC; // Vertical blur output

        void cleanup();
    };

    std::array<BloomResources, Constant::FRAME_OVERLAP> m_bloomTargets;
    Mesh2D m_quad;
    public:
    struct LightingManager {
        // Core configuration constants
        static constexpr uint32_t SHADOW_MAP_WIDTH = 1024;        // High-resolution shadow maps
        static constexpr uint32_t MAX_ACTIVE_LIGHTS = 64;         // Hardware-optimized light limit
        static constexpr uint32_t MAX_ACTIVE_BLOCKERS = 4096;       // Hardware-optimized blocker limit
        static constexpr VkFormat SHADOW_MAP_FORMAT = VK_FORMAT_R32_SFLOAT;  // High-precision shadow storage

        struct ShadowCaster
        {
            glm::vec2 start;
            glm::vec2 end;
        };

        struct LightFlags {
            static constexpr uint32_t Enabled = 1u << 0;
            static constexpr uint32_t CastShadows = 1u << 1;
            static constexpr uint32_t IsSpot = 1u << 2;
        };

        struct GPULightProperties {
            glm::vec3 color;          // Light color (12 bytes)
            float intensity;           // Light intensity multiplier (4 bytes)

            // Light shape parameters
            float radius;              // Maximum light influence (4 bytes)
            float innerRadius;         // Full-brightness radius (4 bytes)
            float distanceFalloff;     // Distance attenuation exponent (4 bytes)

            // Spot light parameters
            float coneFalloff;         // Angular falloff exponent (4 bytes)

            // Packed state flags - minimize memory while maintaining alignment
            uint32_t flags;            // Enabled, shadows, spot flags (4 bytes)
            glm::vec3 padding;         // Padding to 16 bytes
        };

        struct LightData {
            glm::vec2 position;
            float lightAngle;
            float lightConeAngle;
        };

        // Shadow Map Resources
        struct ShadowMapResources {
            AllocatedImage shadowMapImage;                // Shadow map texture (1024 x numLights)
            VkImageView shadowMapImageView;               // Image view for the shadow map
            AllocatedImage finalShadowMapImage;           // Final shadow map texture (1024 x numLights)
            VkImageView finalShadowMapImageView;          // Image view for the final shadow map
            VkSampler shadowMapSampler;                   // Sampler for shadow map texture
            //DescriptorSetManager::DescriptorSetHandle shadowMapDescriptorSet;       // Descriptor set for shadow maps
            void cleanup();
        };
        // Light Data
        struct LightResources {
            AllocatedBuffer lightBuffer;                  // Storage buffer for light data// Size of the light buffer
            AllocatedBuffer lightPropertiesBuffer; // Extended light properties
            DescriptorSetManager::DescriptorSetHandle LightDescriptorSet;

            void cleanup(); // Descriptor set for light data
        };
        VkDescriptorSetLayout lightingPassDescriptorLayout;  // Descriptor set layout for light data
        // Blocker Geometry
        struct ShadowCasterResources {
            AllocatedBuffer blockerBuffer;                // Storage buffer for blocker geometry
            uint32_t activeVertexCount = 0;  // Number of active vertices for rendering
            /*DescriptorSetManager::DescriptorSetHandle blockerDescriptorSet;*/
        };
        /*VkDescriptorSetLayout blockerDescriptorLayout; // Descriptor set layout for blocker data*/
        struct ComputeResources {
            DescriptorSetManager::DescriptorSetHandle shadowRefinement;  // For the second compute pass
        };
        VkDescriptorSetLayout shadowMapRefinementLayout; // Descriptor set layout for shadow map refinement
        // Per-Frame Data
        struct FrameData {
            ShadowMapResources shadowMapResources;
            LightResources lightResources;
            ShadowCasterResources shadowCasterResources;
            ComputeResources computeResources;
            std::vector<GPULightProperties> lights;           // Active lights for this frame
            std::vector<LightData> lightData;              // Light data for this frame
            std::vector<ShadowCaster> blockers;                // Active blockers for this frame

            void clear();
        };
        std::array<FrameData, Constant::FRAME_OVERLAP> frameStates; // Per-frame light and blocker data
        Renderer* m_renderer;

        // Initialization and cleanup
        void initialize(Renderer* renderr);
        void cleanup();

        void addLight(const LightComponent& light, uint32_t frameIndex);
        void addBlocker(const Transform& transform, uint32_t frameIndex);

        void updateLightingData(uint32_t frameIndex);

        void createDescriptorSetLayouts();
        void createShadowMapResources();
        void initializeFrameResources();

    }m_lightingSystem;

    private:
    void uploadQuad();
    void initializeQuad();

    void resizeAllTargets();
    void updateCameraBuffer();
    template<typename InstanceType , typename ZDepthFunc>
    void updateInstanceBuffer(std::vector<InstanceType>& instances, 
                                   AllocatedBuffer& instanceBuffer, 
                                   VkDeviceSize& bufferSize,
                                   ZDepthFunc getZDepth) const;

    void updateTextureBuffer(std::vector<SpriteInstanceData>& sprites, AllocatedBuffer& buffer, VkDeviceSize& size) const;
    void updateLineBuffer();
    bool isInViewport(const glm::vec2& position, const glm::vec2& size, float rotation) const;

};

