#include "VFS/VFS.h"
#include "Graphics/RenderComponent.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Containers/GUICollection.h"
#include "Graphics/AnimationComponent.h"


const ResourceMesh* AnimationComponent::GetMesh()
{
    return meshHandle.GetResource();
}
const std::vector<UserResourceHandle<ResourceMaterial>>& AnimationComponent::GetMaterialsList() const
{
    return materials;
}
const ResourceAnimation* AnimationComponent::GetAnimation() const
{
    return animHandle.GetResource();
}
const bool AnimationComponent::IsPlaying() const
{
    return isPlaying;
}
const bool AnimationComponent::IsLooping() const
{
    return loop;
}
const float AnimationComponent::GetTime() const
{
    return time;
}
const float AnimationComponent::GetPlaybackSpeed() const
{
    return playbackSpeed;
}


void AnimationComponent::EditorDraw()
{
    const std::string* meshText{ ST<MagicResourceManager>::Get()->Editor_GetName(meshHandle.GetHash()) };

    gui::TextUnformatted("Mesh");
    gui::SameLine();
    gui::TextBoxReadOnly("##1", meshText ? meshText->c_str() : "");
    gui::PayloadTarget<size_t>("MESH_HASH", [this](size_t hash) -> void {
        meshHandle = hash;

        //now based on this new mesh, update the default materials
        auto newMesh{ meshHandle.GetResource() };
        size_t meshCount = newMesh->handles.size();
        materials.resize(meshCount);
        for (int i = 0; i < meshCount; i++)
        {
            materials[i] = newMesh->defaultMaterialHashes[i];
        }
        });

    auto mesh{ meshHandle.GetResource() };

    // Animation input
    gui::TextUnformatted(std::string("Animation Clip"));
    gui::SameLine();
    gui::TextBoxReadOnly("##2", std::to_string(animHandle.GetHash()));
    gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
        animHandle = hash;
        });

    gui::TextUnformatted(std::string("Is Playing"));
    gui::SameLine();
    gui::Checkbox("##3", &isPlaying);

    gui::TextUnformatted(std::string("Looping"));
    gui::SameLine();
    gui::Checkbox("##4", &loop);


    if (mesh)
    {
        size_t meshCount = mesh->handles.size();
        std::string materialHeaderLabel = "Materials (" + std::to_string(meshCount) + ")";
        if (gui::CollapsingHeader materialHeader{ materialHeaderLabel.c_str() })
        {
            for (int i = 0; i < meshCount; i++)
            {
                if (i >= materials.size()) break;    //go reassign the mesh
                gui::SetID id(i);

                const std::string* materialText{ ST<MagicResourceManager>::Get()->Editor_GetName(materials[i].GetHash()) };

                gui::TextUnformatted(std::string("Material") + std::to_string(i));
                gui::SameLine();
                gui::TextBoxReadOnly("##", materialText ? materialText->c_str() : "");
                gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
                    materials[i] = hash;
                    });
                gui::SameLine();

                if (gui::Button repeatButton{ ICON_FA_REPEAT })
                {
                    materials[i] = mesh->defaultMaterialHashes[i];
                }
            }

            if (gui::Button resetButton{ "Reset Materials" })
            {
                materials.resize(meshCount);
                for (int i = 0; i < meshCount; i++)
                {
                    materials[i] = mesh->defaultMaterialHashes[i];
                }
            }
        }
    }
}


/*
AnimationSystem::AnimationSystem()
    : System_Internal{ &AnimationSystem::ProcessComp }
{}

void AnimationSystem::ProcessComp(AnimationComponent& comp)
{
    auto mesh{ MagicResourceManager::Meshes().GetResource(comp.GetMeshHash()) };
    if (!mesh)
        return;

    const auto& materialList = comp.GetMaterialsList();
    size_t materialCount = materialList.size();

    for (size_t i{}; i < mesh->handles.size(); ++i)
    {
        size_t materialHashToUse = 0;
        if (i < materialCount && materialList[i] != 0)
        {
            materialHashToUse = materialList[i];
        }
        else
        {
            // Use the default material from the mesh
            materialHashToUse = mesh->defaultMaterialHashes[i];
        }

        auto material{ MagicResourceManager::Materials().GetResource(materialHashToUse) };
        if (!material)
            continue;

        auto clip = MagicResourceManager::Animations().GetResource(comp.GetAnimationHash());
        if (clip)
        {
            SceneObject::AnimBinding binding;
            binding.clipA = clip->handle;
            binding.clipB = Resource::INVALID_CLIP_ID;
            binding.speed = comp.GetPlaybackSpeed();
            binding.blend = 0.0f;
            binding.timeA = comp.GetTime();
            binding.timeB = 0.0f;
            binding.flags = (comp.IsPlaying() ? SceneObject::AnimBinding::Playing : 0) | (comp.IsLooping() ? SceneObject::AnimBinding::Loop : 0);

            ST<GraphicsScene>::Get()->AddAnimatedObject(
                mesh->handles[i],
                material->handle,
                ecs::GetEntityTransform(&comp),
                mesh->transforms[i],
                binding
            );
            comp.time += 0.001;
        }
        else
        {
            ST<GraphicsScene>::Get()->AddObject(
                mesh->handles[i],
                material->handle,
                ecs::GetEntityTransform(&comp),
                mesh->transforms[i]
            );
        }
    }
}

*/
