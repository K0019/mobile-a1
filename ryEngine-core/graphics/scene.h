#pragma once

#include "resource/resource_types.h"
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <vector>

namespace Resource
{
  class SceneLoader;
  class ResourceManager;
}

class Renderer;

enum class LightType : uint8_t
{
  Directional = 0,
  Point       = 1,
  Spot        = 2,
  Area        = 3
};

struct SceneLight
{
  LightType type = LightType::Point;
  vec3 position = vec3(0.0f);
  vec3 direction = vec3(0.0f, -1.0f, 0.0f);
  vec3 color = vec3(1.0f);
  vec3 attenuation = vec3(1.0f, 0.09f, 0.032f); // constant, linear, quadratic
  float intensity = 1.0f;
  float innerConeAngle = 0.0f;   // For spot lights (radians)
  float outerConeAngle = 0.785f; // 45 degrees in radians
  vec2 areaSize = vec2(1.0f);    // For area lights
  std::string name;
};

struct SceneCamera
{
  vec3 position = vec3(0.0f);
  vec3 target = vec3(0.0f, 0.0f, -1.0f);
  vec3 up = vec3(0.0f, 1.0f, 0.0f);
  float fov = 60.0f; // Field of view in degrees
  float nearPlane = 0.1f;
  float farPlane = 1000.0f;
  float aspectRatio = 16.0f / 9.0f;
  std::string name;
};

enum class SceneObjectType : uint8_t
{
  Mesh   = 0,
  Light  = 1,
  Camera = 2,
  Empty  = 3
};

struct SceneObject
{
  SceneObjectType type = SceneObjectType::Empty;
  mat4 transform = mat4(1.0f);

  // Mesh data
  MeshHandle mesh;
  MaterialHandle material;
  // Light/Camera indices (into scene arrays)
  uint32_t lightIndex = UINT32_MAX;
  uint32_t cameraIndex = UINT32_MAX;

  uint32_t flags = 0; // visibility, shadow casting, etc.
  std::string name;
  struct AnimBinding
  {
    enum FlagBits : uint8_t
    {
      Playing = 1 << 0,
      Loop    = 1 << 1,
      Crossfade = 1 << 2
    };

    Resource::SkeletonId skeleton = Resource::INVALID_SKELETON_ID;
    Resource::ClipId clipA = Resource::INVALID_CLIP_ID;
    Resource::ClipId clipB = Resource::INVALID_CLIP_ID;
    Resource::MorphSetId morphSet = Resource::INVALID_MORPH_SET_ID;
    uint16_t jointCount = 0;
    uint16_t morphCount = 0;
    float timeA = 0.0f;
    float timeB = 0.0f;
    float speed = 1.0f;
    float blend = 0.0f;
    uint8_t flags = 0;
    std::vector<mat4> skinMatrices;
    std::vector<float> morphWeights;
    // Map from mesh joint index to skeleton joint index
    std::vector<int16_t> jointRemap;
    // Copy of the mesh's inverse bind matrices, in mesh joint order
    std::vector<mat4> invBindMatrices;
  };

  std::optional<AnimBinding> anim;

  bool isRenderable() const { return type == SceneObjectType::Mesh && mesh.isValid() && material.isValid(); }

  uint32_t getSourceMeshIndex() const { return meshIndex; }

  private:
    // Temporary loading indices - only accessible to SceneLoader
    friend class Resource::SceneLoader;
    uint32_t meshIndex = UINT32_MAX;
    uint32_t materialIndex = UINT32_MAX;
};
