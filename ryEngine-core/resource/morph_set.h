#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "resource/animation_ids.h"
#include "resource/resource_types.h"

namespace Resource
{
  class MorphSet
  {
    public:
      MorphSet() = default;
      explicit MorphSet(const std::vector<MeshMorphTargetInfo>& targets);

      uint32_t count() const { return static_cast<uint32_t>(m_targets.size()); }
      std::span<const MeshMorphTargetInfo> targets() const { return m_targets; }

    private:
      std::vector<MeshMorphTargetInfo> m_targets;
  };
} // namespace Resource
