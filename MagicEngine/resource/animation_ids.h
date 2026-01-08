#pragma once

#include <cstdint>

namespace Resource
{
  using ClipId = uint32_t;
  using SkeletonId = uint32_t;
  using MorphSetId = uint32_t;

  inline constexpr ClipId INVALID_CLIP_ID = 0;
  inline constexpr SkeletonId INVALID_SKELETON_ID = 0;
  inline constexpr MorphSetId INVALID_MORPH_SET_ID = 0;
} // namespace Resource

