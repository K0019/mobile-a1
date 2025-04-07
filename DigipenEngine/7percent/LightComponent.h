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
/*****************************************************************//*!
\class LightComponent
\brief
    Attaches a light to an entity.
*//******************************************************************/
class LightComponent : public IRegisteredComponent<LightComponent>
#ifdef IMGUI_ENABLED
    , public IEditorComponent<LightComponent>
#endif 
{
public:
    // Flags stored as individual bools for clearer code and easier serialization
    // Will be packed during GPU upload
    struct LightState {
        bool enabled = true;
        bool castShadows = true;  // New: Control shadow casting
        bool isSpot = false;      // Determines if angular falloff is applied
    };

    explicit LightComponent();
    
    LightComponent(
        const Vector3& color,
        float intensity,
        float radius,
        float falloffExponent = 1.0f
    );

    // Core light properties
    Vector3 color{1.0f};
    float intensity{1.0f};
    
    // Distance attenuation
    float radius{10.0f};          // Maximum light influence radius
    float falloffExponent{1.0f};  // Controls distance falloff curve
    float innerRadius{0.0f};      // Distance before falloff begins (optional optimization)
    
    // Spot light properties (only relevant when isSpot = true)
    // Stored in degrees for editor friendliness, converted to radians during GPU upload
    float coneAngle{45.0f};       // Full cone angle (replaces inner/outer - simplifies usage)
    float coneFalloff{0.5f};      // [0,1] Controls softness of cone edge
    
    LightState state;

    // Helper functions for common operations
    float getRadiusSquared() const { return radius * radius; }
    float getConeAngleRadians() const { return glm::radians(coneAngle); }
    
    // Calculate derived properties
    float getInnerConeAngle() const { return coneAngle * (1.0f - coneFalloff); }
    float getOuterConeAngle() const { return coneAngle; }

    void SetRadius(float newRadius) { radius = newRadius; }

#ifdef IMGUI_ENABLED
    static void EditorDraw(LightComponent& component);
#endif 

    property_vtable()
};

// Property reflection
property_begin(LightComponent)
{
    property_var(color),
    property_var(intensity),
    property_var(radius),
    property_var(falloffExponent),
    property_var(innerRadius),
    property_var(coneAngle),
    property_var(coneFalloff),
    property_var(state.enabled),
    property_var(state.castShadows),
    property_var(state.isSpot),
}
property_vend_h(LightComponent)

/*****************************************************************//*!
\class LightBlinkComponent
\brief
    Changes the alpha of a light to cause a "blink" effect.
*//******************************************************************/
class LightBlinkComponent : public IRegisteredComponent<LightBlinkComponent>
#ifdef IMGUI_ENABLED
    , public IEditorComponent<LightBlinkComponent>
#endif
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
    Vector2 AddTimeElapsed(float dt);

#ifdef IMGUI_ENABLED
private:
    /*****************************************************************//*!
    \brief
        Draws this component to the inspector.
    \param comp
        The component.
    *//******************************************************************/
    static void EditorDraw(LightBlinkComponent& comp);
#endif

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
