#pragma once
#include <vector>
#include <span>
#include "resource/processed_extraction.h"
#include "import_config.h"
struct aiScene;
struct aiMesh;

namespace Resource
{
  namespace MeshLoading
  {
    ProcessedMesh extractMesh(const aiScene* scene, const aiMesh* aiMesh, uint32_t meshIndex,
                              const LoadingConfig& config);

    // Collection utilities
    std::vector<const aiMesh*> collectMeshPointers(const aiScene* scene, const LoadingConfig& config);

    // Utility functions
    bool isValidMesh(const ProcessedMesh& mesh);

    vec4 calculateBounds(std::span<const Vertex> vertices, const std::vector<MorphTargetData>* morphTargets = nullptr);

    std::string generateMeshCacheKey(const ProcessedMesh& mesh);

    // Core extraction implementations (internal use)
    namespace Detail
    {
      void extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const LoadingConfig& config);
    }
  } // namespace MeshLoading
} // namespace AssetLoading
