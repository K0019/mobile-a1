#include "resource/morph_set.h"

namespace Resource
{
  MorphSet::MorphSet(const std::vector<MeshMorphTargetInfo>& targets)
    : m_targets(targets)
  {
  }

} // namespace Resource
