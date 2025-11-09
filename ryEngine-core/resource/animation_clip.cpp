#include "resource/animation_clip.h"

#include <algorithm>

#include <glm/gtx/quaternion.hpp>

#include "resource/processed_assets.h"

namespace Resource
{
  namespace
  {
    template <typename KeyContainer, typename ValueType>
    ValueType sampleVectorKeys(const KeyContainer& keys, float time, const ValueType& defaultValue)
    {
      if(keys.empty())
        return defaultValue;

      if(keys.size() == 1)
        return keys.front().value;

      if(time <= keys.front().time)
        return keys.front().value;

      if(time >= keys.back().time)
        return keys.back().value;

      for(size_t i = 0; i + 1 < keys.size(); ++i)
      {
        const auto& a = keys[i];
        const auto& b = keys[i + 1];
        if(time >= a.time && time <= b.time)
        {
          const float span = b.time - a.time;
          const float factor = span > 0.0f
            ? std::clamp((time - a.time) / span, 0.0f, 1.0f)
            : 0.0f;
          return a.value * (1.0f - factor) + b.value * factor;
        }
      }

      return keys.back().value;
    }

    glm::quat sampleQuatKeys(const std::vector<AnimationQuatKey>& keys, float time)
    {
      if(keys.empty())
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

      if(keys.size() == 1)
        return glm::normalize(keys.front().value);

      if(time <= keys.front().time)
        return glm::normalize(keys.front().value);

      if(time >= keys.back().time)
        return glm::normalize(keys.back().value);

      for(size_t i = 0; i + 1 < keys.size(); ++i)
      {
        const auto& a = keys[i];
        const auto& b = keys[i + 1];
        if(time >= a.time && time <= b.time)
        {
          const float span = b.time - a.time;
          const float factor = span > 0.0f
            ? std::clamp((time - a.time) / span, 0.0f, 1.0f)
            : 0.0f;
          return glm::normalize(glm::slerp(a.value, b.value, factor));
        }
      }

      return glm::normalize(keys.back().value);
    }
  } // namespace

  AnimationClip::AnimationClip(const ProcessedAnimationClip& clip)
    : m_name(clip.name),
      m_duration(clip.duration),
      m_ticksPerSecond(clip.ticksPerSecond > 0.0f ? clip.ticksPerSecond : 1.0f),
      m_morphChannels(clip.morphChannels)
  {
    m_channels.reserve(clip.skeletalChannels.size());
    m_channelLookup.reserve(clip.skeletalChannels.size());

    for(const auto& processedChannel : clip.skeletalChannels)
    {
      Channel channel;
      channel.jointName = processedChannel.nodeName;
      channel.translationKeys.reserve(processedChannel.translationKeys.size());
      for(const auto& key : processedChannel.translationKeys)
      {
        channel.translationKeys.push_back({ key.time, key.value });
      }

      channel.rotationKeys.reserve(processedChannel.rotationKeys.size());
      for(const auto& key : processedChannel.rotationKeys)
      {
        channel.rotationKeys.push_back({ key.time, key.value });
      }

      channel.scaleKeys.reserve(processedChannel.scaleKeys.size());
      for(const auto& key : processedChannel.scaleKeys)
      {
        channel.scaleKeys.push_back({ key.time, key.value });
      }

      m_channels.push_back(std::move(channel));
      m_channelLookup[m_channels.back().jointName] = m_channels.size() - 1;
    }
  }

  int32_t AnimationClip::channelIndex(const std::string& jointName) const
  {
    auto it = m_channelLookup.find(jointName);
    if(it == m_channelLookup.end())
      return -1;
    return static_cast<int32_t>(it->second);
  }

  AnimationClip::JointSample AnimationClip::sampleChannel(int32_t channelIndex, float time) const
  {
    JointSample result{};
    result.translation = vec3(0.0f);
    result.scale = vec3(1.0f);

    if(channelIndex < 0 || static_cast<size_t>(channelIndex) >= m_channels.size())
      return result;

    const Channel& channel = m_channels[static_cast<size_t>(channelIndex)];
    result.translation = sampleVectorKeys(channel.translationKeys, time, vec3(0.0f));
    result.rotation = sampleQuatKeys(channel.rotationKeys, time);
    result.scale = sampleVectorKeys(channel.scaleKeys, time, vec3(1.0f));
    return result;
  }
} // namespace Resource

