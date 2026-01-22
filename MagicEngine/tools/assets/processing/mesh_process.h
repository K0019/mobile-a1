#pragma once
#include <mikktspace.h>
#include <vector>
#include "resource/processed_extraction.h"

namespace Resource
{
  class MeshOptimizer
  {
    public:
      struct OptimizationResult
      {
        bool success = false;
        uint32_t originalVertexCount = 0;
        uint32_t optimizedVertexCount = 0;
        float vertexCacheImprovement = 0.0f;
        float overdrawImprovement = 0.0f;
      };

      // Static optimization functions - no async concerns
      static OptimizationResult optimize(std::vector<Vertex>& vertices,
                                         std::vector<uint32_t>& indices,
                                         std::vector<SkinningData>* skinningData,
                                         std::vector<MorphTargetData>* morphTargets);

      static bool generateTangents(std::vector<Vertex>& vertices,
                                   std::vector<uint32_t>& indices,
                                   std::vector<SkinningData>* skinningData = nullptr,
                                   std::vector<MorphTargetData>* morphTargets = nullptr);

      static void optimizeVertexCache(std::vector<uint32_t>& indices, size_t vertexCount);

      static void optimizeOverdraw(std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices);

      static void optimizeVertexFetch(std::vector<Vertex>& vertices,
                                      std::vector<uint32_t>& indices,
                                      std::vector<SkinningData>* skinningData,
                                      std::vector<MorphTargetData>* morphTargets);

      static bool shouldOptimize(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    private:
      // Mikktspace integration
      struct MikktspaceContext
      {
        std::vector<Vertex>* unweldedVertices;
      };

      static std::vector<Vertex> unweldMesh(const std::vector<Vertex>& vertices,
                                            const std::vector<uint32_t>& indices,
                                            std::vector<uint32_t>& unweldToOriginal);

      static void generateTangentsUnwelded(std::vector<Vertex>& unweldedVertices);

      static void reindexMesh(std::vector<Vertex>& vertices,
                              std::vector<uint32_t>& indices,
                              const std::vector<Vertex>& unweldedVertices,
                              const std::vector<uint32_t>& unweldToOriginal,
                              std::vector<SkinningData>* skinningData,
                              std::vector<MorphTargetData>* morphTargets);

      // Mikktspace callbacks
      static int getNumFaces(const SMikkTSpaceContext* pContext);

      static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace);

      static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert);

      static void getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert);

      static void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert);

      static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert);
  };
} // namespace AssetLoading
