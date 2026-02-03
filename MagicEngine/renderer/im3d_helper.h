#pragma once
#include "scene.h"
#include "frame_data.h"
#include "features/im3d_feature.h"
#include "resource/resource_manager.h"
#include <glm/glm.hpp>
#include <vector>

namespace Im3dHelper
{
  // Initialize Im3d context for this frame (we don't use AppData; just bracket NewFrame/EndFrame)
  void BeginFrame(const FrameData& frameData);

  // Finalize Im3d and submit to render feature (collect raw VertexData per prim type)
  void EndFrame(uint64_t featureHandle, GfxRenderer& renderer);

  // Visualization helpers (sizes in world units)
  void DrawSceneBones(const Resource::Scene& scene, const Resource::ResourceManager& resources,
                      float boneThickness = 0.02f, // World units
                      float jointSize = 0.05f); // World units
  void DrawBoneHierarchy(const glm::mat4& objectTransform, const std::vector<glm::mat4>& boneMatrices,
                         const std::vector<uint32_t>& parentIndices, const std::vector<glm::mat4>& inverseBindMatrices,
                         float boneThickness = 0.02f, // World units
                         float jointSize = 0.05f); // World units
}
