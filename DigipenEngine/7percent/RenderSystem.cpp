/******************************************************************************/
/*!
\file   RenderSystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of RenderSystem.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "RenderSystem.h"

#include "Engine.h"
RenderSystem::RenderSystem() : SystemOperatingByLayer(&RenderSystem::DrawRenderComp)
{
	renderer = ST<Engine>::Get()->_vulkan->_renderer.get();
}
void RenderSystem::DrawRenderComp(RenderComponent& renderComp)
{

	renderer->AddRenderInstance(renderComp);
}
