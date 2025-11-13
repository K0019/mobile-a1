#pragma once
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"


class AnimationComponent
    : public IRegisteredComponent<AnimationComponent>
    , public IEditorComponent<AnimationComponent>
{
public:
    const ResourceMesh* GetMesh();
    const std::vector<UserResourceHandle<ResourceMaterial>>& GetMaterialsList() const;
    const ResourceAnimation* GetAnimation() const;
    const bool   IsPlaying() const;
    const bool   IsLooping() const;
    const float  GetTime() const;
    const float  GetPlaybackSpeed() const;

    void EditorDraw() override;


    float time = 0.0f;
private:
    UserResourceHandle<ResourceMesh> meshHandle;
    std::vector<UserResourceHandle<ResourceMaterial>> materials;
    UserResourceHandle<ResourceAnimation> animHandle;

    bool isPlaying = false;
    bool loop = true;
    float playbackSpeed = 1.0f;

public:
    property_vtable()
};
property_begin(AnimationComponent)
{
    property_var(meshHandle),
    property_var(materials),
    property_var(animHandle),
    property_var(time),
    property_var(isPlaying),
    property_var(loop),
    property_var(playbackSpeed),
}
property_vend_h(AnimationComponent)

