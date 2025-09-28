#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include "DataStructs.h"

struct aiScene;
struct aiMesh;

namespace compiler
{


    struct ProcessedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;
        uint32_t materialIndex = UINT32_MAX;
        vec4 bounds{ 0, 0, 0, 1 }; // x,y,z = center, w = radius
    };

    ProcessedMesh extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const LoadingConfig& config);

    // Collection utilities
    std::vector<const aiMesh*> collectMeshPointers(const aiScene* scene, const LoadingConfig& config);

    // Utility functions
    vec4 calculateBounds(std::span<const Vertex> vertices);

    void extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const LoadingConfig& config);
}
