#pragma once

/******************************************************************************/
/*!
\file   RenderComponent.h
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

#include "Materials.h"
#include "ResourceManager.h"

class RenderComponent : public IRegisteredComponent<RenderComponent>
#ifdef IMGUI_ENABLED 
                        ,IEditorComponent<RenderComponent> 
#endif
{
public:
    explicit RenderComponent();

    explicit RenderComponent(size_t spriteID, bool flippedX = false, bool flippedY = false);

    explicit RenderComponent(const std::string& spriteName, bool flippedX = false, bool flippedY = false);

    size_t GetSpriteID() const;

    bool GetFlippedX() const;

    bool GetFlippedY() const;

    void SetSpriteID(size_t new_spriteID);

    void SetFlippedX(bool new_flippedX);

    void SetFlippedY(bool new_flippedY);

    Vector4 GetColor() const;

    void SetColor(Vector4 new_color);

    const MaterialInstance& GetMaterialInstance() const;

    MaterialInstance& GetMaterialInstance();

    void SetMaterial(const std::string& name);

    std::string GetMaterialName() const;

    size_t spriteID;
    Vector4 color; // REMOVE THIS LINE ONCE SCENE CHANGES ARE STABALIZED
    bool flippedX;
    bool flippedY;
    MaterialInstance m_materialInstance;
    
#ifdef IMGUI_ENABLED
    static void EditorDraw(RenderComponent& comp);
#endif
    
    property_vtable()
};

// Updated property registration to include material instance
property_begin(RenderComponent)
{
    property_var(spriteID),
    property_var(color), // REMOVE THIS LINE ONCE SCENE CHANGES ARE STABALIZED
    property_var(flippedX),
    property_var(flippedY),
    property_var(m_materialInstance)
}
property_vend_h(RenderComponent)
