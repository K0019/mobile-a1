#pragma once
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "resource/processed_assets.h"
#include "resource/skeleton.h"

class AnimationComponent
    : public IRegisteredComponent<AnimationComponent>
    , public IEditorComponent<AnimationComponent>
{

public:
    void  SetupAnimationBinding();
    const ResourceAnimation* GetAnimationClipA() const;
    const ResourceAnimation* GetAnimationClipB() const;

    void EditorDraw() override;


public:
    UserResourceHandle<ResourceAnimation> animHandleA;
    UserResourceHandle<ResourceAnimation> animHandleB;

    //The below can be thought of as the equivalent of the animBinding representation used by ryan
    bool isPlaying = false;
    bool loop = true;
    float speed = 1.0f;

    bool crossfade = false;
    float blend = 0.0f;

    float timeA = 0.0f;
    float timeB = 0.0f;

    uint16_t jointCount = 0;
    uint16_t morphCount = 0;
    std::vector<int16_t> jointRemap;
    std::vector<mat4> invBindMatrices;
    std::vector<mat4> skinMatrices;
    std::vector<float> morphWeights;

    // World-space bone transforms (skeleton space) for bone attachment
    std::vector<mat4> boneWorldTransforms;

    float GetClipDuration(const ResourceAnimation* animationClip);

public:
    property_vtable()
};
property_begin(AnimationComponent)
{
    property_var(animHandleA),
    property_var(animHandleB),
    property_var(isPlaying),
    property_var(loop),
    property_var(crossfade),
    property_var(blend),
    property_var(speed),
    //property_var(timeA),
    //property_var(timeB),
}
property_vend_h(AnimationComponent)


class AnimationSystem : public ecs::System<AnimationSystem, AnimationComponent>
{
public:
    AnimationSystem();

private:
    void ProcessComp(AnimationComponent& comp);
};
