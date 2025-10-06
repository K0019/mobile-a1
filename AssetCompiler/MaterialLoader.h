#pragma once
#include <vector>
#include <string>
#include <map>
#include "MeshCompilerData.h"

struct aiScene;
//struct aiTextureType;
struct aiMaterial;

namespace compiler
{
    // Copied same format as ryan to ensure compatability
    enum class AlphaMode : uint32_t
    {
        Opaque = 0,
        Mask = 1,
        Blend = 2
    };
    enum MaterialType : uint32_t
    {
        INVALID = 0,
        METALLIC_ROUGHNESS = 0x1,
        SPECULAR_GLOSSINESS = 0x2,
    };
    enum MaterialFlags : uint32_t
    {
        // Alpha mode (lower 2 bits)
        ALPHA_MODE_MASK = 0x3,   // 2 bits for masking
        ALPHA_MODE_OPAQUE = 0x0,   // 00
        ALPHA_MODE_MASK_TEST = 0x1,  // 01 (renamed to avoid conflict)
        ALPHA_MODE_BLEND = 0x2,   // 10

        // Material properties
        DOUBLE_SIDED = 0x4,   // Bit 2
        UNLIT = 0x8,   // Bit 3
        CAST_SHADOW = 0x10,  // Bit 4  
        RECEIVE_SHADOW = 0x20,  // Bit 5

        // Default flags
        DEFAULT_FLAGS = CAST_SHADOW | RECEIVE_SHADOW
    };

    struct ProcessedMaterialSlot
    {
        std::string name;
        uint32_t originalIndex;

        // Core PBR properties
        vec4 baseColorFactor = vec4(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        vec3 emissiveFactor = vec3(0.0f);
        float normalScale = 1.0f;
        float occlusionStrength = 1.0f;
        float alphaCutoff = 0.5f;

        // Material properties
        AlphaMode alphaMode = AlphaMode::Opaque;
        uint32_t materialTypeFlags = 0;
        uint32_t flags = 0;

        // Key: Texture type (e.g., "baseColor", "normal")
        // Value: The SOURCE filepath of the texture
        std::map<std::string, std::filesystem::path> texturePaths;
    };

    //ProcessedMaterialSlot extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath);

    // Collection utilities
    std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene);

    //void extractTexturePath(const aiMaterial* aiMat, aiTextureType type, const std::string& key, ProcessedMaterialSlot& outSlot, const std::filesystem::path& modelBasePath);
}


