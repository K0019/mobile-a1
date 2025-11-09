#pragma once
#include "math/utils_math.h"

struct FrameData
{
  uint64_t frameNumber = 0;
  mat4 viewMatrix;
  mat4 projMatrix;
  vec3 cameraPos;
  float deltaTime = 0.0f;
  uint32_t screenWidth = 1;
  uint32_t screenHeight = 1;
  uint32_t depth = 1;

  float zNear = 0.1f;
  float zFar = 1000.0f;
  float fovY = 45.0f;
};