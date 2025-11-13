/******************************************************************************/
/*!
\file   CompilerMath.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Tiny math definition header for the compiler

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <cmath>
#include <random>
#include <vector>
#include <variant>
#include <filesystem>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

namespace compiler
{
    struct Vertex
    {
        vec3 position;
        float uv_x;
        vec3 normal;
        float uv_y;
        vec4 tangent; // xyz = tangent direction, w = bitangent handedness

        //uint8_t  bone_indices[4] = { 0 };
        //uint8_t  bone_weights[4] = { 0 };

        Vertex() = default;

        Vertex(const vec3& pos, const vec3& n, const vec2& uv) : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(0.0f, 0.0f, 0.0f, 1.0f) {}

        Vertex(const vec3& pos, const vec3& n, const vec2& uv, const vec4& tan) : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(tan) {}

        // Helper to get UV as vec2
        vec2 getUV() const { return { uv_x, uv_y }; }

        void setUV(const vec2& uv)
        {
            uv_x = uv.x;
            uv_y = uv.y;
        }

        void setUV(float u, float v)
        {
            uv_x = u;
            uv_y = v;
        }
    };


    // ----- Skeletal Animation structs ----- //
    struct PositionKey
    {
        float time; // Timestamp in ticks
        vec3  value{ 0.0f };
    };

    struct RotationKey
    {
        float time;
        quat  value{ 1.0f, 0.0f, 0.0f, 0.0f };
    };

    struct ScaleKey
    {
        float time;
        vec3  value{ 0.0f };
    };
}
