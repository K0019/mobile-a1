/******************************************************************************/
/*!
\file   LightingSystem.h
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
  This is an interface file for light systems.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "LightComponent.h"

class Renderer;
/*****************************************************************//*!
\class LightingSystem
\brief
    Uploads light objects from the scene to the render pipeline.
*//******************************************************************/
class LightingSystem :public ecs::System<LightingSystem, LightComponent>
{
public:
    LightingSystem();
private:
    Renderer* renderer;
    void UpdateLightComp(LightComponent& lightComp);
};

/*****************************************************************//*!
\class LightBlinkSystem
\brief
    Updates the intensity and radius of light objects based on blink components.
*//******************************************************************/
class LightBlinkSystem : public ecs::System<LightBlinkSystem, LightBlinkComponent, LightComponent>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    LightBlinkSystem();

private:
    /*****************************************************************//*!
    \brief
        Updates a light and blink component.
    \param blinkComp
        The blink component.
    \param lightComp
        The light component.
    *//******************************************************************/
    void UpdateComp(LightBlinkComponent& blinkComp, LightComponent& lightComp);
};