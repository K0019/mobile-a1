/******************************************************************************/
/*!
\file   Bar.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (50%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Matthew Chan Shao Jie (50%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This file contains the definition for functions in the the Bar component.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Bar.h"

BarComponent::BarComponent()
    : maxValue{ 0.0f }
    , currValue{ 0.0f }
{
}

void BarComponent::SetCurr(float value)
{
    currValue = value;
    currValue = std::clamp(currValue, 0.0f, maxValue);
    UpdateTransforms();
}

float BarComponent::GetCurr() const
{
    return currValue;
}

void BarComponent::SetMax(float value)
{
    maxValue = value;
    UpdateTransforms();
}

void BarComponent::UpdateTransforms()
{
    // Get the entity
    ecs::EntityHandle entity = ecs::GetEntity(this);

    // Get transforms for this and parent
    Transform* thisTransform = &entity->GetTransform();

    // Set transform scale and position
    thisTransform->SetLocalScale({ currValue / maxValue, 1.0f, 1.0f });
    thisTransform->SetLocalPosition({ -(maxValue - currValue) / maxValue / 2.0f, 0.0f, 0.0f });
}

void BarComponent::SetColor(const Vec4& color)
{
    // Guard clause if renderComponent is null
    if (ecs::CompHandle<SpriteComponent> renderComponent = ecs::GetEntity(this)->GetComp<SpriteComponent>())
    {
        // Set color
        renderComponent->SetColor(color);
    }
    else
    {
        CONSOLE_LOG_EXPLICIT("Missing Render Component on Bar Entity!", LogLevel::LEVEL_ERROR);
    }
}

void BarComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    ImGui::SliderFloat("Current", &currValue, 0.0f, maxValue);
    ImGui::InputFloat("Maximum", &maxValue);

    UpdateTransforms();
#endif
}