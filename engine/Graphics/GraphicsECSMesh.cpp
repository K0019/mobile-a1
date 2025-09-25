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
#include "GraphicsECSMesh.h"
#include "GraphicsScene.h"
#include "ResourceManager.h"

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
    });

    gui::TextBoxReadOnly("Material", std::to_string(materialHash));
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
    auto mesh{ ResourceManager::Meshes().GetResource(comp.GetMeshHash()) };
    auto material{ ResourceManager::Materials().GetResource(comp.GetMaterialHash()) };
    if (!(mesh && material))
        return;

    ST<GraphicsScene>::Get()->AddObject(mesh->handle, material->handle, ecs::GetEntityTransform(&comp).GetWorldMat() * mesh->transform);
}
