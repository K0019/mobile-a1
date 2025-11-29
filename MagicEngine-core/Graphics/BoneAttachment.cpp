#include "Graphics/BoneAttachment.h"
#include "Graphics/AnimationComponent.h"
#include "Graphics/RenderComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    glm::mat4 composeTRS(const glm::vec3& translation,
        const glm::vec3& rotationDegrees,
        const glm::vec3& scale)
    {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 r = glm::mat4(1.0f);
        r = glm::rotate(r, glm::radians(rotationDegrees.z), glm::vec3(0, 0, 1));
        r = glm::rotate(r, glm::radians(rotationDegrees.y), glm::vec3(0, 1, 0));
        r = glm::rotate(r, glm::radians(rotationDegrees.x), glm::vec3(1, 0, 0));
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
}

void BoneAttachment::CacheBoneIndex()
{
    cachedBoneIndex = -1;

    if (!targetEntity || boneName.empty())
        return;

    ecs::EntityHandle target = targetEntity;

    AnimationComponent* animComp = target->GetComp<AnimationComponent>();
    if (!animComp)
        return;

    RenderComponent* renderComp = target->GetComp<RenderComponent>();
    if (!renderComp || !renderComp->GetMesh())
        return;

    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    const auto* mesh = renderComp->GetMesh();
    if (!mesh || mesh->handles.empty())
        return;

    const auto* meshMetadata = graphicsAssetSystem.getMeshMetadata(mesh->handles[0]);
    if (!meshMetadata)
        return;

    const auto& skeleton = graphicsAssetSystem.Skeleton(meshMetadata->skeletonId);
    cachedBoneIndex = skeleton.indexOfJoint(boneName);
}

void BoneAttachment::EditorDraw()
{
    // Target entity field - use EntityReference's built-in editor draw
    if (targetEntity.EditorDraw("Target Entity"))
    {
        CacheBoneIndex();
    }

    // Bone selection dropdown
    if (targetEntity)
    {
        ecs::EntityHandle target = targetEntity;
        AnimationComponent* animComp = target->GetComp<AnimationComponent>();
        RenderComponent* renderComp = target->GetComp<RenderComponent>();

        if (animComp && renderComp && renderComp->GetMesh())
        {
            auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
            const auto* mesh = renderComp->GetMesh();

            if (mesh && !mesh->handles.empty())
            {
                const auto* meshMetadata = graphicsAssetSystem.getMeshMetadata(mesh->handles[0]);
                if (meshMetadata)
                {
                    const auto& skeleton = graphicsAssetSystem.Skeleton(meshMetadata->skeletonId);
                    auto jointNames = skeleton.jointNames();

                    gui::TextUnformatted("Bone");
                    gui::SameLine();

                    if (gui::Combo combo{ "##BoneCombo", boneName.empty() ? "Select Bone" : boneName.c_str() })
                    {
                        for (const auto& name : jointNames)
                        {
                            if (combo.Selectable(name.c_str(), name == boneName))
                            {
                                boneName = name;
                                CacheBoneIndex();
                            }
                        }
                    }
                }
            }
        }
        else
        {
            gui::TextDisabled("Target has no AnimationComponent or RenderComponent");
        }
    }

    // Cached bone index display
    gui::TextUnformatted("Cached Index");
    gui::SameLine();
    gui::TextDisabled("%d", cachedBoneIndex);

    gui::Separator();

    // Local offset
    gui::VarDrag("Offset", &localOffset, 0.01f);

    // Local rotation
    gui::VarDrag("Rotation", &localRotation, 1.0f);
}


BoneAttachmentSystem::BoneAttachmentSystem() :
    System_Internal{ &BoneAttachmentSystem::ProcessComp }
{}

void BoneAttachmentSystem::ProcessComp(BoneAttachment& comp)
{
    if (!comp.targetEntity)
        return;

    // Re-cache if needed
    if (comp.cachedBoneIndex < 0 && !comp.boneName.empty())
        comp.CacheBoneIndex();

    if (comp.cachedBoneIndex < 0)
        return;

    ecs::EntityHandle target = comp.targetEntity;

    AnimationComponent* animComp = target->GetComp<AnimationComponent>();
    if (!animComp)
        return;

    // Ensure bone index is valid and transforms are available
    if (animComp->boneWorldTransforms.empty() || 
        comp.cachedBoneIndex >= static_cast<int16_t>(animComp->boneWorldTransforms.size()))
        return;

    ecs::EntityHandle thisEntity = ecs::GetEntity(&comp);
    if (!thisEntity)
        return;

    Transform& transform = thisEntity->GetTransform();

    // Get the bone's transform (in skeleton/model space)
    const glm::mat4& boneLocalTransform = animComp->boneWorldTransforms[comp.cachedBoneIndex];

    // Get the skeleton entity's world transform
    const Mat4& skeletonWorldMat = target->GetTransform().GetWorldMat();
    glm::mat4 skeletonWorld = static_cast<const glm::mat4&>(skeletonWorldMat);

    // Include mesh transform if available (same as rendering does)
    RenderComponent* renderComp = target->GetComp<RenderComponent>();
    if (renderComp && renderComp->GetMesh() && !renderComp->GetMesh()->transforms.empty())
    {
        const Mat4& meshTransform = renderComp->GetMesh()->transforms[0];
        skeletonWorld = skeletonWorld * static_cast<const glm::mat4&>(meshTransform);
    }

    // Bone world position = skeletonWorld * boneLocalTransform
    glm::mat4 boneWorld = skeletonWorld * boneLocalTransform;

    // If this entity has a Transform parent, we need to convert to local space
    glm::mat4 finalLocal;
    if (transform.GetParent())
    {
        const Mat4& parentWorldMat = transform.GetParent()->GetWorldMat();
        glm::mat4 parentWorldInv = glm::inverse(static_cast<const glm::mat4&>(parentWorldMat));
        finalLocal = parentWorldInv * boneWorld;
    }
    else
    {
        finalLocal = boneWorld;
    }

    // Extract position
    glm::vec3 localPos = glm::vec3(finalLocal[3]);

    // Extract rotation (normalize columns to remove scale influence)
    glm::vec3 col0 = glm::vec3(finalLocal[0]);
    glm::vec3 col1 = glm::vec3(finalLocal[1]);
    glm::vec3 col2 = glm::vec3(finalLocal[2]);
    
    float len0 = glm::length(col0);
    float len1 = glm::length(col1);
    float len2 = glm::length(col2);
    
    // Avoid division by zero
    if (len0 > 0.0001f) col0 /= len0;
    if (len1 > 0.0001f) col1 /= len1;
    if (len2 > 0.0001f) col2 /= len2;

    glm::mat3 rotMat;
    rotMat[0] = col0;
    rotMat[1] = col1;
    rotMat[2] = col2;

    // Convert rotation matrix to euler angles (degrees)
    glm::vec3 eulerRad;
    eulerRad.x = atan2(rotMat[2][1], rotMat[2][2]);
    eulerRad.y = atan2(-rotMat[2][0], sqrt(rotMat[2][1] * rotMat[2][1] + rotMat[2][2] * rotMat[2][2]));
    eulerRad.z = atan2(rotMat[1][0], rotMat[0][0]);
    glm::vec3 localRot = glm::degrees(eulerRad);

    // Apply rotation offset
    localRot += static_cast<const glm::vec3&>(comp.localRotation);

    // Apply position offset in bone-local space (rotated by bone orientation)
    glm::vec3 offsetWorld = rotMat * static_cast<const glm::vec3&>(comp.localOffset);
    localPos += offsetWorld;

    // Set position and rotation only - preserve the entity's existing scale
    transform.SetLocalPosition(localPos);
    transform.SetLocalRotation(localRot);
}
