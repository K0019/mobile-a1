/******************************************************************************/
/*!
\file   FadeAndDie.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   03/24/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a component that fades out render components
  until they are fully transparent, at which point it deletes the entity.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "pch.h"
#include "FadeAndDie.h"
#include "RenderComponent.h"

FadeAndDieComponent::FadeAndDieComponent() :
#ifdef IMGUI_ENABLED
	REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
	delayTimer{ 1.0f }
	, delayRandomOffset{ 0.4f }
	, fadeLength{ 1.0f }
{
}

void FadeAndDieComponent::Update(float dt)
{
	bool stillAlive{ false };
	auto renderComps{ ecs::GetEntity(this)->GetCompInChildrenVec<RenderComponent>() };
	if (auto render = ecs::GetEntity(this)->GetComp<RenderComponent>())
		renderComps.push_back(render);
	for (size_t i{}, end{ renderComps.size() }; i < end; ++i)
	{
		if (remainingLengths.size() <= i)
			remainingLengths.push_back(delayTimer + util::RandomRangeFloat(-delayRandomOffset, delayRandomOffset) + fadeLength);

		remainingLengths[i] -= dt;
		if (remainingLengths[i] > 0.0f)
			stillAlive = true;
		if (remainingLengths[i] > fadeLength)
			continue;

		Vector4 color{ renderComps[i]->GetColor()};
		color.w = remainingLengths[i] / fadeLength;
		renderComps[i]->SetColor(color);
	}

	if (!stillAlive)
		ecs::DeleteEntity(ecs::GetEntity(this));
}

void FadeAndDieComponent::EditorDraw(FadeAndDieComponent& comp)
{
	gui::VarDrag("Delay", &comp.delayTimer);
	gui::VarDrag("Delay Randomness", &comp.delayRandomOffset);
	gui::VarDrag("Fade Length", &comp.fadeLength);
}

FadeAndDieSystem::FadeAndDieSystem()
	: System_Internal{ &FadeAndDieSystem::UpdateComp }
{
}

void FadeAndDieSystem::UpdateComp(FadeAndDieComponent& comp)
{
	comp.Update(GameTime::FixedDt());
}
