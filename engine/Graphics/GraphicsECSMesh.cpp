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

size_t RenderComponent::GetHash() const
{
    return resourceHash;
}

void RenderComponent::EditorDraw()
{
    gui::TextBoxReadOnly("Mesh", std::to_string(resourceHash));
    gui::PayloadTarget<size_t>("RESOURCE_HASH", [this](size_t hash) -> void {
        resourceHash = hash;
    });
}

RenderSystem::RenderSystem()
    : System_Internal{ &RenderSystem::ProcessComp }
{
}

void RenderSystem::ProcessComp(RenderComponent& comp)
{
    auto mesh{ ResourceManager::Meshes().GetResource(comp.GetHash()) };
    if (!mesh)
        return;

    ST<GraphicsScene>::Get()->AddObject(mesh, ecs::GetEntityTransform(&comp).GetWorldMat());
}
