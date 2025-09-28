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
#include "IRegisteredComponent.h"
#include "IEditorComponent.h"

class RenderComponent
    : public IRegisteredComponent<RenderComponent>
    , public IEditorComponent<RenderComponent>
{
public:
    size_t GetMeshHash() const;
    size_t GetMaterialHash() const;

    void EditorDraw() override;

private:
    size_t meshHash;
    size_t materialHash;

public:
    property_vtable()
};
property_begin(RenderComponent)
{
    property_var(meshHash),
    property_var(materialHash)
}
property_vend_h(RenderComponent)

class RenderSystem : public ecs::System<RenderSystem, RenderComponent>
{
public:
    RenderSystem();

private:
    void ProcessComp(RenderComponent& comp);

};
