/******************************************************************************/
/*!
\file   EventsQueue.cpp
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

#include "UI/HealthBarComponent.h"
#include "UI/BarComponent.h"
#include "Game/Health.h"
#include "Engine/EntityEvents.h"

void HealthBarComponent::OnStart()
{
	ecs::CompHandle<HealthComponent> healthComp{ entWithHealth ? entWithHealth->GetComp<HealthComponent>() : nullptr };
	if (!healthComp)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "HealthBarComponent has no assigned entity that has a HealthComponent! Deleting the health bar entity";
		ecs::DeleteEntity(ecs::GetEntity(this));
		return;
	}

	ecs::CompHandle<BarComponent> barComp{ ecs::GetEntity(this)->GetCompInChildren<BarComponent>() };
	if (!barComp)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "HealthBarComponent could not find a BarComponent in children! Deleting the health bar entity";
		ecs::DeleteEntity(ecs::GetEntity(this));
		return;
	}

	entWithBar = ecs::GetEntity(barComp);
	entWithHealth->GetComp<EntityEventsComponent>()->Subscribe("OnHealthChanged", this, &HealthBarComponent::UpdateBarPercentage);
}

void HealthBarComponent::OnDetached()
{
	if (entWithHealth)
		if (auto eventsComp{ entWithHealth->GetComp<EntityEventsComponent>() })
			eventsComp->Unsubscribe("OnHealthChanged", this, &HealthBarComponent::UpdateBarPercentage);
}

void HealthBarComponent::EditorDraw()
{
	entWithHealth.EditorDraw("Entity w/ Health");
}

void HealthBarComponent::UpdateBarPercentage(float healthPercentage)
{
	entWithBar->GetComp<BarComponent>()->SetPercentageFilled(healthPercentage);
}
