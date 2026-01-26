#pragma once

#include <hina_vk.h>
#include <cstdint>
#include <vector>
#include <string>

namespace AssetCompiler
{
    /**
     * ThumbnailRenderer - Headless Vulkan renderer for generating asset thumbnails.
     *
     * Uses hina_vk in headless mode (no window) to render:
     * - Material thumbnails: sphere with material applied
     * - Mesh thumbnails: mesh from standard viewing angle
     */
    class ThumbnailRenderer
    {
    public:
        static constexpr uint32_t THUMBNAIL_SIZE = 64;

        ThumbnailRenderer() = default;
        ~ThumbnailRenderer();

        // Non-copyable
        ThumbnailRenderer(const ThumbnailRenderer&) = delete;
        ThumbnailRenderer& operator=(const ThumbnailRenderer&) = delete;

        /**
         * Initialize headless Vulkan context and rendering resources.
         * @return true if initialization succeeded
         */
        bool Initialize();

        /**
         * Shutdown and release all resources.
         */
        void Shutdown();

        /**
         * Check if renderer is initialized.
         */
        bool IsInitialized() const { return m_initialized; }

        /**
         * Render a material preview (sphere with solid color).
         * @param baseColor RGBA color (0-1 range)
         * @param outPixels Output pixel buffer (RGBA8, 64x64)
         * @return true if rendering succeeded
         */
        bool RenderMaterialPreview(const float baseColor[4], std::vector<uint8_t>& outPixels);

        /**
         * Render a mesh preview.
         * @param positions Vertex positions (vec3)
         * @param indices Triangle indices
         * @param outPixels Output pixel buffer (RGBA8, 64x64)
         * @return true if rendering succeeded
         */
        bool RenderMeshPreview(const std::vector<float>& positions,
                               const std::vector<uint32_t>& indices,
                               std::vector<uint8_t>& outPixels);

    private:
        bool CreateRenderTarget();
        bool CreateShaderAndPipeline();
        bool CreateSphereMesh();
        void DestroyResources();

        bool m_initialized = false;

        // Render target
        hina_texture m_colorTarget = {};
        hina_texture_view m_colorTargetView = {};
        hina_texture m_depthTarget = {};
        hina_texture_view m_depthTargetView = {};

        // Pipeline resources
        hina_pipeline m_pipeline = {};
        hina_bind_group_layout m_bindGroupLayout = {};
        hina_sampler m_sampler = {};

        // Sphere mesh for material preview
        hina_buffer m_sphereVertexBuffer = {};
        hina_buffer m_sphereIndexBuffer = {};
        uint32_t m_sphereIndexCount = 0;

        // Uniform buffer
        hina_buffer m_uniformBuffer = {};
        void* m_uniformMapped = nullptr;
    };
}
