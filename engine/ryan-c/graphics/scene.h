#pragma once
#include "assets/core/asset_types.h"

namespace AssetLoading
{
  class SceneLoader;
  class AssetSystem;
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

  bool isRenderable() const { return type == SceneObjectType::Mesh && mesh.isValid() && material.isValid(); }

  private:
    // Temporary loading indices - only accessible to SceneLoader
    friend class AssetLoading::SceneLoader;
    uint32_t meshIndex = UINT32_MAX;
    uint32_t materialIndex = UINT32_MAX;
};
