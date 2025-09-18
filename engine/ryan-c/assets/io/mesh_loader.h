#pragma once
#include <vector>
#include <span>
#include "assets/core/import_config.h"
#include "graphics/gpu_data.h"

struct aiScene;
struct aiMesh;

namespace AssetLoading
{
  struct ProcessedMesh
  {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
    uint32_t materialIndex = UINT32_MAX;
    vec4 bounds{0, 0, 0, 1}; // x,y,z = center, w = radius
  };

  namespace MeshLoading
  {
    ProcessedMesh extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const LoadingConfig& config);

    // Collection utilities
    std::vector<const aiMesh*> collectMeshPointers(const aiScene* scene, const LoadingConfig& config);

    // Utility functions
    bool isValidMesh(const ProcessedMesh& mesh);

    vec4 calculateBounds(std::span<const Vertex> vertices);

    std::string generateMeshCacheKey(const ProcessedMesh& mesh);

    // Core extraction implementations (internal use)
    namespace Detail
    {
      void extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const LoadingConfig& config);
    }
  }
} // namespace AssetLoading
