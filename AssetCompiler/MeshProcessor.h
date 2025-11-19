/******************************************************************************/
/*!
\file   MeshProcessor.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Rocky Sutarius (0%) - just copied everything
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Uses meshoptimizer to optimise a mesh.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include <mikktspace.h>
#include <vector>
#include <span>
#include "CompilerMath.h"
#include "CompilerTypes.h"

namespace compiler
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

        //static vec4 calculateBounds(std::span<const compiler::Vertex> vertices);
        static vec4 calculateBounds(std::span<const Vertex> vertices, const std::vector<MorphTargetData>* morphTargets);

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
}