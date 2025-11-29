#include "VFS/VFS.h"
#include "Graphics/RenderComponent.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Containers/GUICollection.h"
#include "Graphics/AnimationComponent.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"


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

    const Resource::ProcessedMorphChannel* findMorphChannel(const Resource::AnimationClip& clip,
        uint32_t meshIndex,
        const std::string& meshName,
        const std::string& objectName)
    {
        const auto& channels = clip.morphChannels();
        if (channels.empty())
            return nullptr;

        // Prefer exact mesh index match when available
        for (const auto& channel : channels)
        {
            if (channel.meshIndex != UINT32_MAX && channel.meshIndex == meshIndex)
                return &channel;
        }

        // Fallback to mesh name
        for (const auto& channel : channels)
        {
            if (!channel.meshName.empty() && channel.meshName == meshName)
                return &channel;
        }

        // Last resort: try matching against scene object name
        for (const auto& channel : channels)
        {
            if (channel.meshName == objectName)
                return &channel;
        }

        return nullptr;
    }

    void applyMorphKey(const Resource::ProcessedMorphKey& key,
        float scale,
        std::vector<float>& weights)
    {
        if (scale <= 0.0f)
            return;

        for (size_t i = 0; i < key.targetIndices.size(); ++i)
        {
            const uint32_t target = key.targetIndices[i];
            if (target < weights.size())
                weights[target] += key.weights[i] * scale;
        }
    }

    void accumulateMorphWeights(
        const Resource::AnimationClip& clip,
        float time,
        float clipWeight,
        const Resource::ResourceManager& rm,
        const RenderComponent& renderComp,  // Replaces object.mesh
        const std::string& objectName,      // Replaces object.name
        std::vector<float>& weights)        // Replaces anim.morphWeights
    {
        if (clipWeight <= 0.0f || weights.empty())
            return;

        // object.getSourceMeshIndex() used for -> findMorphChannel(.meshIndex)
        // oriignal: for (uint32_t i = 0; i < node->mNumMeshes; ++i) -> meshIndex = node->mMeshes[i]; -> object.meshIndex = meshIndex;
        
        const ResourceMesh* meshAsset = renderComp.GetMesh();
        if (!meshAsset) return;

        for (uint32_t subMeshIndex = 0; subMeshIndex < meshAsset->handles.size(); ++subMeshIndex)
        {
            const auto* meshMetadata = rm.getMeshMetadata(meshAsset->handles[subMeshIndex]);
            const std::string meshName = meshMetadata ? meshMetadata->meshName : std::string{};

            const auto* channel = findMorphChannel(clip, subMeshIndex, meshName, objectName);
            if (!channel || channel->keys.empty())
                continue; 

            if (channel->keys.size() == 1)
            {
                applyMorphKey(channel->keys.front(), clipWeight, weights);
                return;
            }

            size_t keyIndex = 0;
            while (keyIndex + 1 < channel->keys.size() &&
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
    }

    float advanceTime(float current, float delta, float duration, bool loop)
    {
        if (duration <= 0.0f)
            return 0.0f;

        float next = current + delta;
        if (loop)
        {
            next = std::fmod(next, duration);
            if (next < 0.0f)
                next += duration;
            return next;
        }
        return std::clamp(next, 0.0f, duration);
    }

}

void AnimationComponent::SetupAnimationBinding()
{
    RenderComponent* renderComp = ecs::GetEntity(this)->GetComp<RenderComponent>();

    if (!renderComp || !renderComp->GetMesh())
        return;

    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    const auto* mesh = renderComp->GetMesh();
    const auto* meshCold = graphicsAssetSystem.getMeshMetadata(mesh->handles[0]);
    if (!meshCold)
        return;

    if (meshCold->skeletonId != Resource::INVALID_SKELETON_ID)
    {
        // Store mesh-specific data required for correct skinning
        jointCount = static_cast<uint16_t>(meshCold->jointNames.size());
        invBindMatrices = meshCold->jointInverseBindMatrices;
        skinMatrices.resize(jointCount);
        jointRemap.resize(jointCount);

        // Calculate the remap table
        const auto& skeleton = graphicsAssetSystem.Skeleton(meshCold->skeletonId);
        if (skeleton.jointCount() == 0) return;

        // Build the remap table: mesh joint index -> skeleton joint index
        for (uint16_t jMesh = 0; jMesh < jointCount; ++jMesh)
        {
            const std::string& meshJointName = meshCold->jointNames[jMesh];
            const int16_t jSkel = skeleton.indexOfJoint(meshJointName);
            jointRemap[jMesh] = jSkel;
        }
    }

    if (meshCold->morphSetId != Resource::INVALID_MORPH_SET_ID)
    {
        const auto& morph = graphicsAssetSystem.Morph(meshCold->morphSetId);
        morphCount = morph.count();
        morphWeights.assign(morph.count(), 0.0f);
    }
    else
    {
        morphCount = 0;
        morphWeights.clear();
    }
}

const ResourceAnimation* AnimationComponent::GetAnimationClipA() const
{
    return animHandleA.GetResource();
}
const ResourceAnimation* AnimationComponent::GetAnimationClipB() const
{
    return animHandleB.GetResource();
}

void AnimationComponent::TransitionTo(size_t newAnimHash, float duration)
{
    // Don't transition to the same animation
    if (animHandleA.GetHash() == newAnimHash)
        return;

    // Store current animation as B (the "from" animation)
    animHandleB = animHandleA.GetHash();
    timeB = timeA;

    // Set new animation as A (the "to" animation)
    animHandleA = newAnimHash;
    timeA = 0.0f;

    // Start transition
    isTransitioning = true;
    transitionDuration = duration;
    transitionTime = 0.0f;

    SetupAnimationBinding();
}


void AnimationComponent::EditorDraw()
{
    // Animation clip input
    const std::string* clip1Name{ ST<MagicResourceManager>::Get()->Editor_GetName(animHandleA.GetHash()) };
    gui::TextUnformatted("Animation");
    gui::SameLine();
    gui::TextBoxReadOnly("##AnimClip", clip1Name ? clip1Name->c_str() : "");
    gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
        TransitionTo(hash, transitionDuration);
        });

    gui::Separator();

    // Playback controls
    gui::TextUnformatted("Playing");
    gui::SameLine();
    gui::Checkbox("##playing", &isPlaying);

    gui::TextUnformatted("Looping");
    gui::SameLine();
    gui::Checkbox("##looping", &loop);

    gui::Separator();

    gui::TextUnformatted("Speed");
    gui::SameLine();
    gui::VarDrag("##playbackspeed", &speed, 0.01f, 0.0f, 10.0f);

    gui::TextUnformatted("Transition");
    gui::SameLine();
    gui::VarDrag("##transitionduration", &transitionDuration, 0.01f, 0.0f, 2.0f);

    gui::TextUnformatted("Time");
    gui::SameLine();
    float duration = GetClipDuration(GetAnimationClipA());
    if (duration > 0.0f)
        gui::Slider("##timeA", &timeA, 0.0f, duration);
    else
        gui::VarDrag("##timeA", &timeA, 0.01f, 0.0f, 0.0f);

    // Show transition state
    if (isTransitioning)
    {
        gui::TextDisabled("Transitioning: %.0f%%", (transitionTime / transitionDuration) * 100.0f);
    }
}

float AnimationComponent::GetClipDuration(const ResourceAnimation* animationClip)
{
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };

    const Resource::AnimationClip* clip = animationClip && animationClip->handle != Resource::INVALID_CLIP_ID
        ? &graphicsAssetSystem.Clip(animationClip->handle) : nullptr;

    if (clip)
        return clip->duration();
    return -1.0f;
}



AnimationSystem::AnimationSystem() :
    System_Internal{ &AnimationSystem::ProcessComp }
{}


void AnimationSystem::ProcessComp(AnimationComponent & comp)
{
    if (!comp.isPlaying || !comp.GetAnimationClipA())
        return;

    const ResourceAnimation* clip = comp.GetAnimationClipA();
    if (!clip)
        return;


    RenderComponent* renderComp = ecs::GetEntity(&comp)->GetComp<RenderComponent>();
    if (!renderComp)
        return;

    const ResourceMesh* mesh = renderComp->GetMesh();
    if (!mesh || mesh->handles.empty())
        return;

    const float dt = GameTime::Dt();
    
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    const Resource::AnimationClip* clipA = comp.GetAnimationClipA() && comp.GetAnimationClipA()->handle != Resource::INVALID_CLIP_ID
        ? &graphicsAssetSystem.Clip(comp.GetAnimationClipA()->handle) : nullptr;
    const Resource::AnimationClip* clipB = comp.isTransitioning && comp.GetAnimationClipB() && comp.GetAnimationClipB()->handle != Resource::INVALID_CLIP_ID
        ? &graphicsAssetSystem.Clip(comp.GetAnimationClipB()->handle) : nullptr;

    // Update transition
    float transitionBlend = 0.0f;
    if (comp.isTransitioning)
    {
        comp.transitionTime += dt;
        if (comp.transitionTime >= comp.transitionDuration)
        {
            // Transition complete
            comp.isTransitioning = false;
            comp.transitionTime = 0.0f;
            transitionBlend = 1.0f;
        }
        else
        {
            // Smooth interpolation (ease in-out)
            float t = comp.transitionTime / comp.transitionDuration;
            transitionBlend = t * t * (3.0f - 2.0f * t); // smoothstep
        }
    }

    if (comp.isPlaying)
    {
        if (clipA)
        {
            comp.timeA = advanceTime(comp.timeA, dt * comp.speed, clipA->duration(), comp.loop);
        }
        // During transition, also advance the old animation (clipB) so it doesn't freeze
        if (clipB && comp.isTransitioning)
        {
            comp.timeB = advanceTime(comp.timeB, dt * comp.speed, clipB->duration(), comp.loop);
        }
    }

    const auto* meshMetadata = graphicsAssetSystem.getMeshMetadata(mesh->handles[0]);
    if (!meshMetadata)
        return;

    // --- Skinning Update ---
    const auto& skeleton = graphicsAssetSystem.Skeleton(meshMetadata->skeletonId);
    if (skeleton.jointCount() > 0)
    {
        uint32_t jointCount = skeleton.jointCount();
        // Ensure output buffer is correctly sized for the mesh's joint palette
        if (comp.skinMatrices.size() != jointCount)
        {
            comp.SetupAnimationBinding();
        }

        // Fast path for rest pose: if no clip is active, fill with identity matrices.
        if (!clipA)
        {
            std::fill(comp.skinMatrices.begin(), comp.skinMatrices.end(), glm::mat4(1.0f));
        }
        else
        {
            const uint32_t skeletonJointCount = skeleton.jointCount();
            const auto parents = skeleton.parentIndices();
            const auto& topoOrder = skeleton.topoOrder();

            // 1. First pass: compute global transforms in SKELETON space.
            std::vector<glm::mat4> globalsSkel(skeletonJointCount);

            // During transition: blend from clipB (old) to clipA (new)
            const float blend = transitionBlend;

            for (const uint32_t jSkel : topoOrder)
            {
                const std::string& jointName = skeleton.jointName(jSkel);

                // Sample pose from clip A (the new/target animation)
                const int32_t channelIdxA = clipA->channelIndex(jointName);
                auto poseA = (channelIdxA >= 0)
                    ? clipA->sampleChannel(channelIdxA, comp.timeA)
                    : skeleton.bindPoseTRS(jSkel);

                // During transition, blend from old pose (B) to new pose (A)
                auto pose = poseA;
                if (clipB && blend < 1.0f)
                {
                    const int32_t channelIdxB = clipB->channelIndex(jointName);
                    auto poseB = (channelIdxB >= 0)
                        ? clipB->sampleChannel(channelIdxB, comp.timeB)
                        : skeleton.bindPoseTRS(jSkel);

                    // blend=0 means 100% B (old), blend=1 means 100% A (new)
                    pose.translation = glm::mix(poseB.translation, poseA.translation, blend);
                    pose.scale = glm::mix(poseB.scale, poseA.scale, blend);
                    pose.rotation = glm::normalize(glm::slerp(poseB.rotation, poseA.rotation, blend));
                }

                const glm::mat4 local = composeTRS(pose.translation, pose.rotation, pose.scale);
                const int parentIdx = parents[jSkel];

                globalsSkel[jSkel] = (parentIdx >= 0)
                    ? globalsSkel[parentIdx] * local
                    : local;
            }

            // 2. Second pass: produce final skinning matrices in MESH space.
            for (uint16_t jMesh = 0; jMesh < jointCount; ++jMesh)
            {
                const int16_t jSkel = comp.jointRemap[jMesh];
                if (jSkel < 0)
                {
                    // This mesh joint is not influenced by the skeleton.
                    comp.skinMatrices[jMesh] = glm::mat4(1.0f);
                }
                else
                {
                    // Transform from bind space to current animated space.
                    comp.skinMatrices[jMesh] = globalsSkel[jSkel] * comp.invBindMatrices[jMesh];
                }
            }

            // Store bone world transforms for bone attachment system
            comp.boneWorldTransforms = std::move(globalsSkel);
        }
    }

    // --- Morph Target Update ---
    const auto& morph = graphicsAssetSystem.Morph(meshMetadata->morphSetId);
    if (morph.count() > 0)
    {
        if (comp.morphWeights.size() != morph.count())
        {
            comp.SetupAnimationBinding();
        }

        std::fill(comp.morphWeights.begin(), comp.morphWeights.end(), 0.0f);
        if (clipA)
        {
            // During transition: blend from old (B) to new (A)
            const float blend = transitionBlend;
            const float newWeight = comp.isTransitioning ? blend : 1.0f;
            const float oldWeight = comp.isTransitioning ? (1.0f - blend) : 0.0f;

            accumulateMorphWeights(*clipA, comp.timeA, newWeight, graphicsAssetSystem, *renderComp, "", comp.morphWeights);
            if (clipB && oldWeight > 0.0f)
                accumulateMorphWeights(*clipB, comp.timeB, oldWeight, graphicsAssetSystem, *renderComp, "", comp.morphWeights);
        }
    }
}