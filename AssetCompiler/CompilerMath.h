/******************************************************************************/
/*!
\file   CompilerMath.h
\brief  Math types for the compiler - now re-exports from MagicEngine
*/
/******************************************************************************/

#pragma once

// Use MagicEngine's math and vertex types directly
#include "math/utils_math.h"
#include "renderer/gpu_data.h"
#include "resource/processed_assets.h"

// Re-export glm types at global scope (for compatibility with existing code)
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

namespace compiler
{
    // Re-export Vertex from engine (it's in global namespace)
    using ::Vertex;

    // Animation key types (same as engine's)
    using PositionKey = Resource::ProcessedAnimationVectorKey;
    using RotationKey = Resource::ProcessedAnimationQuatKey;
    using ScaleKey = Resource::ProcessedAnimationVectorKey;
}
