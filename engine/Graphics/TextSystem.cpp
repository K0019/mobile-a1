/******************************************************************************/
/*!
\file   TextSystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
definition of text system

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TextSystem.h"

#include "Engine.h"
#include "ryan-c/Renderer.h"

TextSystem::TextSystem()
    : System_Internal{ &TextSystem::DrawTextComp }
{
    renderer = ST<Engine>::Get()->_vulkan->_renderer.get();
}

void TextSystem::DrawTextComp(TextComponent& textComp)
{
    textComp.CalculateWorldTransform();
    renderer->AddTextInstance(textComp);
}

FPSTextSystem::FPSTextSystem()
    : System_Internal{ &FPSTextSystem::UpdateFPSText }
    , doToggle{ false }
{
}

bool FPSTextSystem::PreRun()
{
    // Clear stringstream and update with new FPS
    ss.str(std::string{});
    ss << "FPS: " << GameTime::Fps();

    // Whether to flip the doDisplay bool
    doToggle = Input::GetKeyPressed(KEY::F10);

    return true;
}

void FPSTextSystem::UpdateFPSText(FPSTextComponent& comp)
{
    if (doToggle)
        comp.SetDoDisplay(!comp.GetDoDisplay());

    if (ecs::CompHandle<TextComponent> textComp{ ecs::GetEntity(&comp)->GetComp<TextComponent>() })
        textComp->SetText(comp.GetDoDisplay() ? ss.str() : "");
}
