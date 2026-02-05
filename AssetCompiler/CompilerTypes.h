/******************************************************************************/
/*!
\file   CompilerTypes.h
\brief  Asset compiler types - now re-exports from MagicEngine
*/
/******************************************************************************/

#pragma once

#include <map>

// Include engine types
#include "CompilerMath.h"
#include "resource/processed_assets.h"
#include "resource/resource_types.h"

namespace compiler
{
    // ===== Texture keys (compiler-specific) =====
    namespace texturekeys
    {
        constexpr const char* BASE_COLOR = "baseColor";
        constexpr const char* METALLIC_ROUGHNESS = "metallicRoughness";
        constexpr const char* NORMAL = "normal";
        constexpr const char* EMISSIVE = "emissive";
        constexpr const char* OCCLUSION = "occlusion";

        constexpr const char* ALL[] = {
            BASE_COLOR,
            METALLIC_ROUGHNESS,
            NORMAL,
            EMISSIVE,
            OCCLUSION
        };
    }

    // ===== Re-export engine types =====

    // Skinning and morphing
    using SkinningData = Resource::SkinningData;
    using MorphTargetVertexDelta = Resource::MorphTargetVertexDelta;
    using MorphTargetData = Resource::MorphTargetData;

    // Compiler's bone structure (for extracting from Assimp)
    struct ProcessedBone
    {
        std::string name;
        int32_t parentIndex = -1;
        mat4 inverseBindPose = mat4(1.0f);
        mat4 bindPose = mat4(1.0f);
    };

    // Compiler's skeleton structure (stores bones with names and lookup map)
    struct ProcessedSkeleton
    {
        std::vector<ProcessedBone> bones;
        std::map<std::string, uint32_t> boneNameToIndex;
        bool empty() const { return bones.empty(); }
    };

    // Animation types - compiler uses slightly different naming conventions
    struct ProcessedBoneChannel
    {
        std::string nodeName;
        std::vector<PositionKey> positionKeys;  // Compiler calls these positionKeys (engine: translationKeys)
        std::vector<RotationKey> rotationKeys;
        std::vector<ScaleKey> scaleKeys;
    };
    using ProcessedMorphKey = Resource::ProcessedMorphKey;
    using ProcessedMorphChannel = Resource::ProcessedMorphChannel;

    struct ProcessedAnimation
    {
        std::string name;
        float duration = 0.0f;
        float ticksPerSecond = 0.0f;
        std::vector<ProcessedBoneChannel> skeletalChannels;
        std::vector<ProcessedMorphChannel> morphChannels;
    };

    // Mesh
    using ProcessedMesh = Resource::ProcessedMesh;

    // Material types (from global scope in resource_types.h)
    using AlphaMode = ::AlphaMode;
    using MaterialType = ::MaterialType;
    using MaterialFlags = ::MaterialFlags;
    using FilePathSource = ::FilePathSource;

    // Compiler-specific embedded texture source - owns actual pixel data for compression
    // (Different from engine's EmbeddedMemorySource which just stores identifiers)
    // IMPORTANT: This struct owns its data - copies are made when loading from FBX
    // to avoid dangling pointers when the FBX scene is freed.
    struct EmbeddedTextureSource
    {
        std::string name;
        // For compressed embedded data (jpg, png, etc.) - owned by this struct
        std::vector<uint8_t> compressedData;
        // For raw RGBA embedded data - owned by this struct
        std::vector<uint8_t> rawData;
        uint32_t width = 0;
        uint32_t height = 0;

        // Accessors for backward compatibility with code expecting raw pointers
        const uint8_t* getCompressedData() const { return compressedData.empty() ? nullptr : compressedData.data(); }
        size_t getCompressedSize() const { return compressedData.size(); }
        const uint8_t* getRawData() const { return rawData.empty() ? nullptr : rawData.data(); }

        bool operator<(const EmbeddedTextureSource& other) const { return name < other.name; }
        bool operator==(const EmbeddedTextureSource& other) const { return name == other.name; }
    };

    // Compiler's own TextureDataSource variant using compiler-specific types
    using TextureDataSource = std::variant<std::monostate, FilePathSource, EmbeddedTextureSource>;

    // Constants
    static constexpr uint32_t INVALID_BONE_INDEX = Resource::INVALID_BONE_INDEX;
    static constexpr uint32_t INVALID_JOINT_INDEX = INVALID_BONE_INDEX;
    static constexpr int MAX_BONES_PER_VERTEX = 4;

    // ===== Compiler-specific types (not in engine) =====

    // Material slot with source texture paths (compiler writes .material files)
    struct ProcessedMaterialSlot
    {
        std::string name;
        uint32_t originalIndex = 0;

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
        std::map<std::string, TextureDataSource> texturePaths;
    };

    // ===== Material Deduplication =====

    // Hash texture source by content, not just name
    inline size_t hashTextureSource(const TextureDataSource& source)
    {
        if (std::holds_alternative<std::monostate>(source))
            return 0;

        if (std::holds_alternative<FilePathSource>(source))
        {
            // Hash the file path
            return std::hash<std::string>{}(std::get<FilePathSource>(source).path);
        }

        // EmbeddedTextureSource - hash actual pixel content, not name
        const auto& embedded = std::get<EmbeddedTextureSource>(source);
        const uint8_t* data = embedded.getCompressedData() ? embedded.getCompressedData() : embedded.getRawData();
        size_t size = embedded.getCompressedData() ? embedded.getCompressedSize()
                      : (static_cast<size_t>(embedded.width) * embedded.height * 4);

        if (!data || size == 0) return 0;

        // FNV-1a hash of full content
        size_t hash = 14695981039346656037ULL;
        for (size_t i = 0; i < size; ++i)
        {
            hash ^= data[i];
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    // Content-based material signature for deduplication
    struct MaterialSignature
    {
        // PBR factors (quantized to avoid floating point comparison issues)
        int32_t baseColorR, baseColorG, baseColorB, baseColorA;  // *1000
        int32_t metallicFactor;   // *1000
        int32_t roughnessFactor;  // *1000
        int32_t emissiveR, emissiveG, emissiveB;  // *1000
        int32_t normalScale;      // *1000
        int32_t occlusionStrength; // *1000
        int32_t alphaCutoff;      // *1000

        AlphaMode alphaMode;
        uint32_t materialTypeFlags;
        uint32_t flags;

        // Texture content hashes (0 if no texture)
        size_t baseColorTexHash;
        size_t metallicRoughnessTexHash;
        size_t normalTexHash;
        size_t emissiveTexHash;
        size_t occlusionTexHash;

        bool operator==(const MaterialSignature& other) const
        {
            return baseColorR == other.baseColorR && baseColorG == other.baseColorG &&
                   baseColorB == other.baseColorB && baseColorA == other.baseColorA &&
                   metallicFactor == other.metallicFactor && roughnessFactor == other.roughnessFactor &&
                   emissiveR == other.emissiveR && emissiveG == other.emissiveG && emissiveB == other.emissiveB &&
                   normalScale == other.normalScale && occlusionStrength == other.occlusionStrength &&
                   alphaCutoff == other.alphaCutoff && alphaMode == other.alphaMode &&
                   materialTypeFlags == other.materialTypeFlags && flags == other.flags &&
                   baseColorTexHash == other.baseColorTexHash &&
                   metallicRoughnessTexHash == other.metallicRoughnessTexHash &&
                   normalTexHash == other.normalTexHash &&
                   emissiveTexHash == other.emissiveTexHash &&
                   occlusionTexHash == other.occlusionTexHash;
        }

        size_t hash() const
        {
            // Combine all fields into a single hash
            size_t h = 14695981039346656037ULL;
            auto combine = [&h](size_t val) {
                h ^= val;
                h *= 1099511628211ULL;
            };

            combine(static_cast<size_t>(baseColorR));
            combine(static_cast<size_t>(baseColorG));
            combine(static_cast<size_t>(baseColorB));
            combine(static_cast<size_t>(baseColorA));
            combine(static_cast<size_t>(metallicFactor));
            combine(static_cast<size_t>(roughnessFactor));
            combine(static_cast<size_t>(emissiveR));
            combine(static_cast<size_t>(emissiveG));
            combine(static_cast<size_t>(emissiveB));
            combine(static_cast<size_t>(normalScale));
            combine(static_cast<size_t>(occlusionStrength));
            combine(static_cast<size_t>(alphaCutoff));
            combine(static_cast<size_t>(alphaMode));
            combine(static_cast<size_t>(materialTypeFlags));
            combine(static_cast<size_t>(flags));
            combine(baseColorTexHash);
            combine(metallicRoughnessTexHash);
            combine(normalTexHash);
            combine(emissiveTexHash);
            combine(occlusionTexHash);

            return h;
        }

        static MaterialSignature fromMaterial(const ProcessedMaterialSlot& mat)
        {
            MaterialSignature sig{};

            // Quantize floats to avoid floating point comparison issues
            sig.baseColorR = static_cast<int32_t>(mat.baseColorFactor.x * 1000.0f);
            sig.baseColorG = static_cast<int32_t>(mat.baseColorFactor.y * 1000.0f);
            sig.baseColorB = static_cast<int32_t>(mat.baseColorFactor.z * 1000.0f);
            sig.baseColorA = static_cast<int32_t>(mat.baseColorFactor.w * 1000.0f);
            sig.metallicFactor = static_cast<int32_t>(mat.metallicFactor * 1000.0f);
            sig.roughnessFactor = static_cast<int32_t>(mat.roughnessFactor * 1000.0f);
            sig.emissiveR = static_cast<int32_t>(mat.emissiveFactor.x * 1000.0f);
            sig.emissiveG = static_cast<int32_t>(mat.emissiveFactor.y * 1000.0f);
            sig.emissiveB = static_cast<int32_t>(mat.emissiveFactor.z * 1000.0f);
            sig.normalScale = static_cast<int32_t>(mat.normalScale * 1000.0f);
            sig.occlusionStrength = static_cast<int32_t>(mat.occlusionStrength * 1000.0f);
            sig.alphaCutoff = static_cast<int32_t>(mat.alphaCutoff * 1000.0f);

            sig.alphaMode = mat.alphaMode;
            sig.materialTypeFlags = mat.materialTypeFlags;
            sig.flags = mat.flags;

            // Hash textures by content
            auto getTexHash = [&mat](const char* key) -> size_t {
                auto it = mat.texturePaths.find(key);
                if (it == mat.texturePaths.end()) return 0;
                return hashTextureSource(it->second);
            };

            sig.baseColorTexHash = getTexHash(texturekeys::BASE_COLOR);
            sig.metallicRoughnessTexHash = getTexHash(texturekeys::METALLIC_ROUGHNESS);
            sig.normalTexHash = getTexHash(texturekeys::NORMAL);
            sig.emissiveTexHash = getTexHash(texturekeys::EMISSIVE);
            sig.occlusionTexHash = getTexHash(texturekeys::OCCLUSION);

            return sig;
        }
    };

    // Scene graph node
    struct SceneNode
    {
        std::string name;
        mat4 localTransform = mat4(1.0f);
        int32_t meshIndex = -1;
        int32_t parentIndex = -1;
    };

    // Complete scene structure for compiler
    struct Scene
    {
        std::string name;

        std::vector<SceneNode> nodes;
        std::vector<ProcessedMesh> meshes;
        std::vector<ProcessedMaterialSlot> materials;

        // Skeleton extracted from scene
        ProcessedSkeleton skeleton;

        std::vector<ProcessedAnimation> animations;

        // Scene bounds
        vec3 center{ 0 };
        float radius = 0;
        vec3 boundingMin{ FLT_MAX };
        vec3 boundingMax{ -FLT_MAX };

        // Statistics
        uint32_t totalMeshes = 0;
        uint32_t totalMaterials = 0;
        uint32_t totalTextures = 0;
    };

    struct SceneLoadData
    {
        std::optional<Scene> scene;
        std::vector<std::string> errors;
    };

    // ===== ID types (compiler-specific) =====
    using ClipId = uint32_t;
    using SkeletonId = uint32_t;
    using MorphSetId = uint32_t;

    inline constexpr ClipId INVALID_CLIP_ID = 0;
    inline constexpr SkeletonId INVALID_SKELETON_ID = 0;
    inline constexpr MorphSetId INVALID_MORPH_SET_ID = 0;
}
