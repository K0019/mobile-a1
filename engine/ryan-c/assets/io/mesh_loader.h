#pragma once
#include <vector>
#include <span>

#include "import_config.h"
#include "processed_assets.h"
#include "graphics/gpu_data.h"

struct aiScene;
struct aiMesh;

namespace AssetLoading
{
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
