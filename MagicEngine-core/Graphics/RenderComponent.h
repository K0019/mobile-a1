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
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

class RenderComponent
    : public IRegisteredComponent<RenderComponent>
    , public IEditorComponent<RenderComponent>
{
public:
    const ResourceMesh* GetMesh() const;
    const std::vector<UserResourceHandle<ResourceMaterial>>& GetMaterialsList() const;
    std::vector<UserResourceHandle<ResourceMaterial>>& GetMaterialsList();

    void EditorDraw() override;

    // Shadow casting override (-1 = use material setting, 0 = off, 1 = on)
    int castShadowOverride = -1;

private:
    UserResourceHandle<ResourceMesh> meshHandle;
    std::vector<UserResourceHandle<ResourceMaterial>> materials;

public:
    property_vtable()
};
property_begin(RenderComponent)
{
    property_var(meshHandle),
    property_var(materials),
    property_var(castShadowOverride)
}
property_vend_h(RenderComponent)

// RenderSystem is no longer needed - GraphicsMain reads directly from ECS RenderComponents
