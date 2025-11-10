#pragma once
#include "CompilerMath.h"
#include <map>
#include <array>

namespace compiler
{
    using ClipId = uint32_t;
    using SkeletonId = uint32_t;
    using MorphSetId = uint32_t;

    inline constexpr ClipId INVALID_CLIP_ID = 0;
    inline constexpr SkeletonId INVALID_SKELETON_ID = 0;
    inline constexpr MorphSetId INVALID_MORPH_SET_ID = 0;

    static constexpr uint32_t INVALID_BONE_INDEX = static_cast<uint32_t>(-1);
    static constexpr uint32_t INVALID_JOINT_INDEX = INVALID_BONE_INDEX;
    static constexpr int MAX_BONES_PER_VERTEX = 4;


    // ----- Animations, bones, skeletons, friends ----- //
    // Skeletal animation
    struct SkinningData
    {
        std::array<uint32_t, MAX_BONES_PER_VERTEX> boneIndices{ INVALID_BONE_INDEX, INVALID_BONE_INDEX, INVALID_BONE_INDEX, INVALID_BONE_INDEX };
        std::array<float, MAX_BONES_PER_VERTEX> weights{ 0.0f, 0.0f, 0.0f, 0.0f };
    };
    struct ProcessedBoneChannel
    {
        uint32_t boneIndex; // The index into ProcessedSkeleton::bones
        std::vector<PositionKey> positionKeys;
        std::vector<RotationKey> rotationKeys;
        std::vector<ScaleKey>    scaleKeys;
    };
    struct ProcessedBone
    {
        std::string name;
        int32_t parentIndex = -1;
        mat4 inverseBindPose = mat4(1.0f);
        mat4 bindPose;
    };
    struct ProcessedSkeleton
    {
        std::vector<ProcessedBone> bones;
        std::map<std::string, uint32_t> boneNameToIndex;
    };

    // Morph animation
    struct MorphTargetVertexDelta
    {
        uint32_t vertexIndex;
        vec3     deltaPosition{ 0.0f };
        vec3     deltaNormal{ 0.0f };
        vec3     deltaTangent{ 0.0f };
    };
    struct MorphTargetData
    {
        std::string name;
        std::vector<MorphTargetVertexDelta> deltas;
    };
    struct ProcessedMorphKey
    {
        float time = 0.0f;
        std::vector<uint32_t> targetIndices;
        std::vector<float> weights;
    };
    struct ProcessedMorphChannel
    {
        uint32_t meshIndex = UINT32_MAX;
        std::string meshName;
        std::vector<ProcessedMorphKey> keys;
    };

    struct ProcessedAnimation
    {
        std::string name;
        float duration;
        float ticksPerSecond;
        std::vector<ProcessedBoneChannel> skeletalChannels;
        std::vector<ProcessedMorphChannel> morphChannels;
    };


    // ----- Mesh -----
    struct ProcessedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;
        uint32_t materialIndex = UINT32_MAX;
        vec4 bounds{ 0, 0, 0, 1 }; // x,y,z = center, w = radius

        //Animations
        std::vector<SkinningData> skinning;
        ProcessedSkeleton skeleton;
        std::vector<MorphTargetData> morphTargets;
    };


    // ----- Materials specific -----
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


    // ----- Scene -----
    struct SceneNode
    {
        std::string name;
        mat4 localTransform;
        int32_t meshIndex = -1;
        int32_t parentIndex = -1;
    };
    struct Scene
    {
        std::string name;

        std::vector<SceneNode> nodes;
        std::vector<ProcessedMesh> meshes; // Objects contain direct handles
        std::vector<ProcessedMaterialSlot> materials; // Objects contain direct handles

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

}