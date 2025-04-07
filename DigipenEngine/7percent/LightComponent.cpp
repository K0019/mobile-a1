/******************************************************************************/
/*!
\file   LightComponent.cpp
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
  This is the source file for light components.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "LightComponent.h"

LightComponent::LightComponent() : 
    LightComponent(Vector3(1.0f), 1.0f, 10.0f, 1.0f) {}

LightComponent::LightComponent(
    const Vector3& color,
    float intensity,
    float radius,
    float falloffExponent
)
    : color{ color }
    , intensity{ intensity }
    , radius{ radius }
    , falloffExponent{ falloffExponent }
    , state{ true, true, false } // enabled, castShadows, isSpot
{
}

void LightComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    // Core light state
    bool enabled = state.enabled;
    if(ImGui::Checkbox("Enabled", &enabled)) {
        state.enabled = enabled;
    }

    if(!enabled) return;

    // Shadow casting control
    ImGui::SameLine();
    ImGui::Checkbox("Cast Shadows", &state.castShadows);
    
    ImGui::SeparatorText("Light Properties");

    // Color and intensity editor - maintained from original
    float colorWithIntensity[4] = {
        color.x,
        color.y,
        color.z,
        intensity
    };
    if(ImGui::ColorEdit4("Color & Intensity", colorWithIntensity)) {
        color = Vector3(
            colorWithIntensity[0],
            colorWithIntensity[1],
            colorWithIntensity[2]
        );
        intensity = colorWithIntensity[3];
    }

    // Distance attenuation controls
    ImGui::DragFloat("Radius", &radius, 1.0f, 1.0f,1000.0f, "%.0f units");
    ImGui::SliderFloat("Falloff Exponent", &falloffExponent, 0.1f, 5.0f);
    ImGui::SliderFloat("Inner Radius", &innerRadius, 0.0f, 
                      radius, "%.1f units");

    // Spot light controls
    bool isSpot = state.isSpot;
    if(ImGui::Checkbox("Spot Light", &isSpot)) {
        state.isSpot = isSpot;
    }

    if(state.isSpot) {
        ImGui::SeparatorText("Spotlight Properties");
        
        // Simplified angle control with single angle + falloff
        ImGui::SliderFloat("Cone Angle", &coneAngle, 1.0f, 180.0f);
        
        // Falloff control determines inner angle automatically
        ImGui::SliderFloat("Edge Softness", &coneFalloff, 0.0f, 1.0f);
        
        // Display calculated angles for reference
        ImGui::BeginDisabled();
        float innerAngle = getInnerConeAngle();
        float outerAngle = getOuterConeAngle();
        ImGui::LabelText("Effective Angles", 
            "Inner: %.1f, Outer: %.1f", 
            innerAngle, outerAngle);
        ImGui::EndDisabled();
    }

    // Advanced debug visualization
    if(ImGui::CollapsingHeader("Debug Info")) {
        ImGui::BeginDisabled();
        ImGui::LabelText("Inner Angle (rad)", "%.3f", 
                        glm::radians(getInnerConeAngle()));
        ImGui::LabelText("Outer Angle (rad)", "%.3f", 
                        glm::radians(getOuterConeAngle()));
        ImGui::LabelText("Effective Range", "%.1f units", 
                        radius - innerRadius);
        ImGui::EndDisabled();
    }
#endif
}

LightBlinkComponent::LightBlinkComponent()
    : minAlpha{ 0.0f }
    , maxAlpha{ 1.0f }
    , minRadius{ 0.0f }
    , maxRadius{ 1.0f }
    , speed{ 1.0f }
    , accumulatedTime{ 0.0f }
{
}

Vector2 LightBlinkComponent::AddTimeElapsed(float dt)
{
    constexpr float period{ 2 * math::PI_f };

    accumulatedTime += dt * speed;
    accumulatedTime -= static_cast<float>(static_cast<int>(accumulatedTime / period)) * period;

    float t{ sinf(accumulatedTime) * 0.5f + 0.5f };
    return Vector2{
        util::Lerp(minAlpha, maxAlpha, t),
        util::Lerp(minRadius, maxRadius, t)
    };
}

void LightBlinkComponent::EditorDraw()
{
    gui::Slider("Min Brightness", &minAlpha, 0.0f, 1.0f);
    gui::Slider("Max Brightness", &maxAlpha, 0.0f, 1.0f);
    gui::Slider("Min Radius", &minRadius, 1.0f, 1000.0f);
    gui::Slider("Max Radius", &maxRadius, 1.0f, 1000.0f);
    gui::Slider("Speed", &speed, 0.1f, 100.0f);
}
