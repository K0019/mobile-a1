#include "animation_update.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "resource/animation_clip.h"
#include "resource/processed_assets.h"
#include "resource/resource_manager.h"
#include "resource/skeleton.h"

namespace
{
  glm::mat4 composeTRS(const glm::vec3& translation,
                       const glm::quat& rotation,
                       const glm::vec3& scale)
  {
    const glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
    const glm::mat4 r = glm::mat4_cast(rotation);
    const glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
  }

  float advanceTime(float current,
                    float delta,
                    float duration,
                    bool loop)
  {
    if(duration <= 0.0f)
      return 0.0f;

    float next = current + delta;
    if(loop)
    {
      next = std::fmod(next, duration);
      if(next < 0.0f)
        next += duration;
      return next;
    }
    return std::clamp(next, 0.0f, duration);
  }

  const Resource::ProcessedMorphChannel* findMorphChannel(const Resource::AnimationClip& clip,
                                                          uint32_t meshIndex,
                                                          const std::string& meshName,
                                                          const std::string& objectName)
  {
    const auto& channels = clip.morphChannels();
    if(channels.empty())
      return nullptr;

    // Prefer exact mesh index match when available
    for(const auto& channel : channels)
    {
      if(channel.meshIndex != UINT32_MAX && channel.meshIndex == meshIndex)
        return &channel;
    }

    // Fallback to mesh name
    for(const auto& channel : channels)
    {
      if(!channel.meshName.empty() && channel.meshName == meshName)
        return &channel;
    }

    // Last resort: try matching against scene object name
    for(const auto& channel : channels)
    {
      if(channel.meshName == objectName)
        return &channel;
    }

    return nullptr;
  }

  void applyMorphKey(const Resource::ProcessedMorphKey& key,
                     float scale,
                     std::vector<float>& weights)
  {
    if(scale <= 0.0f)
      return;

    for(size_t i = 0; i < key.targetIndices.size(); ++i)
    {
      const uint32_t target = key.targetIndices[i];
      if(target < weights.size())
        weights[target] += key.weights[i] * scale;
    }
  }

  void accumulateMorphWeights(const Resource::AnimationClip& clip,
                              const SceneObject& object,
                              float time,
                              float clipWeight,
                              std::vector<float>& weights,
                              const Resource::ResourceManager& rm)
  {
    if(clipWeight <= 0.0f || weights.empty())
      return;

    const auto* meshMetadata = rm.getMeshMetadata(object.mesh);
    const std::string meshName = meshMetadata ? meshMetadata->meshName : std::string{};

    const auto* channel = findMorphChannel(clip,
                                           object.getSourceMeshIndex(),
                                           meshName,
                                           object.name);
    if(!channel || channel->keys.empty())
      return;

    if(channel->keys.size() == 1)
    {
      applyMorphKey(channel->keys.front(), clipWeight, weights);
      return;
    }

    size_t keyIndex = 0;
    while(keyIndex + 1 < channel->keys.size() &&
          time > channel->keys[keyIndex + 1].time)
    {
      ++keyIndex;
    }

    const size_t nextIndex = std::min(keyIndex + 1, channel->keys.size() - 1);
    const auto& keyA = channel->keys[keyIndex];
    const auto& keyB = channel->keys[nextIndex];
    const float span = keyB.time - keyA.time;
    const float factor = span > 0.0f
      ? std::clamp((time - keyA.time) / span, 0.0f, 1.0f)
      : 0.0f;

    applyMorphKey(keyA, clipWeight * (1.0f - factor), weights);
    applyMorphKey(keyB, clipWeight * factor, weights);
  }
} // namespace

namespace Animation
{
  void Animate(const Resource::ResourceManager& rm,
               SceneObject& obj,
               float dt)
  {
    if(obj.type != SceneObjectType::Mesh || !obj.mesh.isValid() || !obj.anim)
      return;

    SceneObject::AnimBinding& anim = *obj.anim;
    if((anim.flags & SceneObject::AnimBinding::Playing) == 0)
      return;

    const bool crossfade = (anim.flags & SceneObject::AnimBinding::Crossfade) != 0;
    const Resource::AnimationClip* clipA = anim.clipA != Resource::INVALID_CLIP_ID
      ? &rm.Clip(anim.clipA)
      : nullptr;
    const Resource::AnimationClip* clipB = crossfade && anim.clipB != Resource::INVALID_CLIP_ID
      ? &rm.Clip(anim.clipB)
      : nullptr;

    // Advance time for both clips
    if(clipA)
    {
      anim.timeA = advanceTime(anim.timeA,
                               dt * anim.speed,
                               clipA->duration(),
                               (anim.flags & SceneObject::AnimBinding::Loop) != 0);
    }
    if(clipB)
    {
      anim.timeB = advanceTime(anim.timeB,
                               dt * anim.speed,
                               clipB->duration(),
                               (anim.flags & SceneObject::AnimBinding::Loop) != 0);
    }

    // --- Skinning Update ---
    if(anim.jointCount > 0)
    {
      // Ensure output buffer is correctly sized for the mesh's joint palette
      if(anim.skinMatrices.size() != anim.jointCount)
        anim.skinMatrices.resize(anim.jointCount);

      // Fast path for rest pose: if no clip is active, fill with identity matrices.
      if(!clipA)
      {
        std::fill(anim.skinMatrices.begin(), anim.skinMatrices.end(), glm::mat4(1.0f));
      }
      else
      {
        const Resource::Skeleton& skeleton = rm.Skeleton(anim.skeleton);
        const uint32_t skeletonJointCount = skeleton.jointCount();
        const auto parents = skeleton.parentIndices();
        const auto& topoOrder = skeleton.topoOrder();

        // 1. First pass: compute global transforms in SKELETON space.
        std::vector<glm::mat4> globalsSkel(skeletonJointCount);

        const float blend = clipB ? std::clamp(anim.blend, 0.0f, 1.0f) : 0.0f;

        for(const uint32_t jSkel : topoOrder)
        {
          const std::string& jointName = skeleton.jointName(jSkel);
          
          // Sample pose from clip A, or use bind pose if channel is missing
          const int32_t channelIdxA = clipA->channelIndex(jointName);
          auto pose = (channelIdxA >= 0)
            ? clipA->sampleChannel(channelIdxA, anim.timeA)
            : skeleton.bindPoseTRS(jSkel);

          if(clipB && blend > 0.0f)
          {
            const int32_t channelIdxB = clipB->channelIndex(jointName);
            auto poseB = (channelIdxB >= 0)
                ? clipB->sampleChannel(channelIdxB, anim.timeB)
                : skeleton.bindPoseTRS(jSkel);
            
            pose.translation = glm::mix(pose.translation, poseB.translation, blend);
            pose.scale = glm::mix(pose.scale, poseB.scale, blend);
            pose.rotation = glm::normalize(glm::slerp(pose.rotation, poseB.rotation, blend));
          }

          const glm::mat4 local = composeTRS(pose.translation, pose.rotation, pose.scale);
          const int parentIdx = parents[jSkel];
          
          globalsSkel[jSkel] = (parentIdx >= 0)
            ? globalsSkel[parentIdx] * local
            : local;
        }

        // 2. Second pass: produce final skinning matrices in MESH space.
        for(uint16_t jMesh = 0; jMesh < anim.jointCount; ++jMesh)
        {
          const int16_t jSkel = anim.jointRemap[jMesh];
          if(jSkel < 0)
          {
            // This mesh joint is not influenced by the skeleton.
            anim.skinMatrices[jMesh] = glm::mat4(1.0f);
          }
          else
          {
            // Transform from bind space to current animated space.
            anim.skinMatrices[jMesh] = globalsSkel[jSkel] * anim.invBindMatrices[jMesh];
          }
        }
      }
    }

    // --- Morph Target Update ---
    if(anim.morphCount > 0)
    {
      if(anim.morphWeights.size() != anim.morphCount)
        anim.morphWeights.assign(anim.morphCount, 0.0f);

      std::fill(anim.morphWeights.begin(), anim.morphWeights.end(), 0.0f);
      if(clipA)
      {
        const float blend = clipB ? std::clamp(anim.blend, 0.0f, 1.0f) : 0.0f;
        const float primaryWeight = clipB ? (1.0f - blend) : 1.0f;
        accumulateMorphWeights(*clipA, obj, anim.timeA, primaryWeight, anim.morphWeights, rm);
        if(clipB && blend > 0.0f)
          accumulateMorphWeights(*clipB, obj, anim.timeB, blend, anim.morphWeights, rm);
      }
    }
  }
} // namespace Animation
