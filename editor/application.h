#pragma once
#include <assets/io/scene_loader.h>
#include <base/imgui_context.h>
#include <math/camera.h>
#include "graphics/scene.h"

class Application
{
  public:
  void Initialize(Context& context);
  void Update(Context& context, FrameData& frame);
  void Shutdown(Context& context);

  CameraPositioner_FirstPerson positioner_ = {vec3(0.0f, 1.0f, -1.5f), vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f)};
  Camera camera = Camera(positioner_);

  std::unique_ptr<Resource::SceneLoader> sceneLoader_;
  std::unique_ptr<editor::ImGuiContext> imguiContext_;
  uint64_t gridFeatureHandle_ = 0;  // Handle for the GridFeature, if created
  uint64_t sceneFeatureHandle_ = 0; // Handle for the SceneFeature, if created
  Resource::Scene loadedScene_;
  uint32_t irradianceMap = 0;
  uint32_t prefilteredEnvMap = 0;
  uint32_t brdfLUT = 0;
};
