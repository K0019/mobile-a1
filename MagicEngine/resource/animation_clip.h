#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/gtc/quaternion.hpp>

#include "math/utils_math.h"
#include "resource/animation_ids.h"

namespace Resource
{
  struct ProcessedAnimationClip;
  struct ProcessedMorphChannel;

  struct AnimationVectorKey
  {
    float time = 0.0f;
    vec3 value{ 0.0f };
  };

  struct AnimationQuatKey
  {
    float time = 0.0f;
    glm::quat value{ 1.0f, 0.0f, 0.0f, 0.0f };
  };

  class AnimationClip
  {
    public:
      struct JointSample
      {
        vec3 translation{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        vec3 scale{ 1.0f };
      };

      AnimationClip() = default;
      explicit AnimationClip(const ProcessedAnimationClip& clip);

      const std::string& name() const { return m_name; }
      float duration() const { return m_duration; }
      float ticksPerSecond() const { return m_ticksPerSecond; }

      int32_t channelIndex(const std::string& jointName) const;
      JointSample sampleChannel(int32_t channelIndex, float time) const;

      const std::vector<ProcessedMorphChannel>& morphChannels() const { return m_morphChannels; }

    private:
      struct Channel
      {
        std::string jointName;
        std::vector<AnimationVectorKey> translationKeys;
        std::vector<AnimationQuatKey> rotationKeys;
        std::vector<AnimationVectorKey> scaleKeys;
      };

      std::string m_name;
      float m_duration = 0.0f;
      float m_ticksPerSecond = 1.0f;
      std::vector<Channel> m_channels;
      std::unordered_map<std::string, size_t> m_channelLookup;
      std::vector<ProcessedMorphChannel> m_morphChannels;
  };
} // namespace Resource

