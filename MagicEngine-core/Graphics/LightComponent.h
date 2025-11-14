/******************************************************************************/
/*!
\file   LightComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   01/15/2025

\author Ryan Cheong (70%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (30%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for light components.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "graphics/scene.h"

/*****************************************************************//*!
\class LightComponent
\brief
    Attaches a light to an entity.
*//******************************************************************/
class LightComponent
    : public IRegisteredComponent<LightComponent>
    , public IEditorComponent<LightComponent>
{
public:
    // Flags stored as individual bools for clearer code and easier serialization
    // Will be packed during GPU upload
    SceneLight light;

    explicit LightComponent();

    virtual void EditorDraw() override;

public:
    void Serialize(Serializer& writer) const override;
    void Deserialize(Deserializer& reader) override;

    // ===== Lua wrappers =====
public:
    int GetTypeLua() const
    {
        return static_cast<int>(light.type); // 0..3
    }
    void SetTypeLua(int t)
    {
        light.type = static_cast<LightType>(t);
    }

    float GetIntensity() const { return light.intensity; }
    void  SetIntensity(float v) { light.intensity = v; }

    float GetInnerConeAngle() const { return light.innerConeAngle; }
    void  SetInnerConeAngle(float v) { light.innerConeAngle = v; }

    float GetOuterConeAngle() const { return light.outerConeAngle; }
    void  SetOuterConeAngle(float v) { light.outerConeAngle = v; }

    std::string GetName() const { return light.name; }
    void        SetName(const std::string& n) { light.name = n; }


};


/*****************************************************************//*!
\class LightBlinkComponent
\brief
    Changes the alpha of a light to cause a "blink" effect.
*//******************************************************************/
class LightBlinkComponent
    : public IRegisteredComponent<LightBlinkComponent>
    , public IEditorComponent<LightBlinkComponent>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    LightBlinkComponent();

    /*****************************************************************//*!
    \brief
        Progresses the blinking of this component. Returns the properties
        that the light should have now.
    \param dt
        The amount of time that has passed.
    \return
        2 floats:
            x - The intensity of the light.
            y - The radius of the light.
    *//******************************************************************/
    Vec2 AddTimeElapsed(float dt);

private:
    /*****************************************************************//*!
    \brief
        Draws this component to the inspector.
    *//******************************************************************/
    virtual void EditorDraw() override;

private:
    //! The min/max brightness of the light.
    float minAlpha, maxAlpha;
    //! The min/max radius of the light.
    float minRadius, maxRadius;
    //! The speed of the light oscillating
    float speed;

    //! The accumulated time.
    float accumulatedTime;

    property_vtable()

public:
    // ===== For Lua =====
    float GetMinAlpha()   const { return minAlpha; }
    void  SetMinAlpha(float v) { minAlpha = v; }

    float GetMaxAlpha()   const { return maxAlpha; }
    void  SetMaxAlpha(float v) { maxAlpha = v; }

    float GetMinRadius()  const { return minRadius; }
    void  SetMinRadius(float v) { minRadius = v; }

    float GetMaxRadius()  const { return maxRadius; }
    void  SetMaxRadius(float v) { maxRadius = v; }

    float GetSpeed()      const { return speed; }
    void  SetSpeed(float v) { speed = v; }

};
property_begin(LightBlinkComponent)
{
    property_var(minAlpha),
    property_var(maxAlpha),
    property_var(minRadius),
    property_var(maxRadius),
    property_var(speed)
}
property_vend_h(LightBlinkComponent)
