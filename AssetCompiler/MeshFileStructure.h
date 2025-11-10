/******************************************************************************/
/*!
\file   MeshFileStructure.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Defines the structure of a .mesh file

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <cstdint>
#include "CompilerMath.h"
#include "CompilerTypes.h"

namespace compiler
{
    constexpr uint32_t MESH_FILE_MAGIC = { 'MESH' };

    #pragma pack(push, 1)
    // Header at the very start of the file
    struct MeshFileHeader
    {
        uint32_t magic = MESH_FILE_MAGIC;

        uint32_t numNodes;
        uint32_t numMeshes;
        uint32_t totalIndices;
        uint32_t totalVertices;
        uint32_t materialNameBufferSize; // Total size of the material name block

        // Skelaton
        uint32_t hasSkeleton;
        uint32_t numBones;
        uint32_t boneNameBufferSize;

        // Bounds for the entire scene.
        // This is actually not needed, since
        // in the mesh importer's SetResourceHandlesMesh, we take the bounds of every individual mesh,
        // and nowhere do we ever use the bounds of the entire scene.
        // might be useful to quickly cull the entire scene instead of part by part though??
        vec3 sceneBoundsCenter;
        float sceneBoundsRadius;
        vec3 sceneBoundsMin;
        vec3 sceneBoundsMax;

        // Offsets to the start of each data block from the beginning of the file
        uint64_t nodeDataOffset;
        uint64_t meshInfoDataOffset;
        uint64_t materialNamesOffset;
        uint64_t indexDataOffset;
        uint64_t vertexDataOffset;

        uint64_t skinningDataOffset; 
        uint64_t boneDataOffset;
        uint64_t boneNameOffset;
    };

    // Information for each node inside fbx
    struct MeshNode
    {
        mat4 transform;
        int32_t parentIndex; // Index into the node array, -1 for root
        int32_t meshIndex;   // Index into the mesh info array, -1 if no mesh
        char name[64];       // Fixed-size name for simplicity
    };

    // Describes how to get mesh data from the buffers
    struct MeshInfo
    {
        uint32_t indexCount;
        uint32_t firstIndex;        // Offset into the index buffer
        uint32_t firstVertex;       // Offset into the vertex buffer
        uint32_t materialNameIndex; // Index into the material name offset table

        // Bounding volume for this individual mesh part
        vec4 meshBounds; // (x,y,z, radius)
    };

    // Structure of bone data
    struct MeshFile_Bone
    {
        mat4     inverseBindPose; // Assimp's aiBone::mOffsetMatrix
        mat4     bindPose;        // The bone's global transform in bind pose
        int32_t  parentIndex;     // Index into this file's array of Bones. -1 for root.
        uint32_t nameOffset;      // Offset into the boneNameBuffer
    };
    #pragma pack(pop)

}
