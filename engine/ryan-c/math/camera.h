#pragma once

#include <assert.h>
#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"
using namespace glm;

class CameraPositionerInterface
{
  public:
    virtual ~CameraPositionerInterface() = default;

    virtual mat4 getViewMatrix() const = 0;

    virtual vec3 getPosition() const = 0;
};

class Camera final
{
  public:
    explicit Camera(const CameraPositionerInterface& positioner) : positioner_(&positioner), proj_() {}

    Camera(const Camera&) = default;

    Camera& operator =(const Camera&) = default;

    mat4 getViewMatrix() const { return positioner_->getViewMatrix(); }

    vec3 getPosition() const { return positioner_->getPosition(); }

    mat4 getProjMatrix() const { return proj_; }

    mat4 setProjMatrix(const mat4& proj)
    {
      proj_ = proj;
      return proj_;
    }

  private:
    const CameraPositionerInterface* positioner_;
    mat4 proj_;
};

class CameraPositioner_FirstPerson final : public CameraPositionerInterface
{
  public:
    //
    CameraPositioner_FirstPerson() : up_(0.0f, 1.0f, 0.0f)
    {
      lookAt(vec3(0.0f, 0.0f, 10.0f), vec3(0.0f, 0.0f, 9.0f), up_);
    }

    CameraPositioner_FirstPerson(const vec3& pos, const vec3& target, const vec3& up) : up_(normalize(up))
    {
      if (length(up) < glm::epsilon<float>()) { up_ = vec3(0.0f, 1.0f, 0.0f); }
      lookAt(pos, target, up_);
    }

    void update(float dt, const vec2& delta, bool mousePressed)
    {
      if (mousePressed)
      {
        if (length(delta) > glm::epsilon<float>())
        {
          yaw_ += mouseSpeed_ * delta.x;
          pitch_ -= mouseSpeed_ * delta.y;

          constexpr float pitchLimit = glm::pi<float>() / 2.0f - 0.01f; // Approx +/- 89.9 degrees
          pitch_ = std::max(-pitchLimit, std::min(pitchLimit, pitch_));
          yaw_ = std::fmod(yaw_, 2.0f * glm::pi<float>());
          if (yaw_ < 0.0f) { yaw_ += 2.0f * glm::pi<float>(); }
        }
      }
      vec3 forward;
      forward.x = cos(pitch_) * sin(yaw_);
      forward.y = sin(pitch_);
      forward.z = cos(pitch_) * cos(yaw_);
      forward = normalize(forward);
      vec3 right = normalize(cross(forward, up_));
      vec3 localUp = normalize(cross(right, forward));
      vec3 accel(0.0f);

      if (movement_.forward_)
        accel += forward;
      if (movement_.backward_)
        accel -= forward;
      if (movement_.left_)
        accel -= right;
      if (movement_.right_)
        accel += right;
      if (movement_.up_)
        accel += localUp;
      if (movement_.down_)
        accel -= localUp;

      if (movement_.fastSpeed_)
        accel *= fastCoef_;

      // Apply damping or acceleration
      if (length(accel) < glm::epsilon<float>()) { moveSpeed_ -= moveSpeed_ * std::min((1.0f / damping_) * dt, 1.0f); }
      else
      {
        accel = normalize(accel);
        moveSpeed_ += accel * acceleration_ * dt;
        const float maxSpeed = movement_.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
        if (length(moveSpeed_) > maxSpeed) { moveSpeed_ = normalize(moveSpeed_) * maxSpeed; }
      }
      cameraPosition_ += moveSpeed_ * dt;
    }

    mat4 getViewMatrix() const override
    {
      vec3 forward;
      forward.x = cos(pitch_) * sin(yaw_);
      forward.y = sin(pitch_);
      forward.z = cos(pitch_) * cos(yaw_);

      vec3 target = cameraPosition_ + forward;
      return glm::lookAt(cameraPosition_, target, up_);
    }

    vec3 getPosition() const override { return cameraPosition_; }

    void setPosition(const vec3& pos) { cameraPosition_ = pos; }

    vec3 getAnglesDeg() const { return {glm::degrees(pitch_), glm::degrees(yaw_), 0.0f}; }

    float getPitchRad() const { return pitch_; }

    float getYawRad() const { return yaw_; }

    void setAnglesRad(float pitchRad, float yawRad)
    {
      constexpr float pitchLimit = glm::pi<float>() / 2.0f - 0.01f;
      pitch_ = std::max(-pitchLimit, std::min(pitchLimit, pitchRad));
      yaw_ = std::fmod(yawRad, 2.0f * glm::pi<float>());
      if (yaw_ < 0.0f) { yaw_ += 2.0f * glm::pi<float>(); }
    }

    void setSpeed(const vec3& speed) { moveSpeed_ = speed; }

    void lookAt(const vec3& pos, const vec3& target, const vec3& up)
    {
      cameraPosition_ = pos;
      up_ = normalize(up);
      if (length(up) < glm::epsilon<float>()) { up_ = vec3(0.0f, 1.0f, 0.0f); }
      vec3 dir = normalize(target - pos);
      yaw_ = atan2(dir.x, dir.z);
      pitch_ = asin(dir.y);
      constexpr float pitchLimit = glm::pi<float>() / 2.0f - 0.01f;
      pitch_ = std::max(-pitchLimit, std::min(pitchLimit, pitch_));
    }

    // Movement state struct
    struct Movement
    {
      bool forward_ = false;
      bool backward_ = false;
      bool left_ = false;
      bool right_ = false;
      bool up_ = false;
      bool down_ = false;
      bool fastSpeed_ = false;
    } movement_;

    // Camera parameters
    float mouseSpeed_ = 0.005f; // Adjust this value based on testing!
    float acceleration_ = 150.0f;
    float damping_ = 0.2f;
    float maxSpeed_ = 10.0f;
    float fastCoef_ = 10.0f;

  private:
    vec3 cameraPosition_ = vec3(0.0f);
    vec3 moveSpeed_ = vec3(0.0f);

    // Euler angles (in radians)
    float pitch_ = 0.0f; // Rotation around local X axis
    float yaw_ = 0.0f;   // Rotation around world Z axis
    // World Up vector
    vec3 up_ = vec3(0.0f, 1.0f, 0.0f);
};

class CameraPositioner_MoveTo final : public CameraPositionerInterface
{
  public:
    CameraPositioner_MoveTo(const vec3& pos, const vec3& angles) : positionCurrent_(pos), positionDesired_(pos), anglesCurrent_(angles), anglesDesired_(angles) {}

    void update(float deltaSeconds, [[maybe_unused]] const vec2& mousePos = {}, [[maybe_unused]] bool mousePressed = {})
    {
      positionCurrent_ += dampingLinear_ * deltaSeconds * (positionDesired_ - positionCurrent_);

      // normalization is required to avoid "spinning" around the object 2pi times
      anglesCurrent_ = clipAngles(anglesCurrent_);
      anglesDesired_ = clipAngles(anglesDesired_);

      // update angles
      anglesCurrent_ -= angleDelta(anglesCurrent_, anglesDesired_) * dampingEulerAngles_ * deltaSeconds;

      // normalize new angles
      anglesCurrent_ = clipAngles(anglesCurrent_);

      const vec3 a = radians(anglesCurrent_);

      currentTransform_ = translate(glm::yawPitchRoll(a.y, a.x, a.z), -positionCurrent_);
    }

    void setPosition(const vec3& p) { positionCurrent_ = p; }

    void setAngles(float pitch, float pan, float roll) { anglesCurrent_ = vec3(pitch, pan, roll); }

    void setAngles(const vec3& angles) { anglesCurrent_ = angles; }

    void setDesiredPosition(const vec3& p) { positionDesired_ = p; }

    void setDesiredAngles(float pitch, float pan, float roll) { anglesDesired_ = vec3(pitch, pan, roll); }

    void setDesiredAngles(const vec3& angles) { anglesDesired_ = angles; }

    vec3 getPosition() const override { return positionCurrent_; }

    mat4 getViewMatrix() const override { return currentTransform_; }

  public:
    float dampingLinear_ = 10.0f;
    vec3 dampingEulerAngles_ = vec3(5.0f, 5.0f, 5.0f);

  private:
    vec3 positionCurrent_ = vec3(0.0f);
    vec3 positionDesired_ = vec3(0.0f);

    /// pitch, pan, roll
    vec3 anglesCurrent_ = vec3(0.0f);
    vec3 anglesDesired_ = vec3(0.0f);

    mat4 currentTransform_ = mat4(1.0f);

    static float clipAngle(float d)
    {
      if (d < -180.0f)
        return d + 360.0f;
      if (d > +180.0f)
        return d - 360.f;
      return d;
    }

    static vec3 clipAngles(const vec3& angles)
    {
      return {std::fmod(angles.x, 360.0f), std::fmod(angles.y, 360.0f), std::fmod(angles.z, 360.0f)};
    }

    static vec3 angleDelta(const vec3& anglesCurrent, const vec3& anglesDesired)
    {
      const vec3 d = clipAngles(anglesCurrent) - clipAngles(anglesDesired);
      return {clipAngle(d.x), clipAngle(d.y), clipAngle(d.z)};
    }
};
