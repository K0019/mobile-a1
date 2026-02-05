/**
 * @file gfx_material_system.h
 * @brief Material and texture management for hina-vk backend.
 *
 * Provides:
 * - Texture loading and GPU upload
 * - Material bind group creation (Set 1)
 * - Generational handle management
 *
 * Design:
 * - Each material gets its own bind group with textures
 * - Default textures (white, normal) used for missing maps
 * - Materials reference textures by handle
 */

#pragma once

#include "gfx_types.h"
#include <vector>
#include <string>
#include <cstdint>

namespace gfx {

// ============================================================================
// Texture System
// ============================================================================

/**
 * @brief GPU texture entry with metadata.
 */
struct TextureEntry {
    Texture texture;
    TextureView view;
    uint32_t width = 0;
    uint32_t height = 0;
    hina_format format = HINA_FORMAT_R8G8B8A8_UNORM;
    uint16_t generation = 0;
    bool inUse = false;
    bool isSRGB = true;
};

/**
 * @brief Texture creation parameters.
 */
struct TextureCreateInfo {
    const void* data = nullptr;
    uint32_t width = 1;
    uint32_t height = 1;
    hina_format format = HINA_FORMAT_R8G8B8A8_UNORM;
    bool generateMips = false;
    bool isSRGB = true;
    const char* label = nullptr;  // Debug label for RenderDoc
};

// ============================================================================
// Material System
// ============================================================================

/**
 * @brief CPU-side material definition for the gfx system.
 */
struct GfxMaterial {
    glm::vec4 baseColor = glm::vec4(1.0f);
    float roughness = 0.5f;
    float metallic = 0.0f;
    float emissive = 0.0f;  // Scalar emissive intensity (legacy, multiplied with emissiveColor)
    float ao = 1.0f;
    float rim = 0.0f;

    // Extended PBR properties
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec3 emissiveColor = glm::vec3(1.0f);  // RGB emissive color (multiplied with emissive texture)

    // Texture handles (invalid = use default)
    TextureHandle albedoTexture;
    TextureHandle normalTexture;
    TextureHandle metallicRoughnessTexture;
    TextureHandle emissiveTexture;
    TextureHandle occlusionTexture;

    // Alpha mode
    enum class AlphaMode : uint8_t {
        Opaque = 0,
        Mask = 1,
        Blend = 2
    };
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    bool unlit = false;  // Skip lighting calculations
};

/**
 * @brief GPU-side material entry.
 */
struct MaterialEntry {
    GfxMaterial material;
    PackedMaterial gpuData;
    Buffer constantsUBO;         // Per-material UBO (32 bytes, Set 1 binding 0)
    BindGroup bindGroup;         // Material bind group (Set 1)
    uint16_t generation = 0;
    bool inUse = false;
    bool dirty = true;           // Needs bind group update
};

// ============================================================================
// GfxMaterialSystem
// ============================================================================

/**
 * @brief Manages textures and materials for the gfx renderer.
 */
class GfxMaterialSystem {
public:
    GfxMaterialSystem();
    ~GfxMaterialSystem();

    /**
     * @brief Initialize with bind group layout and default resources.
     * @param materialLayout The material bind group layout from GfxRenderer
     * @param defaultSampler Default sampler for textures
     */
    bool initialize(BindGroupLayout materialLayout, Sampler defaultSampler);

    /**
     * @brief Shutdown and release all resources.
     */
    void shutdown();

    // ========================================================================
    // Texture Management
    // ========================================================================

    /**
     * @brief Create a texture from raw pixel data.
     */
    TextureHandle createTexture(const TextureCreateInfo& info);

    /**
     * @brief Register a pre-created texture (from async upload).
     */
    TextureHandle registerPreCreatedTexture(Texture tex, TextureView view, uint32_t w, uint32_t h, hina_format fmt, bool sRGB);

    /**
     * @brief Load a texture from file (PNG, JPG, etc).
     * @param path File path to the texture
     * @param sRGB Whether to treat as sRGB color space
     */
    TextureHandle loadTexture(const std::string& path, bool sRGB = true);

    /**
     * @brief Destroy a texture.
     */
    void destroyTexture(TextureHandle handle);

    /**
     * @brief Get texture data by handle.
     */
    const TextureEntry* getTexture(TextureHandle handle) const;

    /**
     * @brief Check if a texture handle is valid.
     */
    bool isTextureValid(TextureHandle handle) const;

    /**
     * @brief Get the texture view for a handle.
     * Returns invalid view if handle is invalid.
     */
    TextureView getTextureView(TextureHandle handle) const;

    // ========================================================================
    // Material Management
    // ========================================================================

    /**
     * @brief Create a material with default values.
     */
    MaterialHandle createMaterial();

    /**
     * @brief Create a material with specified properties.
     */
    MaterialHandle createMaterial(const GfxMaterial& material);

    /**
     * @brief Destroy a material.
     */
    void destroyMaterial(MaterialHandle handle);

    /**
     * @brief Get material data by handle.
     */
    const MaterialEntry* getMaterial(MaterialHandle handle) const;

    /**
     * @brief Update material properties (marks dirty for bind group update).
     */
    void updateMaterial(MaterialHandle handle, const GfxMaterial& material);

    /**
     * @brief Check if a material handle is valid.
     */
    bool isMaterialValid(MaterialHandle handle) const;

    /**
     * @brief Ensure all dirty materials have updated bind groups.
     * Call this before rendering.
     */
    void updateDirtyMaterials();

    /**
     * @brief Get the bind group for a material (for binding during draw).
     */
    BindGroup getMaterialBindGroup(MaterialHandle handle) const;

    // ========================================================================
    // Default Resources
    // ========================================================================

    TextureHandle getDefaultWhiteTexture() const { return m_defaultWhiteHandle; }
    TextureHandle getDefaultNormalTexture() const { return m_defaultNormalHandle; }
    MaterialHandle getDefaultMaterial() const { return m_defaultMaterialHandle; }

    // ========================================================================
    // Stats
    // ========================================================================

    uint32_t getTextureCount() const { return m_textureCount; }
    uint32_t getMaterialCount() const { return m_materialCount; }

private:
    // Handle allocation helpers
    TextureHandle allocateTextureEntry();
    void freeTextureEntry(uint16_t index);
    MaterialHandle allocateMaterialEntry();
    void freeMaterialEntry(uint16_t index);

    // Bind group creation
    void createMaterialBindGroup(MaterialEntry& entry);

    // Storage
    std::vector<TextureEntry> m_textures;
    std::vector<uint16_t> m_textureFreeList;
    uint32_t m_textureCount = 0;

    std::vector<MaterialEntry> m_materials;
    std::vector<uint16_t> m_materialFreeList;
    uint32_t m_materialCount = 0;

    // Shared resources
    BindGroupLayout m_materialLayout;
    Sampler m_defaultSampler;
    bool m_initialized = false;

    // Default resources
    TextureHandle m_defaultWhiteHandle;
    TextureHandle m_defaultNormalHandle;
    TextureHandle m_defaultBlackHandle;
    MaterialHandle m_defaultMaterialHandle;
};

} // namespace gfx
