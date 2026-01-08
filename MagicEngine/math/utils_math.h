#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <vector>

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

namespace Math
{
  static constexpr float PI = glm::pi<float>();
  static constexpr float TWOPI = glm::pi<float>() * 2;
} // namespace Math

inline vec2 clampLength(const vec2& v, float maxLength)
{
  const float l = length(v);

  return l > maxLength ? normalize(v) * maxLength : v;
}

struct BoundingBox
{
  vec3 min_;
  vec3 max_;

  BoundingBox() = default;

  BoundingBox(const vec3& min, const vec3& max) : min_(glm::min(min, max)), max_(glm::max(min, max)) {}

  BoundingBox(const vec3* points, size_t numPoints)
  {
    vec3 vmin(std::numeric_limits<float>::max());
    vec3 vmax(std::numeric_limits<float>::lowest());

    for (size_t i = 0; i != numPoints; i++)
    {
      vmin = min(vmin, points[i]);
      vmax = max(vmax, points[i]);
    }
    min_ = vmin;
    max_ = vmax;
  }

  vec3 getSize() const { return {max_[0] - min_[0], max_[1] - min_[1], max_[2] - min_[2]}; }

  vec3 getCenter() const { return 0.5f * vec3(max_[0] + min_[0], max_[1] + min_[1], max_[2] + min_[2]); }

  void transform(const mat4& t)
  {
    vec3 corners[] = {vec3(min_.x, min_.y, min_.z), vec3(min_.x, max_.y, min_.z), vec3(min_.x, min_.y, max_.z), vec3(min_.x, max_.y, max_.z), vec3(max_.x, min_.y, min_.z), vec3(max_.x, max_.y, min_.z), vec3(max_.x, min_.y, max_.z), vec3(max_.x, max_.y, max_.z),};
    for (auto& v : corners)
      v = vec3(t * vec4(v, 1.0f));
    *this = BoundingBox(corners, 8);
  }

  BoundingBox getTransformed(const mat4& t) const
  {
    BoundingBox b = *this;
    b.transform(t);
    return b;
  }

  void combinePoint(const vec3& p)
  {
    min_ = min(min_, p);
    max_ = max(max_, p);
  }
};

template <typename T>
T clamp(T v, T a, T b)
{
  if (v < a)
    return a;
  if (v > b)
    return b;
  return v;
}

namespace internal
{
  static std::mt19937 globalRandomEngine;

  struct RandomEngineSeeder
  {
    RandomEngineSeeder()
    {
      std::random_device rd;
      globalRandomEngine.seed(rd());
      // globalRandomEngine.seed(12345);
    }
  };

  static RandomEngineSeeder seeder;
} // namespace internal

template <typename T>
T randomRange(T minInclusive, T maxExclusive)
{
  static_assert(std::is_arithmetic_v<T>, "randomRange requires arithmetic type");
  assert(minInclusive <= maxExclusive && "minInclusive must be less than or equal to maxExclusive");
  if constexpr (std::is_integral_v<T>)
  {
    if (minInclusive == maxExclusive) { return minInclusive; }
    std::uniform_int_distribution<T> dist(minInclusive, maxExclusive - 1);
    return dist(internal::globalRandomEngine);
  }
  else if constexpr (std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<T> dist(minInclusive, maxExclusive);
    return dist(internal::globalRandomEngine);
  }
  else
    return {}; // avoid compile time warning
}

inline float random01() { return randomRange(0.0f, 1.0f); }

inline float randomFloat(float min, float max) { return randomRange(min, max); }

inline vec3 randomVec(const vec3& min, const vec3& max)
{
  return {randomFloat(min.x, max.x), randomFloat(min.y, max.y), randomFloat(min.z, max.z)};
}

inline vec3 randVec() { return randomVec(vec3(-5.0f, -5.0f, -5.0f), vec3(5.0f, 5.0f, 5.0f)); }

inline void getFrustumPlanes(mat4 viewProj, vec4* planes)
{
  viewProj = transpose(viewProj);
  planes[0] = vec4(viewProj[3] + viewProj[0]); // left
  planes[1] = vec4(viewProj[3] - viewProj[0]); // right
  planes[2] = vec4(viewProj[3] + viewProj[1]); // bottom
  planes[3] = vec4(viewProj[3] - viewProj[1]); // top
  planes[4] = vec4(viewProj[3] + viewProj[2]); // near
  planes[5] = vec4(viewProj[3] - viewProj[2]); // far
}

inline void getFrustumCorners(mat4 viewProj, vec4* points)
{
  const vec4 corners[] = {vec4(-1, -1, -1, 1), vec4(1, -1, -1, 1), vec4(1, 1, -1, 1), vec4(-1, 1, -1, 1), vec4(-1, -1, 1, 1), vec4(1, -1, 1, 1), vec4(1, 1, 1, 1), vec4(-1, 1, 1, 1)};

  const mat4 invViewProj = inverse(viewProj);

  for (int i = 0; i != 8; i++)
  {
    const vec4 q = invViewProj * corners[i];
    points[i] = q / q.w;
  }
}

inline bool isBoxInFrustum(vec4* frustumPlanes, vec4* frustumCorners, const BoundingBox& box)
{
  using glm::dot;

  for (int i = 0; i < 6; i++)
  {
    int r = 0;
    r += dot(frustumPlanes[i], vec4(box.min_.x, box.min_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.max_.x, box.min_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.min_.x, box.max_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.max_.x, box.max_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.min_.x, box.min_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.max_.x, box.min_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.min_.x, box.max_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
    r += dot(frustumPlanes[i], vec4(box.max_.x, box.max_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
    if (r == 8)
      return false;
  }

  // check frustum outside/inside box
  int r = 0;
  r = 0;
  for (int i = 0; i < 8; i++)
    r += frustumCorners[i].x > box.max_.x ? 1 : 0;
  if (r == 8)
    return false;
  r = 0;
  for (int i = 0; i < 8; i++)
    r += frustumCorners[i].x < box.min_.x ? 1 : 0;
  if (r == 8)
    return false;
  r = 0;
  for (int i = 0; i < 8; i++)
    r += frustumCorners[i].y > box.max_.y ? 1 : 0;
  if (r == 8)
    return false;
  r = 0;
  for (int i = 0; i < 8; i++)
    r += frustumCorners[i].y < box.min_.y ? 1 : 0;
  if (r == 8)
    return false;
  r = 0;
  for (int i = 0; i < 8; i++)
    r += frustumCorners[i].z > box.max_.z ? 1 : 0;
  if (r == 8)
    return false;
  r = 0;
  for (int i = 0; i < 8; i++)
    r += frustumCorners[i].z < box.min_.z ? 1 : 0;
  if (r == 8)
    return false;

  return true;
}

inline BoundingBox combineBoxes(const std::vector<BoundingBox>& boxes)
{
  std::vector<vec3> allPoints;
  allPoints.reserve(boxes.size() * 8);

  for (const auto& b : boxes)
  {
    allPoints.emplace_back(b.min_.x, b.min_.y, b.min_.z);
    allPoints.emplace_back(b.min_.x, b.min_.y, b.max_.z);
    allPoints.emplace_back(b.min_.x, b.max_.y, b.min_.z);
    allPoints.emplace_back(b.min_.x, b.max_.y, b.max_.z);

    allPoints.emplace_back(b.max_.x, b.min_.y, b.min_.z);
    allPoints.emplace_back(b.max_.x, b.min_.y, b.max_.z);
    allPoints.emplace_back(b.max_.x, b.max_.y, b.min_.z);
    allPoints.emplace_back(b.max_.x, b.max_.y, b.max_.z);
  }

  return {allPoints.data(), allPoints.size()};
}
