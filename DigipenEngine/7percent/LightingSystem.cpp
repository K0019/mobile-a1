/******************************************************************************/
/*!
\file   LightingSystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   01/15/2025

\author Ryan Cheong (60%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (40%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for light systems.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "LightingSystem.h"
#include "Engine.h"

LightingSystem::LightingSystem() : System_Internal(&LightingSystem::UpdateLightComp) {
renderer = ST<Engine>::Get()->_vulkan->_renderer.get();
}


void LightingSystem::UpdateLightComp(LightComponent& lightComp) {
  renderer->AddLightInstance(lightComp);
}

LightBlinkSystem::LightBlinkSystem()
	: System_Internal{ &LightBlinkSystem::UpdateComp }
{
}

void LightBlinkSystem::UpdateComp(LightBlinkComponent& blinkComp, LightComponent& lightComp)
{
	Vector2 newLightProperties{ blinkComp.AddTimeElapsed(GameTime::FixedDt()) };
	lightComp.intensity = newLightProperties.x;
	lightComp.radius = newLightProperties.y;
}
