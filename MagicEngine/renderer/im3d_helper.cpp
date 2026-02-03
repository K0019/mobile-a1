#include "im3d_helper.h"
#include "gfx_renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <im3d.h>
#include <cstring>

namespace Im3dHelper
{
  static inline Im3d::Vec2 ToIm3D(const glm::vec2& v) { return {v.x, v.y}; }

  static inline Im3d::Vec3 ToIm3D(const glm::vec3& v) { return {v.x, v.y, v.z}; }

  void BeginFrame(const FrameData& /*frameData*/)
  {
    // We don't rely on AppData; just mark frame scope for user code emitting primitives
    Im3d::NewFrame();
  }

  void EndFrame(uint64_t featureHandle, GfxRenderer& renderer)
  {
    Im3d::EndFrame();
    // Early return if feature handle is invalid (Im3D feature not created)
    if (featureHandle == 0) return;
    const Im3d::DrawList* lists = Im3d::GetDrawLists();
    const uint32_t count = Im3d::GetDrawListCount();
    auto* p = static_cast<Im3dRenderFeature::Parameters*>(renderer.GetFeatureParameterBlockPtr(featureHandle));
    if (!p) return;

    // Count vertices per primitive type
    uint32_t totalPointVerts = 0, totalLineVerts = 0, totalTriVerts = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
      const auto& dl = lists[i];
      switch (dl.m_primType)
      {
      case Im3d::DrawPrimitive_Points:    totalPointVerts += dl.m_vertexCount; break;
      case Im3d::DrawPrimitive_Lines:     totalLineVerts  += dl.m_vertexCount; break;
      case Im3d::DrawPrimitive_Triangles: totalTriVerts   += dl.m_vertexCount; break;
      default: break;
      }
    }

    // Resize packed buffers
    const size_t vertSize = sizeof(Im3d::VertexData);
    p->pointVertices.resize(totalPointVerts);
    p->lineVertices.resize(totalLineVerts);
    p->triVertices.resize(totalTriVerts);

    // Copy vertex data
    uint32_t ip = 0, il = 0, it = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
      const auto& dl = lists[i];
      const uint32_t n = dl.m_vertexCount;
      switch (dl.m_primType)
      {
      case Im3d::DrawPrimitive_Points:
        std::memcpy(&p->pointVertices[ip], dl.m_vertexData, n * vertSize);
        ip += n;
        break;
      case Im3d::DrawPrimitive_Lines:
        std::memcpy(&p->lineVertices[il], dl.m_vertexData, n * vertSize);
        il += n;
        break;
      case Im3d::DrawPrimitive_Triangles:
        std::memcpy(&p->triVertices[it], dl.m_vertexData, n * vertSize);
        it += n;
        break;
      default: break;
      }
    }
  }

  void DrawBoneHierarchy(const glm::mat4& objectTransform, const std::vector<glm::mat4>& boneMatrices,
                         const std::vector<uint32_t>& parentIndices, const std::vector<glm::mat4>& inverseBindMatrices,
                         float boneThickness, float jointSize)
  {
    const uint32_t jointCount = (uint32_t)boneMatrices.size();
    if (!jointCount || jointCount != parentIndices.size() || jointCount != inverseBindMatrices.size()) return;
    std::vector<glm::vec3> globalJointPositions(jointCount);
    for (uint32_t i = 0; i < jointCount; ++i)
    {
      const glm::mat4 globalJoint = boneMatrices[i] * glm::inverse(inverseBindMatrices[i]);
      const glm::vec4 posWorld = objectTransform * (globalJoint * glm::vec4(0, 0, 0, 1));
      globalJointPositions[i] = glm::vec3(posWorld);
    }
    // Bones (lines)
    Im3d::PushDrawState();
    Im3d::SetSize(boneThickness);
    for (uint32_t j = 0; j < jointCount; ++j)
    {
      const uint32_t pIdx = parentIndices[j];
      if (pIdx != Resource::INVALID_JOINT_INDEX && pIdx < jointCount)
      {
        const glm::vec3 parentPos = globalJointPositions[pIdx];
        const glm::vec3 jointPos = globalJointPositions[j];
        const glm::vec3 dir = glm::normalize(jointPos - parentPos);
        const glm::vec3 rgb = glm::abs(dir);
        const Im3d::Color col(rgb.x, rgb.y, rgb.z, 1.0f);
        Im3d::DrawLine(ToIm3D(parentPos), ToIm3D(jointPos), boneThickness, col);
      }
    }
    Im3d::PopDrawState();
    // Joints (points)
    Im3d::PushDrawState();
    Im3d::SetSize(jointSize);
    const Im3d::Color jointCol = Im3d::Color_Yellow;
    for (uint32_t j = 0; j < jointCount; ++j)
    {
      Im3d::DrawPoint(ToIm3D(globalJointPositions[j]), jointSize, jointCol);
    }
    Im3d::PopDrawState();
  }

  void DrawSceneBones(const Resource::Scene& scene, const Resource::ResourceManager& resources, float boneThickness,
                      float jointSize)
  {
    const size_t objectCount = scene.objects.size();
    for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
    {
      const SceneObject& object = scene.objects[objectIndex];
      if (!object.mesh.isValid()) continue;
      const SceneObject::AnimBinding* anim = object.anim.has_value() ? &object.anim.value() : nullptr;
      if (!anim || anim->jointCount == 0) continue;
      const auto* meshCold = resources.getMeshMetadata(object.mesh);
      if (!meshCold || meshCold->jointParentIndices.empty()) continue;
      const uint32_t jointCount = anim->jointCount;
      if (anim->skinMatrices.size() < jointCount) continue;
      std::vector<glm::mat4> boneMatrices(jointCount);
      for (uint32_t i = 0; i < jointCount; ++i)
      {
        boneMatrices[i] = anim->skinMatrices[i];
      }
      DrawBoneHierarchy(object.transform, boneMatrices, meshCold->jointParentIndices,
                        meshCold->jointInverseBindMatrices, boneThickness, jointSize);
    }
  }
} // namespace Im3dHelper
