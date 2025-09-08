/******************************************************************************/
/*!
\file   KillWhenAnimationFinish.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   03/11/2025

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief

	Deletes the animation entity when the animation finishes


All content ? 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "pch.h"
#include "KillAnimationWhenFinish.h"
#include "AnimatorComponent.h"

KillWhenAnimationFinishComponent::KillWhenAnimationFinishComponent()
{
}

KillWhenAnimationFinishComponent::~KillWhenAnimationFinishComponent()
{
}

AnimationFinishSystem::AnimationFinishSystem()
	: System_Internal{ &AnimationFinishSystem::Update }
{
}

void AnimationFinishSystem::Update(KillWhenAnimationFinishComponent& comp)
{
	if (!ecs::GetEntity(&comp)->GetComp<AnimatorComponent>()->IsPlaying())
	{
		ecs::DeleteEntity(ecs::GetEntity(&comp));
	}
}
