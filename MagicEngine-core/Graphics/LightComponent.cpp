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

#include "Graphics/LightComponent.h"
#include "Editor/Containers/GUICollection.h"

LightComponent::LightComponent() = default;

void LightComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    ImGui::Text("Light Properties");

    // Light Type Combo
    const char* lightTypeItems[] = { "Directional", "Point", "Spot", "Area" };
    int currentLightType = static_cast<int>(light.type);
    if (ImGui::Combo("Type", &currentLightType, lightTypeItems, IM_ARRAYSIZE(lightTypeItems)))
    {
        light.type = static_cast<LightType>(currentLightType);
    }

    // Name Input
    char nameBuffer[128];
    strncpy_s(nameBuffer, light.name.c_str(), sizeof(nameBuffer));
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
    {
        light.name = nameBuffer;
    }

    // Direction
    if (light.type == LightType::Directional || light.type == LightType::Spot)
    {
        if (ImGui::DragFloat3("Direction", &light.direction.x, 0.01f))
        {
            // Normalize direction to maintain valid direction vector
            light.direction = normalize(light.direction);
        }
    }

    // Color
    if (ImGui::ColorEdit3("Color", &light.color.x))
    {
        // Color updated directly
    }

    // Intensity
    if (ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f))
    {
        // Intensity updated directly
    }

    // Attenuation (for Point and Spot lights)
    if (light.type == LightType::Point || light.type == LightType::Spot)
    {
        if (ImGui::DragFloat3("Attenuation (Const, Lin, Quad)", &light.attenuation.x, 0.01f, 0.0f, 10.0f))
        {
            // Attenuation updated directly
        }
    }

    // Cone angles (for Spot lights)
    if (light.type == LightType::Spot)
    {
        float innerDeg = glm::degrees(light.innerConeAngle);
        float outerDeg = glm::degrees(light.outerConeAngle);
        bool anglesChanged = false;

        if (ImGui::DragFloat("Inner Cone Angle (deg)", &innerDeg, 0.1f, 0.0f, outerDeg))
        {
            anglesChanged = true;
        }
        if (ImGui::DragFloat("Outer Cone Angle (deg)", &outerDeg, 0.1f, innerDeg, 90.0f))
        {
            anglesChanged = true;
        }

        if (anglesChanged)
        {
            light.innerConeAngle = glm::radians(innerDeg);
            light.outerConeAngle = glm::radians(outerDeg);
        }
    }

    // Area size (for Area lights)
    if (light.type == LightType::Area)
    {
        if (ImGui::DragFloat2("Area Size", &light.areaSize.x, 0.1f, 0.0f, 100.0f))
        {
            // Area size updated directly
        }
    }
#endif
}

void LightComponent::Serialize(Serializer& writer) const
{
    writer.Serialize("type", static_cast<uint8_t>(light.type));
    writer.Serialize("direction", light.direction);
    writer.Serialize("color", light.color);
    writer.Serialize("attenuation", light.attenuation);
    writer.Serialize("intensity", light.intensity);
    writer.Serialize("innerConeAngle", light.innerConeAngle);
    writer.Serialize("outerConeAngle", light.outerConeAngle);
    writer.Serialize("areaSize", light.areaSize);
}

void LightComponent::Deserialize(Deserializer& reader)
{
    Vec3 v3;
    Vec2 v2;
    reader.DeserializeVar("type", reinterpret_cast<uint8_t*>(&light.type));
    reader.DeserializeVar("direction", &v3), light.direction = v3;
    reader.DeserializeVar("color", &v3), light.color = v3;
    reader.DeserializeVar("attenuation", &v3), light.attenuation = v3;
    reader.DeserializeVar("intensity", &light.intensity);
    reader.DeserializeVar("innerConeAngle", &light.innerConeAngle);
    reader.DeserializeVar("outerConeAngle", &light.outerConeAngle);
    reader.DeserializeVar("areaSize", &v2), light.areaSize = v2;
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

Vec2 LightBlinkComponent::AddTimeElapsed(float dt)
{
    constexpr float period{ 2 * math::PI_f };

    accumulatedTime += dt * speed;
    accumulatedTime -= static_cast<float>(static_cast<int>(accumulatedTime / period)) * period;

    float t{ sinf(accumulatedTime) * 0.5f + 0.5f };
    return Vec2{
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
