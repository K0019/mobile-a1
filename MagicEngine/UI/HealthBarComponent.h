/******************************************************************************/
/*!
\file   EventsQueue.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides a buffer to queue events for event handlers to later pull.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "ECS/EntityUID.h"

class HealthBarComponent
	: public IRegisteredComponent<HealthBarComponent>
	, public IEditorComponent<HealthBarComponent>
	, public IGameComponentCallbacks<HealthBarComponent>
{
public:
	void OnStart() override;
	void OnDetached() override;

	void EditorDraw() override;

private:
	void UpdateBarPercentage(float healthPercentage);

private:
	EntityReference entWithHealth;
	ecs::EntityHandle entWithBar;

public:
	property_vtable()
};
property_begin(HealthBarComponent)
{
	property_var(entWithHealth)
}
property_vend_h(HealthBarComponent)
