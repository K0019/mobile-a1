#pragma once
#include "renderer/scene.h"

namespace Resource
{
  class ResourceManager;
}

namespace Animation
{
  void Animate(const Resource::ResourceManager& resources, SceneObject& object, float deltaTime);
}
