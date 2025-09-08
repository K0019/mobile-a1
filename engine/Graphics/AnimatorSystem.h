#pragma once
/******************************************************************************/
/*!
\file   AnimatorSystem.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Class that updates the AnimatorComponent.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "AnimatorComponent.h"

class AnimatorSystem : public ecs::System<AnimatorSystem, AnimatorComponent>
{
public:
  explicit AnimatorSystem();
private:
  void UpdateAnimatorComp(AnimatorComponent& animatorComp);
};

