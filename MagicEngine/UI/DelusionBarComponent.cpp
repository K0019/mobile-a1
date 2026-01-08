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

#include "UI/DelusionBarComponent.h"
#include "Game/Delusion.h"
#include "UI/BarComponent.h"
#include "Engine/EntityEvents.h"

void DelusionBarComponent::OnStart()
{
	ecs::CompHandle<DelusionComponent> delusionComp{ entWithDelusion ? entWithDelusion->GetComp<DelusionComponent>() : nullptr };
	if (!delusionComp)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "DelusionBarComponent has no assigned entity that has a DelusionComponent! Deleting the delusion bar entity";
		ecs::DeleteEntity(ecs::GetEntity(this));
		return;
	}

	ecs::CompHandle<BarComponent> barComp{ ecs::GetEntity(this)->GetCompInChildren<BarComponent>() };
	if (!barComp)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "DelusionBarComponent could not find a BarComponent in children! Deleting the delusion bar entity";
		ecs::DeleteEntity(ecs::GetEntity(this));
		return;
	}

	entWithBar = ecs::GetEntity(barComp);
	entWithDelusion->GetComp<EntityEventsComponent>()->Subscribe("OnDelusionChanged", this, &DelusionBarComponent::UpdateBarPercentage);

	// Delusion starts at 0%
	barComp->SetPercentageFilled(0.0f);
}

void DelusionBarComponent::OnDetached()
{
	if (entWithDelusion)
		if (auto eventsComp{ entWithDelusion->GetComp<EntityEventsComponent>() })
			eventsComp->Unsubscribe("OnDelusionChanged", this, &DelusionBarComponent::UpdateBarPercentage);
}

void DelusionBarComponent::EditorDraw()
{
	entWithDelusion.EditorDraw("Entity w/ Delusion");
}

void DelusionBarComponent::UpdateBarPercentage(float delusionPercentage)
{
	entWithBar->GetComp<BarComponent>()->SetPercentageFilled(delusionPercentage);
}
