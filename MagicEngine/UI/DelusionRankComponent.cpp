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

#include "UI/DelusionRankComponent.h"
#include "Game/Delusion.h"
#include "UI/SpriteComponent.h"
#include "Engine/EntityEvents.h"
#include "Editor/Containers/GUICollection.h"

#if __has_include("Editor/EditorUtilResource.h")
#include "Editor/EditorUtilResource.h"
#endif

void DelusionRankComponent::OnStart()
{
	ecs::CompHandle<DelusionComponent> delusionComp{ entWithDelusion ? entWithDelusion->GetComp<DelusionComponent>() : nullptr };
	if (!delusionComp)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "DelusionRankComponent has no assigned entity that has a DelusionComponent! Deleting the delusion rank entity";
		ecs::DeleteEntity(ecs::GetEntity(this));
		return;
	}

	entWithDelusion->GetComp<EntityEventsComponent>()->Subscribe("OnDelusionTierChanged", this, &DelusionRankComponent::UpdateDelusionRank);
}

void DelusionRankComponent::OnDetached()
{
	if (entWithDelusion)
		if (auto eventsComp{ entWithDelusion->GetComp<EntityEventsComponent>() })
			eventsComp->Unsubscribe("OnDelusionTierChanged", this, &DelusionRankComponent::UpdateDelusionRank);
}

void DelusionRankComponent::EditorDraw()
{
	if (!ecs::GetEntity(this)->HasComp<SpriteComponent>())
	{
		gui::SetStyleColor textColor{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 1.0f, 0.2f, 0.2f, 1.0f } };
		gui::TextWrapped("SpriteComponent IS REQUIRED for DelusionRankComponent to work. Please attach a SpriteComponent.");
		gui::Separator();
	}

	entWithDelusion.EditorDraw("Entity w/ Delusion");

#ifdef EDITOR_UTIL_RESOURCE
	for (DELUSION_TIER tier{}; tier < DELUSION_TIER::TOTAL; ++tier)
		editor::EditorUtil_DrawResourceHandle(DelusionComponent::TierToString(tier).c_str(), rankTextures[+tier]);
#endif
}

void DelusionRankComponent::UpdateDelusionRank(DELUSION_TIER rank)
{
	// Switch sprite texture based on the rank
	if (auto spriteComp{ ecs::GetEntity(this)->GetComp<SpriteComponent>() })
		spriteComp->VisitPrimitive([texture = rankTextures[+rank]]([[maybe_unused]] auto& primitive) -> void {
			using T = std::decay_t<decltype(primitive)>;
			if constexpr (std::is_same_v<T, Primitive2DImage>)
				primitive.SetImage(texture.GetHash());
		});
}
