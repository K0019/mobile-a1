// resource/asset_formats/mesh_format.h
#pragma once
#include <cstdint>
#include "math/utils_math.h"

namespace Resource {

// Magic number for mesh files
constexpr uint32_t MESH_FILE_MAGIC = 'MESH';  // 0x4853454D

#pragma pack(push, 1)

/**
 * @brief Header at the start of .mesh files
 *
 * Binary layout:
 * [MeshFileHeader]
 * [Node Data Array]
 * [MeshInfo Array]
 * [Mesh Names Buffer]
 * [Material Names Buffer]
 * [Index Data Array]
 * [Vertex Data Array]
 * [Skinning Data Array]     (if hasSkeleton)
 * [Bone Data Array]          (if hasSkeleton)
 * [Bone Names Buffer]        (if hasSkeleton)
 * [Morph Target Array]       (if hasMorphs)
 * [Morph Delta Array]        (if hasMorphs)
 * [Morph Target Names Buffer](if hasMorphs)
 */
struct MeshFileHeader
{
    uint32_t magic = MESH_FILE_MAGIC;

    // Counts
    uint32_t numNodes;
    uint32_t numMeshes;
    uint32_t totalIndices;
    uint32_t totalVertices;
    uint32_t meshNameBufferSize;
    uint32_t materialNameBufferSize;

    // Skeleton
    uint32_t hasSkeleton;  // 1 or 0
    uint32_t numBones;
    uint32_t boneNameBufferSize;

    // Morphs
    uint32_t hasMorphs;  // 1 or 0
    uint32_t numMorphTargets;
    uint32_t numMorphDeltas;
    uint32_t morphTargetNameBufferSize;

    // Scene bounds (mostly unused)
    vec3 sceneBoundsCenter;
    float sceneBoundsRadius;
    vec3 sceneBoundsMin;
    vec3 sceneBoundsMax;

    // Data block offsets (from start of file)
    uint64_t nodeDataOffset;
    uint64_t meshInfoDataOffset;
    uint64_t meshNamesOffset;
    uint64_t materialNamesOffset;
    uint64_t indexDataOffset;
    uint64_t vertexDataOffset;

    uint64_t skinningDataOffset;
    uint64_t boneDataOffset;
    uint64_t boneNameOffset;

    uint64_t morphTargetDataOffset;
    uint64_t morphDeltaDataOffset;
    uint64_t morphTargetNameOffset;
};

/**
 * @brief Scene graph node (transform hierarchy)
 */
struct MeshNode
{
    mat4 transform;         // Local transform
    int32_t parentIndex;    // Index into node array, -1 for root
    int32_t meshIndex;      // Index into mesh info array, -1 if no mesh
    char name[64];          // Fixed-size name
};

/**
 * @brief Describes how to extract mesh data from buffers
 */
struct MeshInfo
{
    uint32_t indexCount;
    uint32_t firstIndex;        // Offset into global index buffer
    uint32_t firstVertex;       // Offset into global vertex buffer
    uint32_t nameOffset;        // Into mesh names buffer
    uint32_t materialNameIndex; // Into material names buffer

    vec4 meshBounds;  // (x,y,z, radius) for frustum culling

    uint32_t firstMorphTarget;  // Index into morph target array
    uint32_t morphTargetCount;
};

/**
 * @brief Vertex format in .mesh files (48 bytes)
 * Named MeshFile_Vertex to avoid conflict with ::Vertex in gpu_data.h
 */
struct MeshFile_Vertex
{
    vec3 position;   // 12 bytes
    float uv_x;      // 4 bytes
    vec3 normal;     // 12 bytes
    float uv_y;      // 4 bytes
    vec4 tangent;    // 16 bytes (xyz = tangent, w = handedness)
};

/**
 * @brief Bone data for skeletal animation
 */
struct MeshFile_Bone
{
    mat4 inverseBindPose;  // Assimp's aiBone::mOffsetMatrix
    mat4 bindPose;         // Bone's global transform in bind pose
    int32_t parentIndex;   // Index into bone array, -1 for root
    uint32_t nameOffset;   // Into bone names buffer
};

/**
 * @brief Per-vertex morph target delta
 */
struct MeshFile_MorphDelta
{
    uint32_t vertexIndex;  // Global vertex index
    vec3 deltaPosition;
    vec3 deltaNormal;
    vec3 deltaTangent;
};

/**
 * @brief Morph target definition
 */
struct MeshFile_MorphTarget
{
    uint32_t nameOffset;  // Into morph target names buffer
    uint32_t firstDelta;  // Index into morph delta array
    uint32_t deltaCount;
};

#pragma pack(pop)

} // namespace Resource
