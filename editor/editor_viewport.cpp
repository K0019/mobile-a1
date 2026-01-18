#include "editor_viewport.h"
#include <glm/gtc/matrix_transform.hpp>

namespace editor
{
  void EditorViewport::configure(const Camera& camera, uint32_t width, uint32_t height, const ViewportConfig& config)
  {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
    fovY_ = config.fovY;
    zNear_ = config.zNear;
    zFar_ = config.zFar;

    cameraPos_ = camera.getPosition();
    viewMatrix_ = camera.getViewMatrix();
    aspectRatio_ = static_cast<float>(width_) / static_cast<float>(height_);
    projMatrix_ = glm::perspective(glm::radians(fovY_), aspectRatio_, zNear_, zFar_);
  }

  void EditorViewport::applyToView(FrameData& view, FeatureMask featureMask) const
  {
    view.cameraPos = cameraPos_;
    view.viewMatrix = viewMatrix_;
    view.projMatrix = projMatrix_;
    view.zNear = zNear_;
    view.zFar = zFar_;
    view.fovY = fovY_;
    view.viewportWidth = static_cast<float>(width_);
    view.viewportHeight = static_cast<float>(height_);
    view.featureMask = featureMask;
  }

} // namespace editor
