#pragma once
#include "ECS/IRegisteredComponent.h"
#include "ECS/EntityUID.h"
#include "ECS/IEditorComponent.h"

class BoneAttachment
    : public IRegisteredComponent<BoneAttachment>
    , public IEditorComponent<BoneAttachment>
{
public:
    void EditorDraw() override;

    // Caches the bone index from boneName for the target skeleton
    void CacheBoneIndex();

public:
    // Reference to the entity with the AnimationComponent (skeleton owner)
    EntityReference targetEntity;

    // The name of the bone to attach to
    std::string boneName;

    // Cached bone index (-1 if not found or not cached)
    int16_t cachedBoneIndex = -1;

    // Local offset from the bone
    Vec3 localOffset = { 0.0f, 0.0f, 0.0f };

    // Local rotation offset (in degrees)
    Vec3 localRotation = { 0.0f, 0.0f, 0.0f };

    property_vtable()
};

property_begin(BoneAttachment)
{
    property_var(targetEntity),
    property_var(boneName),
    property_var(localOffset),
    property_var(localRotation),
}
property_vend_h(BoneAttachment)


class BoneAttachmentSystem : public ecs::System<BoneAttachmentSystem, BoneAttachment>
{
public:
    BoneAttachmentSystem();

private:
    void ProcessComp(BoneAttachment& comp);
};
