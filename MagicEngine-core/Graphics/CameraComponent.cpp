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
#include "Graphics/CameraComponent.h"
#include "Editor/Containers/GUICollection.h"

int CameraComponent::globalPriority = 0;

CameraComponent::CameraComponent(bool active)
    : active{ active }
{
    if (active)
        SetActive();
}

bool CameraComponent::isActive() const { return active; }

void CameraComponent::SetActive() { active = true; priority = ++globalPriority;}

void CameraComponent::OnAttached()
{
    SetActive();
}

void CameraComponent::EditorDraw()
{
    bool b = isActive();
    if (gui::Checkbox("Currently Active", &b))
        SetActive();
    gui::TextFormatted("Zoom: %.2f", zoom);
}

ShakeComponent::ShakeComponent()
    : trauma{}
    , traumaExponent{ 2.0f }
    , recoverySpeed{ 1.5f }
    , frequency{ 25.0f }
    , time{ util::RandomRangeFloat(0.0f, 100.0f) * frequency }
    , maxPosOffset{ 50.0f, 50.0f, 50.0f }
    , maxRotOffset{}
    , appliedOffsets{ {}, {} }
{
}

void ShakeComponent::InduceStress(float strength, float cap)
{
    if (trauma >= cap)
        return;
    trauma = std::clamp(trauma + strength, 0.0f, cap);
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
        appliedOffsets.pos.x = appliedOffsets.pos.y = appliedOffsets.pos.z =
            appliedOffsets.rot.x = appliedOffsets.rot.y = appliedOffsets.rot.z = 0.0f;
        return appliedOffsets;
    }

    float shake{ std::powf(trauma, traumaExponent) };

    appliedOffsets.pos.x = maxPosOffset.x * (util::PerlinNoise(0.5f, time) * 2.0f) * shake;
    appliedOffsets.pos.y = maxPosOffset.y * (util::PerlinNoise(1.5f, time) * 2.0f) * shake;
    appliedOffsets.pos.z = maxPosOffset.z * (util::PerlinNoise(2.5f, time) * 2.0f) * shake;
    appliedOffsets.rot.x = maxRotOffset.x * (util::PerlinNoise(3.5f, time) * 2.0f) * shake;
    appliedOffsets.rot.y = maxRotOffset.y * (util::PerlinNoise(4.5f, time) * 2.0f) * shake;
    appliedOffsets.rot.z = maxRotOffset.z * (util::PerlinNoise(5.5f, time) * 2.0f) * shake;

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

void ShakeComponent::EditorDraw()
{
    gui::TextBoxReadOnly("Trauma", std::to_string(trauma));
    gui::VarDrag("Shake Speed", &frequency, 1.0f, 0.0f, 100.0f, "%.1f");
    gui::VarDrag("Recovery Speed", &recoverySpeed, 0.05f, 0.1f, 10.0f, "%.1f");
    gui::VarDrag("Falloff Exponent", &traumaExponent, 0.2f, 1.0f, 10.0f, "%.1f");

    gui::Separator();

    gui::VarDrag("Max Position Offset", &maxPosOffset);
    gui::VarDrag("Max Rotation Offset", &maxRotOffset);

    if (gui::Button{ "Test Shake" })
        InduceStress(1.0f);
}
