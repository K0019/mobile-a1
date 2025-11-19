#include "mesh_loader.h"
#include "logging/log.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include "assets/processing/mesh_process.h"

namespace
{
  mat4 toMat4(const aiMatrix4x4& matrix)
  {
    return mat4(matrix.a1, matrix.b1, matrix.c1, matrix.d1,
                matrix.a2, matrix.b2, matrix.c2, matrix.d2,
                matrix.a3, matrix.b3, matrix.c3, matrix.d3,
                matrix.a4, matrix.b4, matrix.c4, matrix.d4);
  }

  void buildNodeMaps(const aiNode* node,
                     const mat4& parentTransform,
                     std::unordered_map<std::string, mat4>& transforms,
                     std::unordered_map<std::string, const aiNode*>& nodes)
  {
    if(!node)
      return;

    const mat4 local = toMat4(node->mTransformation);
    const mat4 global = parentTransform * local;
    const std::string nodeName = node->mName.length > 0 ? node->mName.C_Str() : "";

    if(!nodeName.empty())
    {
      transforms[nodeName] = global;
      nodes[nodeName] = node;
    }

    for(uint32_t i = 0; i < node->mNumChildren; ++i)
    {
      buildNodeMaps(node->mChildren[i], global, transforms, nodes);
    }
  }

  void accumulateWeight(Resource::SkinningData& data, uint32_t jointIndex, float weight)
  {
    if(weight <= 0.0f)
      return;

    for(size_t i = 0; i < data.boneIndices.size(); ++i)
    {
      if(data.boneIndices[i] == jointIndex)
      {
        data.weights[i] += weight;
        return;
      }
    }

    for(size_t i = 0; i < data.boneIndices.size(); ++i)
    {
      if(data.boneIndices[i] == Resource::INVALID_BONE_INDEX)
      {
        data.boneIndices[i] = jointIndex;
        data.weights[i] = weight;
        return;
      }
    }

    size_t smallest = 0;
    for(size_t i = 1; i < data.weights.size(); ++i)
    {
      if(data.weights[i] < data.weights[smallest])
      {
        smallest = i;
      }
    }

    if(weight > data.weights[smallest])
    {
      data.boneIndices[smallest] = jointIndex;
      data.weights[smallest] = weight;
    }
  }

  void normalizeWeights(Resource::SkinningData& data)
  {
    float totalWeight = 0.0f;
    for(size_t i = 0; i < data.weights.size(); ++i)
    {
      if(data.boneIndices[i] != Resource::INVALID_BONE_INDEX)
      {
        totalWeight += data.weights[i];
      }
    }

    if(totalWeight > 0.0f)
    {
      const float inv = 1.0f / totalWeight;
      for(size_t i = 0; i < data.weights.size(); ++i)
      {
        if(data.boneIndices[i] != Resource::INVALID_BONE_INDEX)
          data.weights[i] = std::clamp(data.weights[i] * inv, 0.0f, 1.0f);
        else
          data.weights[i] = 0.0f;
      }
    }
    else
    {
      data = Resource::SkinningData{};
    }
  }
} // namespace

namespace Resource::MeshLoading
{
  ProcessedMesh extractMesh(const aiScene* scene, const aiMesh* aiMesh, uint32_t meshIndex, const LoadingConfig& config)
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

    const size_t vertexCount = mesh.vertices.size();

    if(scene && aiMesh->mNumBones > 0 && vertexCount > 0)
    {
      mesh.skinning.resize(vertexCount);

      mesh.skeleton.parentIndices.assign(aiMesh->mNumBones, INVALID_JOINT_INDEX);
      mesh.skeleton.inverseBindMatrices.resize(aiMesh->mNumBones, mat4(1.0f));
      mesh.skeleton.bindPoseMatrices.resize(aiMesh->mNumBones, mat4(1.0f));
      mesh.skeleton.jointNames.resize(aiMesh->mNumBones);

      std::unordered_map<std::string, uint32_t> boneIndexMap;
      boneIndexMap.reserve(aiMesh->mNumBones);

      std::unordered_map<std::string, mat4> nodeTransforms;
      std::unordered_map<std::string, const aiNode*> nodeLookup;
      buildNodeMaps(scene->mRootNode, mat4(1.0f), nodeTransforms, nodeLookup);

      for(uint32_t boneIdx = 0; boneIdx < aiMesh->mNumBones; ++boneIdx)
      {
        const aiBone* bone = aiMesh->mBones[boneIdx];
        const std::string boneName = bone->mName.length > 0 ? bone->mName.C_Str() : ("Bone_" + std::to_string(boneIdx));
        mesh.skeleton.jointNames[boneIdx] = boneName;
        mesh.skeleton.inverseBindMatrices[boneIdx] = toMat4(bone->mOffsetMatrix);
        boneIndexMap[boneName] = boneIdx;
      }

      for(uint32_t boneIdx = 0; boneIdx < aiMesh->mNumBones; ++boneIdx)
      {
        const aiBone* bone = aiMesh->mBones[boneIdx];
        const std::string boneName = mesh.skeleton.jointNames[boneIdx];

        auto nodeIt = nodeLookup.find(boneName);
        if(nodeIt != nodeLookup.end())
        {
          const aiNode* node = nodeIt->second->mParent;
          while(node)
          {
            const std::string parentName = node->mName.length > 0 ? node->mName.C_Str() : "";
            if(auto parentIt = boneIndexMap.find(parentName); parentIt != boneIndexMap.end())
            {
              mesh.skeleton.parentIndices[boneIdx] = parentIt->second;
              break;
            }
            node = node->mParent;
          }
        }

        auto transformIt = nodeTransforms.find(boneName);
        if(transformIt != nodeTransforms.end())
        {
          mesh.skeleton.bindPoseMatrices[boneIdx] = transformIt->second;
        }
        else
        {
          mesh.skeleton.bindPoseMatrices[boneIdx] = mat4(1.0f);
          LOG_WARNING("Mesh '{}' bone '{}' missing node transform data", mesh.name, boneName);
        }

        for(uint32_t weightIdx = 0; weightIdx < bone->mNumWeights; ++weightIdx)
        {
          const aiVertexWeight& weight = bone->mWeights[weightIdx];
          if(weight.mVertexId < vertexCount)
          {
            accumulateWeight(mesh.skinning[weight.mVertexId], boneIdx, weight.mWeight);
          }
        }
      }

      for(auto& weights : mesh.skinning)
      {
        normalizeWeights(weights);
      }

      bool anySkinnedVertex = std::any_of(mesh.skinning.begin(), mesh.skinning.end(),
        [](const SkinningData& data)
        {
          return data.boneIndices[0] != INVALID_BONE_INDEX;
        });

      if(!anySkinnedVertex)
      {
        mesh.skinning.clear();
        mesh.skeleton = ProcessedSkeleton{};
      }
    }

    if(aiMesh->mNumAnimMeshes > 0 && vertexCount > 0)
    {
      mesh.morphTargets.reserve(aiMesh->mNumAnimMeshes);
      for(uint32_t targetIdx = 0; targetIdx < aiMesh->mNumAnimMeshes; ++targetIdx)
      {
        const aiAnimMesh* animMesh = aiMesh->mAnimMeshes[targetIdx];
        MorphTargetData target;
        target.name = animMesh->mName.length > 0 ? animMesh->mName.C_Str() : ("Morph_" + std::to_string(targetIdx));
        target.deltas.resize(vertexCount);

        const uint32_t limit = std::min<uint32_t>(vertexCount, animMesh->mNumVertices);
        for(uint32_t v = 0; v < limit; ++v)
        {
          auto& delta = target.deltas[v];
          delta.vertexIndex = v;

          if(animMesh->mVertices)
          {
            const aiVector3D& animPos = animMesh->mVertices[v];
            const vec3 basePos = mesh.vertices[v].position;
            delta.deltaPosition = vec3(animPos.x, animPos.y, animPos.z) - basePos;
          }

          if(animMesh->mNormals && v < animMesh->mNumVertices)
          {
            const aiVector3D& animNormal = animMesh->mNormals[v];
            const vec3 baseNormal = mesh.vertices[v].normal;
            delta.deltaNormal = vec3(animNormal.x, animNormal.y, animNormal.z) - baseNormal;
          }

          if(animMesh->mTangents && v < animMesh->mNumVertices)
          {
            const aiVector3D& animTangent = animMesh->mTangents[v];
            const vec3 baseTangent = vec3(mesh.vertices[v].tangent);
            delta.deltaTangent = vec3(animTangent.x, animTangent.y, animTangent.z) - baseTangent;
          }
        }

        mesh.morphTargets.push_back(std::move(target));
      }
    }

    if (!MeshOptimizer::generateTangents(mesh.vertices, mesh.indices, &mesh.skinning, &mesh.morphTargets))
    {
        LOG_WARNING("Failed to generate tangents for mesh '{}'", mesh.name);
    }

    if (config.optimizeMeshes && MeshOptimizer::shouldOptimize(mesh.vertices, mesh.indices))
    {
        auto result = MeshOptimizer::optimize(mesh.vertices, mesh.indices, &mesh.skinning, &mesh.morphTargets);

        if (!result.success)
        {
            LOG_WARNING("Failed to optimize mesh '{}'", mesh.name);
        }
    }

    // Calculate bounding sphere if requested
    if (!mesh.vertices.empty())
    {
      mesh.bounds = calculateBounds(mesh.vertices, mesh.morphTargets.empty() ? nullptr : &mesh.morphTargets);
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

  vec4 calculateBounds(std::span<const Vertex> vertices, const std::vector<MorphTargetData>* morphTargets)
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

    for (size_t i = 0; i < vertices.size(); ++i)
    {
      const float distance = glm::length(vertices[i].position - center);
      radius = std::max(radius, distance);
    }

    if (morphTargets)
    {
        std::vector<float> maxDisplacement(vertices.size(), 0.0f);
        for (const auto& target : *morphTargets)
        {
            for (const auto& delta : target.deltas)
            {
                maxDisplacement[delta.vertexIndex] += glm::length(delta.deltaPosition);
            }
        }

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            const float distanceFromCenter = glm::length(vertices[i].position - center);
            const float maxPossibleDistance = distanceFromCenter + maxDisplacement[i];
            radius = std::max(radius, maxPossibleDistance);
        }
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
