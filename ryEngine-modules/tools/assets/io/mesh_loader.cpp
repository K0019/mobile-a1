#include "mesh_loader.h"
#include "logging/log.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <algorithm>
#include "assets/processing/mesh_process.h"

namespace Resource::MeshLoading
{
  ProcessedMesh extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const LoadingConfig& config)
  {
    ProcessedMesh mesh;

    // Extract mesh name
    mesh.name = aiMesh->mName.length > 0 ? aiMesh->mName.C_Str() : ("Mesh_" + std::to_string(meshIndex));
    mesh.materialIndex = aiMesh->mMaterialIndex;

    if (!aiMesh->mVertices || aiMesh->mNumVertices == 0)
    {
      return mesh;
    }

    // Reserve space for vertices and indices
    mesh.vertices.reserve(aiMesh->mNumVertices);
    mesh.indices.reserve(aiMesh->mNumFaces * 3);

    // Extract vertices
    Detail::extractVertices(aiMesh, mesh.vertices, config);

    // Extract indices - only process triangular faces
    for (uint32_t i = 0; i < aiMesh->mNumFaces; ++i)
    {
      const aiFace& face = aiMesh->mFaces[i];
      if (face.mNumIndices == 3)
      {
        mesh.indices.push_back(face.mIndices[0]);
        mesh.indices.push_back(face.mIndices[1]);
        mesh.indices.push_back(face.mIndices[2]);
      }
    }

    // Apply vertex count limit if specified
    if (config.maxVerticesPerMesh > 0 && mesh.vertices.size() > config.maxVerticesPerMesh)
    {
      mesh.vertices.resize(config.maxVerticesPerMesh);

      // Remove indices that reference truncated vertices
      std::erase_if(mesh.indices,
                    [maxVertex = config.maxVerticesPerMesh](uint32_t idx)
                    {
                      return idx >= maxVertex;
                    });
    }

    // Apply mesh optimization if requested
    if (config.optimizeMeshes && MeshOptimizer::shouldOptimize(mesh.vertices, mesh.indices))
    {
      auto result = MeshOptimizer::optimize(mesh.vertices, mesh.indices, config.generateTangents);

      if (!result.success)
      {
        LOG_WARNING("Failed to optimize mesh '{}'", mesh.name);
      }
    }
    if (config.generateTangents)
    {
      if (!MeshOptimizer::generateTangents(mesh.vertices, mesh.indices))
      {
        LOG_WARNING("Failed to generate tangents for mesh '{}'", mesh.name);
      }
    }

    // Calculate bounding sphere if requested
    if (config.calculateBounds && !mesh.vertices.empty())
    {
      mesh.bounds = calculateBounds(mesh.vertices);
    }

    return mesh;
  }

  std::vector<const aiMesh*> collectMeshPointers(const aiScene* scene, const LoadingConfig& config)
  {
    if (!scene || !scene->mMeshes || scene->mNumMeshes == 0)
    {
      return {};
    }

    std::vector<const aiMesh*> meshPtrs;
    meshPtrs.reserve(std::min(scene->mNumMeshes, config.maxMeshes));

    for (uint32_t i = 0; i < scene->mNumMeshes && i < config.maxMeshes; ++i)
    {
      if (scene->mMeshes[i] && scene->mMeshes[i]->mNumVertices > 0)
      {
        meshPtrs.push_back(scene->mMeshes[i]);
      }
    }

    return meshPtrs;
  }

  vec4 calculateBounds(std::span<const Vertex> vertices)
  {
    if (vertices.empty()) { return vec4(0.0f, 0.0f, 0.0f, 0.0f); }

    // Find axis-aligned bounding box
    vec3 minPos = vertices[0].position;
    vec3 maxPos = vertices[0].position;

    for (const auto& vertex : vertices)
    {
      minPos = glm::min(minPos, vertex.position);
      maxPos = glm::max(maxPos, vertex.position);
    }

    // Calculate bounding sphere center and radius
    const vec3 center = (minPos + maxPos) * 0.5f;
    float radius = 0.0f;

    for (const auto& vertex : vertices)
    {
      const float distance = glm::length(vertex.position - center);
      radius = std::max(radius, distance);
    }

    return {center.x, center.y, center.z, radius};
  }

  bool isValidMesh(const ProcessedMesh& mesh)
  {
    return !mesh.vertices.empty() && !mesh.indices.empty() && mesh.indices.size() % 3 == 0 && !mesh.name.empty();
  }

  std::string generateMeshCacheKey(const ProcessedMesh& mesh)
  {
    size_t hash = std::hash<std::string>{}(mesh.name);
    hash ^= std::hash<size_t>{}(mesh.vertices.size()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= std::hash<size_t>{}(mesh.indices.size()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return mesh.name + "_" + std::to_string(hash);
  }

  namespace Detail
  {
    void extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const LoadingConfig& config)
    {
      vertices.resize(aiMesh->mNumVertices);

      for (uint32_t i = 0; i < aiMesh->mNumVertices; ++i)
      {
        Vertex& vertex = vertices[i];

        // Position (always present)
        vertex.position = vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);

        // Normal
        if (aiMesh->mNormals)
        {
          vertex.normal = vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
        }
        else
        {
          vertex.normal = vec3(0.0f, 1.0f, 0.0f);
        }

        // UV coordinates (use first texture coordinate set)
        if (aiMesh->mTextureCoords[0])
        {
          const float u = aiMesh->mTextureCoords[0][i].x;
          const float v = aiMesh->mTextureCoords[0][i].y;
          vertex.setUV(u, config.flipUVs ? (1.0f - v) : v);
        }
        else
        {
          vertex.setUV(0.0f, 0.0f);
        }

        // Initialize tangent
        vertex.tangent = vec4(0.0f, 0.0f, 0.0f, 1.0f);
      }
    }
  } // namespace Detail
} // namespace AssetLoading::MeshLoading
