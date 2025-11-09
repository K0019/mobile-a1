#include "mesh_process.h"
#include <meshoptimizer.h>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <limits>
#include "logging/log.h"

namespace Resource
{
  namespace
  {
    constexpr uint32_t MIN_VERTICES_FOR_OPTIMIZATION = 100;
    constexpr uint32_t MIN_TRIANGLES_FOR_OPTIMIZATION = 64;
    constexpr float VERTEX_CACHE_SIZE = 16.0f;
  }

  MeshOptimizer::OptimizationResult MeshOptimizer::optimize(std::vector<Vertex>& vertices,
                                                            std::vector<uint32_t>& indices,
                                                            std::vector<SkinningData>* skinningData,
                                                            std::vector<MorphTargetData>* morphTargets)
  {
    OptimizationResult result;
    result.originalVertexCount = static_cast<uint32_t>(vertices.size());

    if (!shouldOptimize(vertices, indices))
    {
      result.success = false;
      result.optimizedVertexCount = result.originalVertexCount;
      return result;
    }

    try
    {
      // Optimize mesh using meshoptimizer
      optimizeVertexCache(indices, vertices.size());
      optimizeOverdraw(indices, vertices);
      optimizeVertexFetch(vertices, indices, skinningData, morphTargets);

      result.success = true;
      result.optimizedVertexCount = static_cast<uint32_t>(vertices.size());

      // Calculate performance metrics
      float originalACMR = static_cast<float>(indices.size()) / VERTEX_CACHE_SIZE;
      float optimizedACMR = meshopt_analyzeVertexCache(indices.data(), indices.size(), vertices.size(), static_cast<unsigned int>(VERTEX_CACHE_SIZE), 0, 0).acmr;

      result.vertexCacheImprovement = std::max(0.0f, (originalACMR - optimizedACMR) / originalACMR);
      result.overdrawImprovement = 0.1f; // Conservative estimate
    }
    catch ([[maybe_unused]] const std::exception& e)
    {
      LOG_WARNING("Mesh optimization failed: {}", e.what());
      result.success = false;
      result.optimizedVertexCount = result.originalVertexCount;
    }

    return result;
  }

  bool MeshOptimizer::generateTangents(std::vector<Vertex>& vertices,
                                       std::vector<uint32_t>& indices,
                                       std::vector<SkinningData>* skinningData,
                                       std::vector<MorphTargetData>* morphTargets)
  {
    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0) { return false; }

    // Create unwelded mesh for tangent generation
    std::vector<uint32_t> unweldToOriginal;
    std::vector<Vertex> unweldedVertices = unweldMesh(vertices, indices, unweldToOriginal);

    // Generate tangents on unwelded mesh
    generateTangentsUnwelded(unweldedVertices);

    // Re-index the mesh
    reindexMesh(vertices, indices, unweldedVertices, unweldToOriginal, skinningData, morphTargets);

    return true;
  }

  std::vector<Vertex> MeshOptimizer::unweldMesh(const std::vector<Vertex>& vertices,
                                                const std::vector<uint32_t>& indices,
                                                std::vector<uint32_t>& unweldToOriginal)
  {
    std::vector<Vertex> unweldedVertices;
    unweldedVertices.reserve(indices.size());
    unweldToOriginal.reserve(indices.size());

    for (uint32_t index : indices)
    {
      if (index < vertices.size())
      {
        unweldedVertices.push_back(vertices[index]);
        unweldToOriginal.push_back(index);
      }
      else
      {
        unweldToOriginal.push_back(std::numeric_limits<uint32_t>::max());
      }
    }

    return unweldedVertices;
  }

  void MeshOptimizer::generateTangentsUnwelded(std::vector<Vertex>& unweldedVertices)
  {
    if (unweldedVertices.empty() || unweldedVertices.size() % 3 != 0) { return; }

    // Setup mikktspace context
    MikktspaceContext context;
    context.unweldedVertices = &unweldedVertices;

    SMikkTSpaceInterface interface;
    interface.m_getNumFaces = getNumFaces;
    interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
    interface.m_getPosition = getPosition;
    interface.m_getNormal = getNormal;
    interface.m_getTexCoord = getTexCoord;
    interface.m_setTSpaceBasic = setTSpaceBasic;
    interface.m_setTSpace = nullptr;

    SMikkTSpaceContext mikktContext;
    mikktContext.m_pInterface = &interface;
    mikktContext.m_pUserData = &context;

    if (!genTangSpaceDefault(&mikktContext)) { LOG_WARNING("Failed to generate tangents using mikktspace"); }
  }

  void MeshOptimizer::reindexMesh(std::vector<Vertex>& vertices,
                                  std::vector<uint32_t>& indices,
                                  const std::vector<Vertex>& unweldedVertices,
                                  const std::vector<uint32_t>& unweldToOriginal,
                                  std::vector<SkinningData>* skinningData,
                                  std::vector<MorphTargetData>* morphTargets)
  {
    // Generate vertex remap
    std::vector<uint32_t> remap(unweldedVertices.size());
    size_t uniqueVertices = meshopt_generateVertexRemap(remap.data(),
                                                        nullptr,
                                                        unweldedVertices.size(),
                                                        unweldedVertices.data(),
                                                        unweldedVertices.size(),
                                                        sizeof(Vertex));

    // Create new index buffer
    indices.resize(unweldedVertices.size());
    for (size_t i = 0; i < unweldedVertices.size(); ++i)
    {
      indices[i] = remap[i];
    }

    // Create new vertex buffer with unique vertices only
    vertices.resize(uniqueVertices);
    meshopt_remapVertexBuffer(vertices.data(), unweldedVertices.data(), unweldedVertices.size(), sizeof(Vertex), remap.data());

    // Build mapping from new vertices back to original vertex indices
    std::vector<uint32_t> originalForUnique(uniqueVertices, std::numeric_limits<uint32_t>::max());
    for(size_t i = 0; i < remap.size(); ++i)
    {
      const uint32_t newIndex = remap[i];
      if(newIndex >= uniqueVertices)
        continue;
      uint32_t sourceIndex = (i < unweldToOriginal.size()) ? unweldToOriginal[i] : std::numeric_limits<uint32_t>::max();
      if(originalForUnique[newIndex] == std::numeric_limits<uint32_t>::max() || sourceIndex != std::numeric_limits<uint32_t>::max())
      {
        originalForUnique[newIndex] = sourceIndex;
      }
    }

    uint32_t originalVertexCount = 0;
    for(uint32_t originalIndex : unweldToOriginal)
    {
      if(originalIndex != std::numeric_limits<uint32_t>::max())
        originalVertexCount = std::max(originalVertexCount, originalIndex + 1);
    }

    auto resolveOriginalIndex = [&](uint32_t newIndex) -> uint32_t
    {
      if(newIndex >= originalForUnique.size())
        return std::numeric_limits<uint32_t>::max();
      return originalForUnique[newIndex];
    };

    if (skinningData && !skinningData->empty())
    {
      if (skinningData->size() == originalVertexCount)
      {
        std::vector<SkinningData> remapped(uniqueVertices);
        for (size_t i = 0; i < uniqueVertices; ++i)
        {
          uint32_t originalIndex = resolveOriginalIndex(static_cast<uint32_t>(i));
          if (originalIndex != std::numeric_limits<uint32_t>::max() &&
              static_cast<size_t>(originalIndex) < skinningData->size())
            remapped[i] = (*skinningData)[originalIndex];
          else
            remapped[i] = SkinningData{};
        }
        skinningData->swap(remapped);
      }
      else
      {
        LOG_WARNING("Tangent generation skipped skinning remap due to size mismatch (expected {}, got {})",
                    originalVertexCount,
                    skinningData->size());
      }
    }

    if (morphTargets && !morphTargets->empty())
    {
      for (auto& target : *morphTargets)
      {
        auto& deltas = target.deltas;
        if (deltas.empty())
          continue;

        if (deltas.size() == originalVertexCount)
        {
          std::vector<MorphTargetVertexDelta> remapped(uniqueVertices);
          for (size_t i = 0; i < uniqueVertices; ++i)
          {
            uint32_t originalIndex = resolveOriginalIndex(static_cast<uint32_t>(i));
            if (originalIndex != std::numeric_limits<uint32_t>::max() &&
                static_cast<size_t>(originalIndex) < deltas.size())
              remapped[i] = deltas[originalIndex];
            else
              remapped[i] = MorphTargetVertexDelta{};
          }
          deltas.swap(remapped);
        }
        else
        {
          LOG_WARNING("Tangent generation skipped morph target '{}' remap due to size mismatch (expected {}, got {})",
                      target.name,
                      originalVertexCount,
                      deltas.size());
        }
      }
    }
  }

  void MeshOptimizer::optimizeVertexCache(std::vector<uint32_t>& indices, size_t vertexCount)
  {
    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertexCount);
  }

  void MeshOptimizer::optimizeOverdraw(std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices)
  {
    // Extract positions for overdraw optimization
    std::vector<float> positions;
    positions.reserve(vertices.size() * 3);

    for (const auto& vertex : vertices)
    {
      positions.push_back(vertex.position.x);
      positions.push_back(vertex.position.y);
      positions.push_back(vertex.position.z);
    }

    meshopt_optimizeOverdraw(indices.data(),
                             indices.data(),
                             indices.size(),
                             positions.data(),
                             vertices.size(),
                             sizeof(float) * 3,
                             1.05f // threshold
    );
  }

  void MeshOptimizer::optimizeVertexFetch(std::vector<Vertex>& vertices,
                                          std::vector<uint32_t>& indices,
                                          std::vector<SkinningData>* skinningData,
                                          std::vector<MorphTargetData>* morphTargets)
  {
    if (vertices.empty())
      return;

    const size_t vertexCount = vertices.size();
    std::vector<uint32_t> remap(vertexCount);
    size_t uniqueVertices = meshopt_optimizeVertexFetchRemap(remap.data(), indices.data(), indices.size(), vertexCount);

    meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());

    std::vector<Vertex> optimizedVertices(uniqueVertices);
    meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), vertexCount, sizeof(Vertex), remap.data());

    vertices = std::move(optimizedVertices);

    if (skinningData && !skinningData->empty())
    {
      if (skinningData->size() == vertexCount)
      {
        std::vector<SkinningData> remapped(uniqueVertices);
        meshopt_remapVertexBuffer(remapped.data(),
                                  skinningData->data(),
                                  vertexCount,
                                  sizeof(SkinningData),
                                  remap.data());
        skinningData->swap(remapped);
      }
      else
      {
        LOG_WARNING("Mesh optimization skipped skinning remap due to size mismatch (expected {}, got {})",
                    vertexCount,
                    skinningData->size());
      }
    }

    if (morphTargets && !morphTargets->empty())
    {
      for (auto& target : *morphTargets)
      {
        auto& deltas = target.deltas;
        if (deltas.empty())
          continue;

        if (deltas.size() == vertexCount)
        {
          std::vector<MorphTargetVertexDelta> remapped(uniqueVertices);
          meshopt_remapVertexBuffer(remapped.data(),
                                    deltas.data(),
                                    vertexCount,
                                    sizeof(MorphTargetVertexDelta),
                                    remap.data());
          deltas.swap(remapped);
        }
        else
        {
          LOG_WARNING("Mesh optimization skipped morph target '{}' remap due to size mismatch (expected {}, got {})",
                      target.name,
                      vertexCount,
                      deltas.size());
        }
      }
    }
  }

  bool MeshOptimizer::shouldOptimize(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
  {
    if (vertices.size() < MIN_VERTICES_FOR_OPTIMIZATION) { return false; }

    if (indices.size() < MIN_TRIANGLES_FOR_OPTIMIZATION * 3) { return false; }

    if (indices.size() % 3 != 0) { return false; }

    return true;
  }

  int MeshOptimizer::getNumFaces(const SMikkTSpaceContext* pContext)
  {
    const auto* ctx = static_cast<const MikktspaceContext*>(pContext->m_pUserData);
    return static_cast<int>(ctx->unweldedVertices->size() / 3);
  }

  int MeshOptimizer::getNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* pContext, [[maybe_unused]] const int iFace)
  {
    return 3; // Always triangles
  }

  void MeshOptimizer::getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
  {
    const auto* ctx = static_cast<const MikktspaceContext*>(pContext->m_pUserData);
    const Vertex& vertex = (*ctx->unweldedVertices)[iFace * 3 + iVert];
    fvPosOut[0] = vertex.position.x;
    fvPosOut[1] = vertex.position.y;
    fvPosOut[2] = vertex.position.z;
  }

  void MeshOptimizer::getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
  {
    const auto* ctx = static_cast<const MikktspaceContext*>(pContext->m_pUserData);
    const Vertex& vertex = (*ctx->unweldedVertices)[iFace * 3 + iVert];
    fvNormOut[0] = vertex.normal.x;
    fvNormOut[1] = vertex.normal.y;
    fvNormOut[2] = vertex.normal.z;
  }

  void MeshOptimizer::getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
  {
    const auto* ctx = static_cast<const MikktspaceContext*>(pContext->m_pUserData);
    const Vertex& vertex = (*ctx->unweldedVertices)[iFace * 3 + iVert];
    fvTexcOut[0] = vertex.uv_x;
    fvTexcOut[1] = vertex.uv_y;
  }

  void MeshOptimizer::setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
  {
    auto* ctx = static_cast<MikktspaceContext*>(pContext->m_pUserData);
    Vertex& vertex = (*ctx->unweldedVertices)[iFace * 3 + iVert];

    vertex.tangent.x = fvTangent[0];
    vertex.tangent.y = fvTangent[1];
    vertex.tangent.z = fvTangent[2];
    vertex.tangent.w = fSign;
  }
} // namespace AssetLoading
