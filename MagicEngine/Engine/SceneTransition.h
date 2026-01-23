/******************************************************************************/
/*!
\file   SceneTransition.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 5
\date   11/01/2026

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a set of classes that implement a scene
  transition system.

All content © 2026 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ECS/ECS.h"
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"

// Marks an entity as the black screen entity responsible for fading the screen out
class SceneTransitionComponent
	: public IRegisteredComponent<SceneTransitionComponent>
	, public IEditorComponent<SceneTransitionComponent>
{
public:
	SceneTransitionComponent();

	// Updates elapsed time and returns the current percentage black. Does not update the sprite comp color.
	float TransitionToBlack(float dt);
	float TransitionToClear(float dt);

	void EditorDraw() override;

private:
	float fadeLength;
	float elapsedTime;

public:
	property_vtable()
};
property_begin(SceneTransitionComponent)
{
	property_var(fadeLength)
}
property_vend_h(SceneTransitionComponent)

class SceneTransitionSystem : public ecs::System<SceneTransitionSystem, SceneTransitionComponent>
{
public:
	SceneTransitionSystem();

	bool PreRun() override;

private:
	void UpdateTransition(SceneTransitionComponent& transitionComp);

private:
	bool isTransitioning;
	bool isTransitioningToBlack;
	bool transitionedLastFrame; // To skip the frame dt of the frame where we load the new scene
	int transitionSceneIndex;
	std::string toScenePath;
};