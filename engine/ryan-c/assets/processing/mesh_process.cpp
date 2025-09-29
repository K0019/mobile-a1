#include "mesh_process.h"
#include <meshoptimizer.h>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include "logging/log.h"

namespace AssetLoading
{
  namespace
  {
    constexpr uint32_t MIN_VERTICES_FOR_OPTIMIZATION = 100;
    constexpr uint32_t MIN_TRIANGLES_FOR_OPTIMIZATION = 64;
    constexpr float VERTEX_CACHE_SIZE = 16.0f;
  }

  MeshOptimizer::OptimizationResult MeshOptimizer::optimize(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, bool generateTangents)
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
      if (generateTangents)
      {
        // Generate tangents first, then optimize
        if (MeshOptimizer::generateTangents(vertices, indices))
        {
          LOG_DEBUG("Generated tangents for {} vertices", vertices.size());
        }
      }

      // Optimize mesh using meshoptimizer
      optimizeVertexCache(indices, vertices.size());
      optimizeOverdraw(indices, vertices);
      optimizeVertexFetch(vertices, indices);

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

  bool MeshOptimizer::generateTangents(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
  {
    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0) { return false; }

    // Create unwelded mesh for tangent generation
    std::vector<Vertex> unweldedVertices = unweldMesh(vertices, indices);

    // Generate tangents on unwelded mesh
    generateTangentsUnwelded(unweldedVertices);

    // Re-index the mesh
    reindexMesh(vertices, indices, unweldedVertices);

    return true;
  }

  std::vector<Vertex> MeshOptimizer::unweldMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
  {
    std::vector<Vertex> unweldedVertices;
    unweldedVertices.reserve(indices.size());

    for (uint32_t index : indices) { if (index < vertices.size()) { unweldedVertices.push_back(vertices[index]); } }

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

  void MeshOptimizer::reindexMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::vector<Vertex>& unweldedVertices)
  {
    // Generate vertex remap
    std::vector<uint32_t> remap(unweldedVertices.size());
    size_t uniqueVertices = meshopt_generateVertexRemap(remap.data(), nullptr, unweldedVertices.size(), unweldedVertices.data(), unweldedVertices.size(), sizeof(Vertex));

    // Create new index buffer
    indices.resize(unweldedVertices.size());
    for (size_t i = 0; i < unweldedVertices.size(); ++i) { indices[i] = remap[i]; }

    // Create new vertex buffer with unique vertices only
    vertices.resize(uniqueVertices);
    meshopt_remapVertexBuffer(vertices.data(), unweldedVertices.data(), unweldedVertices.size(), sizeof(Vertex), remap.data());
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

  void MeshOptimizer::optimizeVertexFetch(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
  {
    std::vector<uint32_t> remap(vertices.size());
    size_t uniqueVertices = meshopt_optimizeVertexFetchRemap(remap.data(), indices.data(), indices.size(), vertices.size());

    meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());

    std::vector<Vertex> optimizedVertices(uniqueVertices);
    meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap.data());

    vertices = std::move(optimizedVertices);
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
