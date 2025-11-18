#include "resource/skeleton.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "resource/processed_assets.h"

namespace Resource
{
  namespace
  {
    const std::string& emptyString()
    {
      static const std::string kEmpty{};
      return kEmpty;
    }

    const mat4& identityMatrix()
    {
      static const mat4 kIdentity = mat4(1.0f);
      return kIdentity;
    }

    const AnimationClip::JointSample& identityJointSample()
    {
      static const AnimationClip::JointSample kIdentity{};
      return kIdentity;
    }
  } // namespace
  Skeleton::Skeleton(const ProcessedSkeleton& skeleton)
    : m_parentIndices(skeleton.parentIndices), m_inverseBindMatrices(skeleton.inverseBindMatrices),
      m_bindPoseMatrices(skeleton.bindPoseMatrices), m_jointNames(skeleton.jointNames)
  {
    for (size_t i = 0; i < m_jointNames.size(); ++i)
    {
      m_jointNameLookup[m_jointNames[i]] = static_cast<int16_t>(i);
    }
    calculateTopoOrder();
    decomposeBindPoses();
  }

  void Skeleton::calculateTopoOrder()
  {
    const uint32_t count = jointCount();
    if (count == 0) return;
    m_topoOrder.clear();
    m_topoOrder.reserve(count);
    std::vector<int32_t> inDegree(count, 0);
    std::vector<std::vector<uint32_t>> adj(count);
    for (uint32_t i = 0; i < count; ++i)
    {
      const uint32_t parent = m_parentIndices[i];
      if (parent != INVALID_JOINT_INDEX)
      {
        inDegree[i]++;
        adj[parent].push_back(i);
      }
    }
    std::vector<uint32_t> queue;
    for (uint32_t i = 0; i < count; ++i)
    {
      if (inDegree[i] == 0)
      {
        queue.push_back(i);
      }
    }
    size_t head = 0;
    while (head < queue.size())
    {
      uint32_t u = queue[head++];
      m_topoOrder.push_back(u);
      for (uint32_t v : adj[u])
      {
        if (--inDegree[v] == 0)
        {
          queue.push_back(v);
        }
      }
    }
    if (m_topoOrder.size() != count)
    {
      // Fallback for cyclic dependencies or malformed skeletons: just use linear order.
      m_topoOrder.resize(count);
      for (uint32_t i = 0; i < count; ++i) m_topoOrder[i] = i;
    }
  }

  void Skeleton::decomposeBindPoses()
  {
    const uint32_t count = jointCount();
    m_decomposedBindPose.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
      vec3 scale, translation, skew;
      vec4 perspective;
      glm::quat rotation;
      glm::decompose(m_bindPoseMatrices[i], scale, rotation, translation, skew, perspective);
      m_decomposedBindPose[i] = {translation, rotation, scale};
    }
  }

  const std::string& Skeleton::jointName(uint32_t joint) const
  {
    if (joint >= m_jointNames.size()) return emptyString();
    return m_jointNames[joint];
  }

  const mat4& Skeleton::inverseBind(uint32_t joint) const
  {
    if (joint >= m_inverseBindMatrices.size()) return identityMatrix();
    return m_inverseBindMatrices[joint];
  }

  const mat4& Skeleton::bindPose(uint32_t joint) const
  {
    if (joint >= m_bindPoseMatrices.size()) return identityMatrix();
    return m_bindPoseMatrices[joint];
  }

  const AnimationClip::JointSample& Skeleton::bindPoseTRS(uint32_t joint) const
  {
    if (joint >= m_decomposedBindPose.size()) return identityJointSample();
    return m_decomposedBindPose[joint];
  }

  int16_t Skeleton::indexOfJoint(const std::string& name) const
  {
    auto it = m_jointNameLookup.find(name);
    if (it != m_jointNameLookup.end())
    {
      return it->second;
    }
    return -1;
  }
} // namespace Resource
