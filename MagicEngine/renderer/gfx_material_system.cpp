/**
 * @file gfx_material_system.cpp
 * @brief Implementation of material and texture management for hina-vk.
 */

#include "gfx_material_system.h"
#include "logging/log.h"
#include <cstring>

// Enable stb_image for texture loading (needed for thumbnail PNGs)
#define GFX_USE_STB_IMAGE
#ifdef GFX_USE_STB_IMAGE
// Note: STB_IMAGE_IMPLEMENTATION is already defined in GraphicsWindow.cpp
#include <stb_image.h>
#endif

namespace gfx {

GfxMaterialSystem::GfxMaterialSystem() {
    m_textures.reserve(256);
    m_textureFreeList.reserve(64);
    m_materials.reserve(128);
    m_materialFreeList.reserve(32);
    LOG_INFO("[GfxMaterialSystem] Created");
}

GfxMaterialSystem::~GfxMaterialSystem() {
    if (m_initialized) {
        shutdown();
    }
    LOG_INFO("[GfxMaterialSystem] Destroyed");
}

bool GfxMaterialSystem::initialize(BindGroupLayout materialLayout, Sampler defaultSampler) {
    if (m_initialized) {
        LOG_WARNING("[GfxMaterialSystem] Already initialized");
        return true;
    }

    m_materialLayout = materialLayout;
    m_defaultSampler = defaultSampler;

    // Create default white texture (1x1)
    {
        static const uint8_t whitePixel[] = { 255, 255, 255, 255 };
        TextureCreateInfo info;
        info.data = whitePixel;
        info.width = 1;
        info.height = 1;
        info.format = HINA_FORMAT_R8G8B8A8_UNORM;
        info.isSRGB = true;
        info.label = "Default_White_1x1";
        m_defaultWhiteHandle = createTexture(info);
        if (!m_defaultWhiteHandle.isValid()) {
            LOG_ERROR("[GfxMaterialSystem] Failed to create default white texture");
            return false;
        }
    }

    // Create default normal texture (1x1 flat normal)
    {
        static const uint8_t normalPixel[] = { 128, 128, 255, 255 };
        TextureCreateInfo info;
        info.data = normalPixel;
        info.width = 1;
        info.height = 1;
        info.format = HINA_FORMAT_R8G8B8A8_UNORM;
        info.isSRGB = false;  // Normal maps are linear
        info.label = "Default_Normal_1x1";
        m_defaultNormalHandle = createTexture(info);
        if (!m_defaultNormalHandle.isValid()) {
            LOG_ERROR("[GfxMaterialSystem] Failed to create default normal texture");
            return false;
        }
    }

    // Create default black texture (1x1)
    {
        static const uint8_t blackPixel[] = { 0, 0, 0, 255 };
        TextureCreateInfo info;
        info.data = blackPixel;
        info.width = 1;
        info.height = 1;
        info.format = HINA_FORMAT_R8G8B8A8_UNORM;
        info.isSRGB = true;
        info.label = "Default_Black_1x1";
        m_defaultBlackHandle = createTexture(info);
        if (!m_defaultBlackHandle.isValid()) {
            LOG_ERROR("[GfxMaterialSystem] Failed to create default black texture");
            return false;
        }
    }

    // Create default material
    {
        GfxMaterial defaultMat;
        defaultMat.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        defaultMat.roughness = 0.5f;
        defaultMat.metallic = 0.0f;
        m_defaultMaterialHandle = createMaterial(defaultMat);
        if (!m_defaultMaterialHandle.isValid()) {
            LOG_ERROR("[GfxMaterialSystem] Failed to create default material");
            return false;
        }
    }

    m_initialized = true;
    LOG_INFO("[GfxMaterialSystem] Initialized with default resources");
    return true;
}

void GfxMaterialSystem::shutdown() {
    if (!m_initialized) return;

    // Destroy all materials
    for (auto& entry : m_materials) {
        if (entry.inUse && hina_bind_group_is_valid(entry.bindGroup)) {
            hina_destroy_bind_group(entry.bindGroup);
        }
    }
    m_materials.clear();
    m_materialFreeList.clear();
    m_materialCount = 0;

    // Destroy all textures
    for (auto& entry : m_textures) {
        if (entry.inUse) {
            if (hina_texture_is_valid(entry.texture)) {
                hina_destroy_texture(entry.texture);
            }
        }
    }
    m_textures.clear();
    m_textureFreeList.clear();
    m_textureCount = 0;

    // Invalidate handles
    m_defaultWhiteHandle = TextureHandle{};
    m_defaultNormalHandle = TextureHandle{};
    m_defaultBlackHandle = TextureHandle{};
    m_defaultMaterialHandle = MaterialHandle{};

    m_initialized = false;
    LOG_INFO("[GfxMaterialSystem] Shutdown complete");
}

// ============================================================================
// Texture Management
// ============================================================================

TextureHandle GfxMaterialSystem::allocateTextureEntry() {
    TextureHandle handle;

    if (!m_textureFreeList.empty()) {
        handle.index = m_textureFreeList.back();
        m_textureFreeList.pop_back();
        handle.generation = m_textures[handle.index].generation;
    } else {
        if (m_textures.size() >= 65535) {
            LOG_ERROR("[GfxMaterialSystem] Maximum texture count exceeded");
            return TextureHandle{};
        }
        handle.index = static_cast<uint16_t>(m_textures.size());
        handle.generation = 1;
        m_textures.push_back({});
        m_textures.back().generation = 1;
    }

    m_textures[handle.index].inUse = true;
    m_textureCount++;
    return handle;
}

void GfxMaterialSystem::freeTextureEntry(uint16_t index) {
    if (index >= m_textures.size()) return;

    TextureEntry& entry = m_textures[index];
    if (!entry.inUse) return;

    if (hina_texture_is_valid(entry.texture)) {
        // Flush all pending staged uploads and wait for completion before destroying
        // The staging command buffer is shared across all textures - destroying
        // any texture with pending commands invalidates the entire batch
        hina_ticket ticket = hina_flush_uploads();
        if (ticket) hina_wait_ticket(ticket);
        hina_destroy_texture(entry.texture);
    }

    entry.texture = {};
    entry.view = {};
    entry.inUse = false;
    entry.generation++;

    m_textureFreeList.push_back(index);
    m_textureCount--;
}

// Helper to check if format is mobile-only (ETC2/EAC/ASTC - not supported on desktop GPUs)
static bool isMobileOnlyFormat(hina_format fmt) {
    switch (fmt) {
        // ETC2 formats
        case HINA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case HINA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        case HINA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case HINA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        case HINA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case HINA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        // EAC formats
        case HINA_FORMAT_EAC_R11_UNORM_BLOCK:
        case HINA_FORMAT_EAC_R11_SNORM_BLOCK:
        case HINA_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case HINA_FORMAT_EAC_R11G11_SNORM_BLOCK:
        // ASTC formats
        case HINA_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case HINA_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case HINA_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return true;
        default:
            return false;
    }
}

TextureHandle GfxMaterialSystem::createTexture(const TextureCreateInfo& info) {
    if (!info.data || info.width == 0 || info.height == 0) {
        LOG_ERROR("[GfxMaterialSystem] Invalid texture create info: data={}, width={}, height={}",
            info.data ? "valid" : "null", info.width, info.height);
        return TextureHandle{};
    }

    // Check for mobile-only formats on desktop - these will crash the driver
    // Note: __linux__ is also defined on Android, so we explicitly exclude __ANDROID__
#if (defined(_WIN32) || defined(__linux__)) && !defined(__ANDROID__)
    if (isMobileOnlyFormat(info.format)) {
        LOG_WARNING("[GfxMaterialSystem] Mobile-only format {} not supported on desktop, using fallback",
            static_cast<int>(info.format));
        // Return invalid handle - caller should use default texture
        return TextureHandle{};
    }
#endif

    TextureHandle handle = allocateTextureEntry();
    if (!handle.isValid()) {
        return handle;
    }

    TextureEntry& entry = m_textures[handle.index];

    hina_texture_desc desc = hina_texture_desc_default();
    desc.width = info.width;
    desc.height = info.height;
    desc.format = info.format;
    desc.usage = HINA_TEXTURE_SAMPLED_BIT;
    desc.initial_data = info.data;
    desc.label = info.label;  // Debug label for RenderDoc

    // Enable automatic mip generation if requested
    // NOTE: Only works for uncompressed formats. KTX2/compressed textures have pre-baked mips.
    // TODO: Implement engine-side or hina-vk mip generation for runtime-loaded uncompressed textures
    if (info.generateMips) {
        desc.mip_levels = HINA_MIP_LEVELS_AUTO;
    }

    entry.texture = hina_make_texture(&desc);
    if (!hina_texture_is_valid(entry.texture)) {
        LOG_ERROR("[GfxMaterialSystem] Failed to create texture");
        freeTextureEntry(handle.index);
        return TextureHandle{};
    }

    entry.view = hina_texture_get_default_view(entry.texture);
    entry.width = info.width;
    entry.height = info.height;
    entry.format = info.format;
    entry.isSRGB = info.isSRGB;

    LOG_DEBUG("[GfxMaterialSystem] Created texture {}x{}", info.width, info.height);
    return handle;
}

TextureHandle GfxMaterialSystem::registerPreCreatedTexture(Texture tex, TextureView view, uint32_t w, uint32_t h, hina_format fmt, bool sRGB) {
    if (!hina_texture_is_valid(tex)) {
        LOG_ERROR("[GfxMaterialSystem] registerPreCreatedTexture: invalid texture");
        return TextureHandle{};
    }

    TextureHandle handle = allocateTextureEntry();
    if (!handle.isValid()) return handle;

    TextureEntry& entry = m_textures[handle.index];
    entry.texture = tex;
    entry.view = view;
    entry.width = w;
    entry.height = h;
    entry.format = fmt;
    entry.isSRGB = sRGB;

    LOG_DEBUG("[GfxMaterialSystem] Registered pre-created texture {}x{}", w, h);
    return handle;
}

TextureHandle GfxMaterialSystem::loadTexture(const std::string& path, bool sRGB) {
#ifdef GFX_USE_STB_IMAGE
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        LOG_ERROR("[GfxMaterialSystem] Failed to load texture: {}", path);
        return TextureHandle{};
    }

    TextureCreateInfo info;
    info.data = data;
    info.width = static_cast<uint32_t>(width);
    info.height = static_cast<uint32_t>(height);
    info.format = HINA_FORMAT_R8G8B8A8_UNORM;
    info.isSRGB = sRGB;

    TextureHandle handle = createTexture(info);
    stbi_image_free(data);

    if (handle.isValid()) {
        LOG_DEBUG("[GfxMaterialSystem] Loaded texture: {}", path);
    }
    return handle;
#else
    LOG_WARNING("[GfxMaterialSystem] stb_image not enabled, cannot load: {}", path);
    return TextureHandle{};
#endif
}

void GfxMaterialSystem::destroyTexture(TextureHandle handle) {
    if (!isTextureValid(handle)) return;
    freeTextureEntry(handle.index);
    LOG_DEBUG("[GfxMaterialSystem] Destroyed texture {}:{}", handle.index, handle.generation);
}

const TextureEntry* GfxMaterialSystem::getTexture(TextureHandle handle) const {
    if (!isTextureValid(handle)) return nullptr;
    return &m_textures[handle.index];
}

bool GfxMaterialSystem::isTextureValid(TextureHandle handle) const {
    if (handle.index >= m_textures.size()) return false;
    const TextureEntry& entry = m_textures[handle.index];
    return entry.inUse && entry.generation == handle.generation;
}

TextureView GfxMaterialSystem::getTextureView(TextureHandle handle) const {
    if (!isTextureValid(handle)) {
        return {};
    }
    const TextureEntry& entry = m_textures[handle.index];
    // Always wait for texture upload to complete before returning the view
    // This prevents using textures that are still in UNDEFINED layout
    if (hina_texture_is_valid(entry.texture)) {
        hina_wait_texture(entry.texture);
    }
    return entry.view;
}

// ============================================================================
// Material Management
// ============================================================================

MaterialHandle GfxMaterialSystem::allocateMaterialEntry() {
    MaterialHandle handle;

    if (!m_materialFreeList.empty()) {
        handle.index = m_materialFreeList.back();
        m_materialFreeList.pop_back();
        handle.generation = m_materials[handle.index].generation;
    } else {
        if (m_materials.size() >= 65535) {
            LOG_ERROR("[GfxMaterialSystem] Maximum material count exceeded");
            return MaterialHandle{};
        }
        handle.index = static_cast<uint16_t>(m_materials.size());
        handle.generation = 1;
        m_materials.push_back({});
        m_materials.back().generation = 1;
    }

    m_materials[handle.index].inUse = true;
    m_materialCount++;
    return handle;
}

void GfxMaterialSystem::freeMaterialEntry(uint16_t index) {
    if (index >= m_materials.size()) return;

    MaterialEntry& entry = m_materials[index];
    if (!entry.inUse) return;

    if (hina_bind_group_is_valid(entry.bindGroup)) {
        hina_destroy_bind_group(entry.bindGroup);
    }

    // Destroy material constants UBO
    if (hina_buffer_is_valid(entry.constantsUBO)) {
        hina_destroy_buffer(entry.constantsUBO);
    }

    uint16_t nextGen = entry.generation + 1;
    entry = {};
    entry.generation = nextGen;

    m_materialFreeList.push_back(index);
    m_materialCount--;
}

MaterialHandle GfxMaterialSystem::createMaterial() {
    return createMaterial(GfxMaterial{});
}

MaterialHandle GfxMaterialSystem::createMaterial(const GfxMaterial& material) {
    MaterialHandle handle = allocateMaterialEntry();
    if (!handle.isValid()) {
        return handle;
    }

    MaterialEntry& entry = m_materials[handle.index];
    entry.material = material;
    entry.dirty = true;

    // Pack GPU data with all material properties
    uint32_t flags = 0;
    if (material.unlit) flags |= MAT_FLAG_UNLIT;
    if (material.doubleSided) flags |= MAT_FLAG_DOUBLE_SIDED;

    entry.gpuData.pack(
        material.baseColor,
        material.roughness,
        material.metallic,
        material.emissive,
        material.alphaCutoff,
        material.normalScale,
        material.occlusionStrength,
        material.emissiveColor,
        flags
    );

    // Create bind group if we have a valid layout
    if (hina_bind_group_layout_is_valid(m_materialLayout)) {
        createMaterialBindGroup(entry);
    }

    LOG_DEBUG("[GfxMaterialSystem] Created material {}:{}", handle.index, handle.generation);
    return handle;
}

void GfxMaterialSystem::destroyMaterial(MaterialHandle handle) {
    if (!isMaterialValid(handle)) return;
    freeMaterialEntry(handle.index);
    LOG_DEBUG("[GfxMaterialSystem] Destroyed material {}:{}", handle.index, handle.generation);
}

const MaterialEntry* GfxMaterialSystem::getMaterial(MaterialHandle handle) const {
    if (!isMaterialValid(handle)) return nullptr;
    return &m_materials[handle.index];
}

void GfxMaterialSystem::updateMaterial(MaterialHandle handle, const GfxMaterial& material) {
    if (!isMaterialValid(handle)) return;

    MaterialEntry& entry = m_materials[handle.index];
    entry.material = material;

    // Pack GPU data with all material properties
    uint32_t flags = 0;
    if (material.unlit) flags |= MAT_FLAG_UNLIT;
    if (material.doubleSided) flags |= MAT_FLAG_DOUBLE_SIDED;

    entry.gpuData.pack(
        material.baseColor,
        material.roughness,
        material.metallic,
        material.emissive,
        material.alphaCutoff,
        material.normalScale,
        material.occlusionStrength,
        material.emissiveColor,
        flags
    );

    // Immediately rebuild bind group since texture references may have changed
    if (hina_bind_group_layout_is_valid(m_materialLayout)) {
        createMaterialBindGroup(entry);
    }
}

bool GfxMaterialSystem::isMaterialValid(MaterialHandle handle) const {
    if (handle.index >= m_materials.size()) return false;
    const MaterialEntry& entry = m_materials[handle.index];
    return entry.inUse && entry.generation == handle.generation;
}

void GfxMaterialSystem::createMaterialBindGroup(MaterialEntry& entry) {
    // Destroy old bind group if exists
    if (hina_bind_group_is_valid(entry.bindGroup)) {
        hina_destroy_bind_group(entry.bindGroup);
    }

    // Create or update material constants UBO (32 bytes)
    if (!hina_buffer_is_valid(entry.constantsUBO)) {
        hina_buffer_desc ubo_desc = {};
        ubo_desc.size = sizeof(PackedMaterial);  // 32 bytes
        ubo_desc.memory = HINA_BUFFER_CPU;
        ubo_desc.usage = HINA_BUFFER_UNIFORM;
        entry.constantsUBO = hina_make_buffer(&ubo_desc);
        if (!hina_buffer_is_valid(entry.constantsUBO)) {
            LOG_ERROR("[GfxMaterialSystem] Failed to create material constants UBO");
            return;
        }
    }

    // Upload packed material data to UBO
    void* mapped = hina_mapped_buffer_ptr(entry.constantsUBO);
    if (mapped) {
        std::memcpy(mapped, &entry.gpuData, sizeof(PackedMaterial));
        // No unmap needed for HOST_COHERENT persistently mapped buffers
    }

    // Get texture views (use defaults if not set)
    // Must wait for upload completion to avoid sampling from UNDEFINED layout
    auto getView = [this](TextureHandle h, TextureHandle def) -> TextureView {
        if (isTextureValid(h)) {
            const TextureEntry& entry = m_textures[h.index];
            if (hina_texture_is_valid(entry.texture)) {
                hina_wait_texture(entry.texture);
            }
            return entry.view;
        }
        if (isTextureValid(def)) {
            const TextureEntry& defEntry = m_textures[def.index];
            if (hina_texture_is_valid(defEntry.texture)) {
                hina_wait_texture(defEntry.texture);
            }
            return defEntry.view;
        }
        return {};
    };

    TextureView albedoView = getView(entry.material.albedoTexture, m_defaultWhiteHandle);
    TextureView normalView = getView(entry.material.normalTexture, m_defaultNormalHandle);
    TextureView mrView = getView(entry.material.metallicRoughnessTexture, m_defaultWhiteHandle);
    TextureView emissiveView = getView(entry.material.emissiveTexture, m_defaultBlackHandle);
    TextureView occlusionView = getView(entry.material.occlusionTexture, m_defaultWhiteHandle);

    // Create bind group entries
    // Binding 0: Material constants UBO (32 bytes packed material)
    // Bindings 1-5: Textures
    hina_bind_group_entry entries[6] = {};

    // Binding 0: Material constants UBO
    entries[0].binding = 0;
    entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
    entries[0].buffer.buffer = entry.constantsUBO;
    entries[0].buffer.offset = 0;
    entries[0].buffer.size = sizeof(PackedMaterial);

    entries[1].binding = 1;  // Albedo
    entries[1].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entries[1].combined.view = albedoView;
    entries[1].combined.sampler = m_defaultSampler;

    entries[2].binding = 2;  // Normal
    entries[2].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entries[2].combined.view = normalView;
    entries[2].combined.sampler = m_defaultSampler;

    entries[3].binding = 3;  // MetallicRoughness
    entries[3].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entries[3].combined.view = mrView;
    entries[3].combined.sampler = m_defaultSampler;

    entries[4].binding = 4;  // Emissive
    entries[4].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entries[4].combined.view = emissiveView;
    entries[4].combined.sampler = m_defaultSampler;

    entries[5].binding = 5;  // Occlusion
    entries[5].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entries[5].combined.view = occlusionView;
    entries[5].combined.sampler = m_defaultSampler;

    hina_bind_group_desc desc = {};
    desc.layout = m_materialLayout;
    desc.entries = entries;
    desc.entry_count = 6;

    entry.bindGroup = hina_create_bind_group(&desc);
    if (!hina_bind_group_is_valid(entry.bindGroup)) {
        LOG_WARNING("[GfxMaterialSystem] Failed to create material bind group");
    }

    entry.dirty = false;
}

void GfxMaterialSystem::updateDirtyMaterials() {
    for (auto& entry : m_materials) {
        if (entry.inUse && entry.dirty) {
            createMaterialBindGroup(entry);
        }
    }
}

BindGroup GfxMaterialSystem::getMaterialBindGroup(MaterialHandle handle) const {
    if (!isMaterialValid(handle)) {
        // Return default material bind group
        if (isMaterialValid(m_defaultMaterialHandle)) {
            return m_materials[m_defaultMaterialHandle.index].bindGroup;
        }
        return {};
    }
    return m_materials[handle.index].bindGroup;
}

} // namespace gfx
