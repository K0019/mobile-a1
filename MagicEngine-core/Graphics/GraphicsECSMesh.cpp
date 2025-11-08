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
#include "Graphics/GraphicsECSMesh.h"
#include "Engine/Graphics Interface/GraphicsScene.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Containers/GUICollection.h"

size_t RenderComponent::GetMeshHash() const
{
    return meshHash;
}
size_t RenderComponent::GetMaterialHash() const
{
    return materialHash;
}

void RenderComponent::EditorDraw()
{
    gui::TextBoxReadOnly("Mesh", std::to_string(meshHash));
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
    size_t meshCount = mesh->handles.size();
    gui::TextFormatted("Materials List: %d", meshCount);
    for (int i = 0; i < meshCount; i++)
    {
        if (i >= materials.size()) break;    //go reassign the mesh

        gui::SetID id(i);

        gui::TextBoxReadOnly("MaterialSlot", std::to_string(materials[i]));
        gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
            materials[i] = hash;
        });
    }

    //gui::TextBoxReadOnly("Materials List", "Materials List");

    gui::PayloadTarget<size_t>("MATERIAL_HASH", [this](size_t hash) -> void {
        materialHash = hash;
    });

}

RenderSystem::RenderSystem()
    : System_Internal{ &RenderSystem::ProcessComp }
{
}

void RenderSystem::ProcessComp(RenderComponent& comp)
{
    auto mesh{ MagicResourceManager::Meshes().GetResource(comp.GetMeshHash()) };
    auto material{ MagicResourceManager::Materials().GetResource(comp.GetMaterialHash()) };
    //if (!(mesh && material))
    //    return;

    //for (size_t i{}; i < mesh->handles.size(); ++i)
    //    ST<GraphicsScene>::Get()->AddObject(mesh->handles[i], material->handle, ecs::GetEntityTransform(&comp).GetWorldMat() * mesh->transforms[i]);
    //    ST<GraphicsScene>::Get()->AddObject(mesh->handles[i], material->handle, ecs::GetEntityTransform(&comp), mesh->transforms[i]);

    if (!mesh)
        return;

    for (size_t i{}; i < mesh->handles.size(); ++i)
    {
        auto material{ MagicResourceManager::Materials().GetResource(mesh->defaultMaterialHashes[i]) };

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
