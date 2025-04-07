/******************************************************************************/
/*!
\file   CameraComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of CameraComponent.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "CameraComponent.h"

int CameraComponent::globalPriority = 0;

CameraComponent::CameraComponent(bool active): 
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw), 
#endif
    active(active)
{
    if(active)
    {
        SetActive();
    }
}

bool CameraComponent::isActive() const { return active; }

void CameraComponent::SetActive() { active = true; priority = ++globalPriority;}

void CameraComponent::OnAttached()
{
    SetActive();
}

#ifdef IMGUI_ENABLED
void CameraComponent::EditorDraw(CameraComponent& comp)
{
    bool active = comp.isActive();
    if(ImGui::Checkbox("Currently Active", &active))
    {
        comp.SetActive();
    }
    ImGui::Text("Zoom: %.2f", comp.zoom);
}
#endif

ShakeComponent::ShakeComponent()
    : REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw)
    , trauma{}
    , traumaExponent{ 2.0f }
    , recoverySpeed{ 1.5f }
    , frequency{ 25.0f }
    , time{ util::RandomRangeFloat(0.0f, 100.0f) * frequency }
    , maxPosOffset{ 50.0f, 50.0f }
    , maxRotOffset{ 20.0f }
    , appliedOffsets{ {}, 0.0f }
{
}

void ShakeComponent::InduceStress(float strength, float cap)
{
    if (trauma >= cap)
        return;
    trauma = math::Clamp(trauma + strength, 0.0f, cap);
}

void ShakeComponent::UpdateTime(float dt)
{
    time += dt * frequency;

    trauma -= recoverySpeed * dt;
    if (trauma < 0.0f)
        trauma = 0.0f;
}

const ShakeComponent::Offsets& ShakeComponent::CalcOffsets()
{
    if (trauma <= std::numeric_limits<float>::epsilon())
    {
        appliedOffsets.pos.x = appliedOffsets.pos.y = appliedOffsets.rot = 0.0f;
        return appliedOffsets;
    }

    float shake{ std::powf(trauma, traumaExponent) };

    appliedOffsets.pos.x = maxPosOffset.x * (util::PerlinNoise(0.5f, time) * 2.0f) * shake;
    appliedOffsets.pos.y = maxPosOffset.y * (util::PerlinNoise(1.5f, time) * 2.0f) * shake;
    appliedOffsets.rot = maxRotOffset * (util::PerlinNoise(2.5f, time) * 2.0f) * shake;

    return appliedOffsets;
}

const ShakeComponent::Offsets& ShakeComponent::GetOffsets() const
{
    return appliedOffsets;
}

float ShakeComponent::GetTrauma() const
{
    return trauma;
}

void ShakeComponent::EditorDraw(ShakeComponent& comp)
{
    gui::TextBoxReadOnly("Trauma", std::to_string(comp.trauma));
    gui::VarDrag("Shake Speed", &comp.frequency, 1.0f, 0.0f, 100.0f, "%.1f");
    gui::VarDrag("Recovery Speed", &comp.recoverySpeed, 0.05f, 0.1f, 10.0f, "%.1f");
    gui::VarDrag("Falloff Exponent", &comp.traumaExponent, 0.2f, 1.0f, 10.0f, "%.1f");

    gui::Separator();

    gui::VarDrag("Max Position Offset", &comp.maxPosOffset);
    gui::VarDrag("Max Rotation Offset", &comp.maxRotOffset);

    if (gui::Button{ "Test Shake" })
        comp.InduceStress(1.0f);
}
