#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include "math/utils_math.h"
#include "resource/animation_clip.h"
#include "resource/animation_ids.h"

namespace Resource
{
  struct ProcessedSkeleton;

  class Skeleton
  {
  public:
    Skeleton() = default;

    explicit Skeleton(const ProcessedSkeleton& skeleton);

    uint32_t jointCount() const { return static_cast<uint32_t>(m_parentIndices.size()); }

    std::span<const uint32_t> parentIndices() const { return m_parentIndices; }

    std::span<const mat4> inverseBindMatrices() const { return m_inverseBindMatrices; }

    std::span<const mat4> bindPoseMatrices() const { return m_bindPoseMatrices; }

    std::span<const std::string> jointNames() const { return m_jointNames; }

    std::span<const uint32_t> topoOrder() const { return m_topoOrder; }

    const std::string& jointName(uint32_t joint) const;

    const mat4& inverseBind(uint32_t joint) const;

    const mat4& bindPose(uint32_t joint) const;

    const AnimationClip::JointSample& bindPoseTRS(uint32_t joint) const;

    int16_t indexOfJoint(const std::string& name) const;

  private:
    void calculateTopoOrder();

    void decomposeBindPoses();

    std::vector<uint32_t> m_parentIndices;
    std::vector<mat4> m_inverseBindMatrices;
    std::vector<mat4> m_bindPoseMatrices;
    std::vector<std::string> m_jointNames;
    std::unordered_map<std::string, int16_t> m_jointNameLookup;
    std::vector<uint32_t> m_topoOrder;
    std::vector<AnimationClip::JointSample> m_decomposedBindPose;
  };
} // namespace Resource
