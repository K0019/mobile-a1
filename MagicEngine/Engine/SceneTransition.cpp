/******************************************************************************/
/*!
\file   SceneTransition.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 5
\date   11/01/2026

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing a set of classes that implement a scene
  transition system.

All content © 2026 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "SceneTransition.h"
#include "Events/EventsQueue.h"
#include "Events/EventsTypeBasic.h"
#include "Engine/SceneManagement.h"
#include "UI/SpriteComponent.h"
#include "VFS/VFS.h"
#include "FilepathConstants.h"
#include "Editor/Containers/GUICollection.h"

SceneTransitionComponent::SceneTransitionComponent()
	: fadeLength{}
	, elapsedTime{}
{
}

float SceneTransitionComponent::TransitionToBlack(float dt)
{
	elapsedTime += dt;
	if (elapsedTime > fadeLength)
		elapsedTime = fadeLength;

	return elapsedTime / fadeLength;
}

float SceneTransitionComponent::TransitionToClear(float dt)
{
	elapsedTime -= dt;
	if (elapsedTime < 0.0f)
		elapsedTime = 0.0f;

	return elapsedTime / fadeLength;
}

void SceneTransitionComponent::EditorDraw()
{
	gui::VarDefault("Fade Length", &fadeLength);
}

SceneTransitionSystem::SceneTransitionSystem()
	: System_Internal{ &SceneTransitionSystem::UpdateTransition }
	, isTransitioning{ false }
	, isTransitioningToBlack{ false }
	, transitionedLastFrame{ false }
	, transitionSceneIndex{ -1 }
{
}

bool SceneTransitionSystem::PreRun()
{
	EventsReader<Events::TransitionScene> eventsReader{};
	while (auto event{ eventsReader.ExtractEvent() })
	{
		// Ignore event if already transitioning
		if (isTransitioning)
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Cannot transition scene to " << event->toScenePath << " while already transitioning to scene " << toScenePath << "!";
			continue;
		}

		// Assert: Transition scene comp should not exist
		assert(ecs::GetCompsBegin<SceneTransitionComponent>() == ecs::GetCompsEnd<SceneTransitionComponent>());

		isTransitioning = isTransitioningToBlack = true;
		toScenePath = event->toScenePath;

		// Load transition scene
		transitionSceneIndex = ST<SceneManager>::Get()->LoadScene(VFS::JoinPath(Filepaths::scenesSave, "transition.scene"), false);
		// Not flushing here incurs an additional one frame delay to the start of transition I think, but let's stay on the safe side and wait for the engine flush
	}

	return isTransitioning;
}

void SceneTransitionSystem::UpdateTransition(SceneTransitionComponent& transitionComp)
{
	// Get the UI element
	auto spriteComp{ ecs::GetEntity(&transitionComp)->GetComp<SpriteComponent>() };
	if (!spriteComp)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Entity with SceneTransitionComponent is missing a SpriteComponent!";
		return;
	}
	Vec4 color{ spriteComp->GetColor() };

	if (isTransitioningToBlack)
	{
		// Real DT so we don't get affected by timescale
		color.a = transitionComp.TransitionToBlack(GameTime::RealDt());
		spriteComp->SetColor(color);

		if (color.a >= 0.995f)
		{
			isTransitioningToBlack = false;
			transitionedLastFrame = true;
			ST<SceneManager>::Get()->UnloadScene(ST<SceneManager>::Get()->GetActiveScene()->GetIndex());
			ST<SceneManager>::Get()->LoadScene(toScenePath);
		}
	}
	else
	{
		if (transitionedLastFrame)
		{
			transitionedLastFrame = false;
			return;
		}

		color.a = transitionComp.TransitionToClear(GameTime::RealDt());
		spriteComp->SetColor(color);

		if (color.a <= 0.005f)
		{
			ST<SceneManager>::Get()->UnloadScene(transitionSceneIndex);
			isTransitioning = false;
		}
	}
}
