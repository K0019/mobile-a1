#pragma once
#include "renderer/frame_data.h"
#include "math/camera.h"
#include <cstdint>

namespace editor
{
  // ============================================================================
  // ViewportConfig - Settings for viewport projection
  // ============================================================================

  struct ViewportConfig
  {
    float fovY = 45.0f;
    float zNear = 0.1f;
    float zFar = 1000.0f;
  };

  // ============================================================================
  // EditorViewport - Manages view configuration and camera-to-frame-data mapping
  // ============================================================================

  class EditorViewport
  {
  public:
    EditorViewport() = default;

    // Configure the viewport with camera and surface dimensions
    void configure(const Camera& camera, uint32_t width, uint32_t height, const ViewportConfig& config = {});

    // Apply this viewport's settings to a FrameData view
    void applyToView(FrameData& view, FeatureMask featureMask) const;

    // Accessors
    const vec3& getCameraPosition() const { return cameraPos_; }
    const mat4& getViewMatrix() const { return viewMatrix_; }
    const mat4& getProjMatrix() const { return projMatrix_; }
    float getAspectRatio() const { return aspectRatio_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

  private:
    vec3 cameraPos_{0.0f};
    mat4 viewMatrix_{1.0f};
    mat4 projMatrix_{1.0f};
    float aspectRatio_ = 16.0f / 9.0f;
    float fovY_ = 45.0f;
    float zNear_ = 0.1f;
    float zFar_ = 1000.0f;
    uint32_t width_ = 1;
    uint32_t height_ = 1;
  };

} // namespace editor
