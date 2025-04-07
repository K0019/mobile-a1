#pragma once

/******************************************************************************/
/*!
\file   RenderSystem.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
System that updates the Renderer with the data provided by the RenderComponent.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "RenderComponent.h"
#include "EntityLayers.h"

class Renderer;

class RenderSystem : public ecs::SystemOperatingByLayer<RenderSystem, RenderComponent>
{
public:
  explicit RenderSystem();
private:
  Renderer* renderer;
  void DrawRenderComp(RenderComponent& renderComp);
};

