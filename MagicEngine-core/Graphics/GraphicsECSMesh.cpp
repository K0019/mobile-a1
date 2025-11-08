/******************************************************************************/
/*!
\file   RenderComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of RenderComponent.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VFS/VFS.h"
#include "Graphics/GraphicsECSMesh.h"
#include "Engine/Graphics Interface/GraphicsScene.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Containers/GUICollection.h"

size_t RenderComponent::GetMeshHash() const
{
    return meshHash;
}

const std::vector<size_t>& RenderComponent::GetMaterialsList() const
{
    return materials;
}

void RenderComponent::EditorDraw()
{
    auto& fpManager = ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager();
    auto meshEntry = fpManager.GetFileEntry(meshHash);

    std::string meshText = std::to_string(meshHash);
    if (meshEntry)
        meshText = VFS::GetStem(meshEntry->path);

    gui::TextUnformatted("Mesh");
    gui::SameLine();
    gui::TextBoxReadOnly("##", meshText);
    gui::PayloadTarget<size_t>("MESH_HASH", [this](size_t hash) -> void {
        meshHash = hash;

        //now based on this new mesh, update the default materials
        auto newMesh{ MagicResourceManager::Meshes().GetResource(meshHash) };
        size_t meshCount = newMesh->handles.size();
        materials.resize(meshCount);
        for (int i = 0; i < meshCount; i++)
        {
            materials[i] = newMesh->defaultMaterialHashes[i];
        }
    });

    auto mesh{ MagicResourceManager::Meshes().GetResource(meshHash) };


    if (mesh)
    {
        size_t meshCount = mesh->handles.size();
        std::string materialHeaderLabel = "Materials (" + std::to_string(meshCount) + ")";
        if (ImGui::CollapsingHeader(materialHeaderLabel.c_str()))
        {
            for (int i = 0; i < meshCount; i++)
            {
                if (i >= materials.size()) break;    //go reassign the mesh
                gui::SetID id(i);

                std::string materialText = std::to_string(materials[i]);
                auto materialEntry = fpManager.GetFileEntry(materials[i]);
                if(materialEntry)
                    materialText = VFS::GetStem(materialEntry->path);

                gui::TextUnformatted(std::string("Material") + std::to_string(i));
                gui::SameLine();
                gui::TextBoxReadOnly("##", materialText);
                gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
                    materials[i] = hash;
                });
                gui::SameLine();
            
                if (ImGui::Button(ICON_FA_REPEAT))
                {
                    materials[i] = mesh->defaultMaterialHashes[i];
                }
            }

            if (gui::Button("Reset Materials"))
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

RenderSystem::RenderSystem()
    : System_Internal{ &RenderSystem::ProcessComp }
{
}

void RenderSystem::ProcessComp(RenderComponent& comp)
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

        ST<GraphicsScene>::Get()->AddObject(
            mesh->handles[i],
            material->handle,
            ecs::GetEntityTransform(&comp),
            mesh->transforms[i]
        );
    }
}
