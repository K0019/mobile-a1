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

    // Compiler-specific embedded texture source - stores actual pixel data for compression
    // (Different from engine's EmbeddedMemorySource which just stores identifiers)
    struct EmbeddedTextureSource
    {
        std::string name;
        // For compressed embedded data (jpg, png, etc.)
        const uint8_t* compressedData = nullptr;
        size_t compressedSize = 0;
        // For raw RGBA embedded data
        const uint8_t* rawData = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;

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
