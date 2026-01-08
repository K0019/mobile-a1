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
#include "Game/Delusion.h"
#include "ECS/EntityUID.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

class DelusionRankComponent
	: public IRegisteredComponent<DelusionRankComponent>
	, public IEditorComponent<DelusionRankComponent>
	, public IGameComponentCallbacks<DelusionRankComponent>
{
public:
	void OnStart() override;
	void OnDetached() override;

	void EditorDraw() override;

private:
	void UpdateDelusionRank(DELUSION_TIER rank);

private:
	EntityReference entWithDelusion;
	std::array<UserResourceHandle<ResourceTexture>, +DELUSION_TIER::TOTAL> rankTextures;

public:
	property_vtable()
};
property_begin(DelusionRankComponent)
{
	property_var(entWithDelusion),
	property_var(rankTextures)
}
property_vend_h(DelusionRankComponent)
