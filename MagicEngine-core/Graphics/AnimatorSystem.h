#pragma once
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"


class AnimationComponent
    : public IRegisteredComponent<AnimationComponent>
    , public IEditorComponent<AnimationComponent>
{
public:
    const size_t GetAnimationHash() const;
    const bool   IsPlaying() const;
    const bool   IsLooping() const;
    const float  GetTime() const;
    const float  GetPlaybackSpeed() const;

    void EditorDraw() override;
    size_t GetMeshHash() const;
    const std::vector<size_t>& GetMaterialsList() const;

    float time = 0.0f;
private:
    size_t meshHash;

    size_t animHash;
    bool isPlaying = false;
    bool loop = true;
    float playbackSpeed = 1.0f;

    std::vector<size_t> materials;


public:
    property_vtable()
};
property_begin(AnimationComponent)
{
    property_var(meshHash),
    property_var(animHash),
    property_var(time),
    property_var(isPlaying),
    property_var(loop),
    property_var(playbackSpeed),
    property_var(materials),
}
property_vend_h(AnimationComponent)


class AnimationSystem : public ecs::System<AnimationSystem, AnimationComponent>
{
public:
    AnimationSystem();

private:
    void ProcessComp(AnimationComponent& comp);

};
