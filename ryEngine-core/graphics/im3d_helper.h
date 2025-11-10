#pragma once

#include "scene.h"
#include "frame_data.h"
#include "features/im3d_feature.h"
#include "resource/resource_manager.h"
#include <glm/glm.hpp>
#include <vector>

struct PointGPU {
    glm::vec3 p; float sizePx;                 // 16B
    uint32_t  abgr; uint32_t _pad0, _pad1, _pad2; // 16B, struct = 32B
};

struct LineGPU {
    glm::vec3 p0; float sizePx;  // 16B
    glm::vec3 p1; uint32_t abgr; // 16B, struct = 32B
};

struct TriGPU {
    glm::vec3 p0; uint32_t c0;   // 16B
    glm::vec3 p1; uint32_t c1;   // 16B
    glm::vec3 p2; uint32_t c2;   // 16B, struct = 48B
};

static_assert(sizeof(PointGPU) % 16 == 0 && sizeof(LineGPU) % 16 == 0 && sizeof(TriGPU) % 16 == 0,
    "std430 alignment");

namespace Im3dHelper
{
    // Initialize Im3d context for this frame (we don't use AppData; just bracket NewFrame/EndFrame)
    void BeginFrame(const FrameData& frameData);

    // Finalize Im3d and submit to render feature (build packed buffer once on CPU)
    void EndFrame(uint64_t featureHandle, Renderer& renderer);

    // Visualization helpers
    void DrawSceneBones(const Resource::Scene& scene,
        const Resource::ResourceManager& resources,
        float boneThickness = 2.0f,
        float jointSize = 4.0f);

    void DrawBoneHierarchy(const glm::mat4& objectTransform,
        const std::vector<glm::mat4>& boneMatrices,
        const std::vector<uint32_t>& parentIndices,
        const std::vector<glm::mat4>& inverseBindMatrices,
        float boneThickness = 2.0f,
        float jointSize = 4.0f);
}
