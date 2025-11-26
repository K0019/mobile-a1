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


void AnimationComponent::EditorDraw()
{
    // Animation input
    const std::string* clip1Name{ ST<MagicResourceManager>::Get()->Editor_GetName(animHandleA.GetHash()) };
    gui::TextUnformatted(std::string("Clip 1"));
    gui::SameLine();
    gui::TextBoxReadOnly("##AnimClip1", clip1Name ? clip1Name->c_str() : "");
    gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
        animHandleA = hash;
        SetupAnimationBinding();
        });

    if (crossfade)
    {
        const std::string* clip2Name{ ST<MagicResourceManager>::Get()->Editor_GetName(animHandleB.GetHash()) };
        gui::TextUnformatted(std::string("Clip 2"));
        gui::SameLine();
        gui::TextBoxReadOnly("##AnimClip2", clip2Name ? clip2Name->c_str() : "");
        gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
            animHandleB = hash;
            SetupAnimationBinding();
            });

        gui::TextUnformatted(std::string("Blend"));
        gui::SameLine();
        gui::VarDrag("##blend", &blend);
    }

    gui::TextUnformatted(std::string("Is Playing"));
    gui::SameLine();
    gui::Checkbox("##playing", &isPlaying);

    gui::TextUnformatted(std::string("Looping"));
    gui::SameLine();
    gui::Checkbox("##looping", &loop);

    gui::TextUnformatted(std::string("Playback Speed"));
    gui::SameLine();
    gui::VarDrag("##playbackspeed", &speed);


    gui::TextUnformatted(std::string("Crossfade"));
    gui::SameLine();
    gui::Checkbox("##crossfade", &crossfade);
    

    gui::TextUnformatted(std::string("ClipA playback"));
    gui::SameLine();
    gui::VarDrag("##timeA", &timeA);
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


    ecs::EntityHandle entity = ecs::GetEntity(&comp);
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
    const Resource::AnimationClip* clipB = comp.crossfade && comp.GetAnimationClipB() && comp.GetAnimationClipB()->handle != Resource::INVALID_CLIP_ID
        ? &graphicsAssetSystem.Clip(comp.GetAnimationClipB()->handle) : nullptr;


    uint8_t flags = 0;
    if (comp.isPlaying)  flags |= 1;
    if (comp.loop)       flags |= 2;
    if (comp.crossfade)  flags |= 4;
    

    if (comp.isPlaying)
    {
        if (clipA)
        {
            comp.timeA = advanceTime(comp.timeA, dt * comp.speed, clipA->duration(), (flags & SceneObject::AnimBinding::Loop) != 0);
        }
        if (clipB)
        {
            comp.timeB = advanceTime(comp.timeB, dt * comp.speed, clipB->duration(), (flags & SceneObject::AnimBinding::Loop) != 0);
        }
    }

    //const auto* meshData = graphicsAssetSystem.getMesh(mesh->handles[0]);
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

            const float blend = clipB ? std::clamp(comp.blend, 0.0f, 1.0f) : 0.0f;

            for (const uint32_t jSkel : topoOrder)
            {
                const std::string& jointName = skeleton.jointName(jSkel);

                // Sample pose from clip A, or use bind pose if channel is missing
                const int32_t channelIdxA = clipA->channelIndex(jointName);
                auto pose = (channelIdxA >= 0)
                    ? clipA->sampleChannel(channelIdxA, comp.timeA)
                    : skeleton.bindPoseTRS(jSkel);

                if (clipB && blend > 0.0f)
                {
                    const int32_t channelIdxB = clipB->channelIndex(jointName);
                    auto poseB = (channelIdxB >= 0)
                        ? clipB->sampleChannel(channelIdxB, comp.timeB)
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
            const float blend = clipB ? std::clamp(comp.blend, 0.0f, 1.0f) : 0.0f;
            const float primaryWeight = clipB ? (1.0f - blend) : 1.0f;
            //accumulateMorphWeights(*clipA, obj, anim.timeA, primaryWeight, anim.morphWeights, rm);

            // object.name = node->mName.length > 0 ? node->mName.C_Str() : ("Object_" + std::to_string(objects.size()));
            // regarding the "" passed into accumulateMorphWeights, the above line is the ryan's original line of what it should be
            // but i dont have the concepts of nodes during runtime. its a fallback anyway. dont care.

            accumulateMorphWeights(*clipA, comp.timeA, primaryWeight, graphicsAssetSystem, *renderComp, "", comp.morphWeights);
            if (clipB && blend > 0.0f)
                //accumulateMorphWeights(*clipB, obj, anim.timeB, blend, anim.morphWeights, rm);
                accumulateMorphWeights(*clipB, comp.timeB, blend, graphicsAssetSystem, *renderComp, "", comp.morphWeights);
        }
    }
}