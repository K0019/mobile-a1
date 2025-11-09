/******************************************************************************/
/*!
\file   GraphicsECSSprite.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Main component for rendering sprites. All sprite rendering will go through this component, regardless of whether it has an animation or not.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"

class RenderComponent
    : public IRegisteredComponent<RenderComponent>
    , public IEditorComponent<RenderComponent>
{
public:
    size_t GetMeshHash() const;
    const std::vector<size_t>& GetMaterialsList() const;

    void EditorDraw() override;

private:
    size_t meshHash;
    std::vector<size_t> materials;

public:
    property_vtable()
};
property_begin(RenderComponent)
{
    property_var(meshHash),
    property_var(materials)
}
property_vend_h(RenderComponent)

class RenderSystem : public ecs::System<RenderSystem, RenderComponent>
{
public:
    RenderSystem();

private:
    void ProcessComp(RenderComponent& comp);

};
