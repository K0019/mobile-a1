/******************************************************************************/
/*!
\file   LayersMatrix.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   01/15/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This source file implements an editor window class rendering an interface that allows
  the user to customize which entity layers collide with which other layers.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "LayersMatrix.h"
#include "EntityLayers.h"

LayersMatrix::LayersMatrix()
	: gui::Window{ "Layers Matrix", gui::Vec2{ 768.0f, 300.0f } }
	, currentContent{ CONTENT_TYPE::COLLISION_MATRIX }
{
}

void LayersMatrix::DrawContents()
{
	if (gui::Button{ "Collision" })
		currentContent = CONTENT_TYPE::COLLISION_MATRIX;
	gui::SameLine();
	if (gui::Button{ "Systems" })
		currentContent = CONTENT_TYPE::SYSTEMS_LAYERS;

	gui::Separator();

	switch (currentContent)
	{
	case CONTENT_TYPE::COLLISION_MATRIX:
		DrawCollisionLayerMatrix();
		break;
	case CONTENT_TYPE::SYSTEMS_LAYERS:
		DrawSystemsLayersMatrix();
		break;
	}

}

void LayersMatrix::DrawCollisionLayerMatrix()
{
	if (gui::Table table{ "##LayersMatrix", +ENTITY_LAYER::TOTAL + 1, true, gui::FLAG_TABLE::SIZING_FIXED_SAME | gui::FLAG_TABLE::ROW_BG })
	{
		// Header row
		table.AddColumnHeader("##", gui::FLAG_TABLE_COLUMN::WIDTH_STRETCH);
		for (ENTITY_LAYER i{ static_cast<ENTITY_LAYER>(0) }; i < ENTITY_LAYER::TOTAL; ++i)
			table.AddColumnHeader(EntityLayerComponent::GetLayerName(i));
		table.SubmitColumnHeaders();

		// For each layer row
		bool collidesFlag{};
		int id{};
		for (ENTITY_LAYER i{ static_cast<ENTITY_LAYER>(0) }; i < ENTITY_LAYER::TOTAL; ++i)
		{
			gui::TextUnformatted(EntityLayerComponent::GetLayerName(i));
			table.NextColumn();

			for (ENTITY_LAYER j{ static_cast<ENTITY_LAYER>(0) }; j < ENTITY_LAYER::TOTAL; ++j)
			{
				gui::SetID setId{ ++id };

				collidesFlag = EntityLayersMask::TestMatrix(i, j);
				if (gui::Checkbox("##", &collidesFlag))
					EntityLayersMask::SetMatrix(i, j, collidesFlag);
				table.NextColumn();
			}
		}
	}
}

void LayersMatrix::DrawSystemsLayersMatrix()
{
	if (gui::Table table{ "##LayersMatrix", +ENTITY_LAYER::TOTAL + 1, true, gui::FLAG_TABLE::SIZING_FIXED_SAME | gui::FLAG_TABLE::ROW_BG })
	{
		// Header row
		table.AddColumnHeader("##", gui::FLAG_TABLE_COLUMN::WIDTH_STRETCH);
		for (ENTITY_LAYER i{ static_cast<ENTITY_LAYER>(0) }; i < ENTITY_LAYER::TOTAL; ++i)
			table.AddColumnHeader(EntityLayerComponent::GetLayerName(i));
		table.SubmitColumnHeaders();

		// Sort systems by name
		auto sortedSystemTypeMeta{ util::ToSortedVectorOfRefs<ecs::SysHash, ecs::SysTypeMeta>(ecs::GetSysMetaBegin(), ecs::GetSysMetaEnd(), [](const auto& a, const auto& b) -> bool {
			return a.second.get().name < b.second.get().name;
		}) };

		// For each system (row)
		bool isActive{};
		int id{};
		for (const auto& systemTypeMeta : sortedSystemTypeMeta)
		{
			gui::TextUnformatted(systemTypeMeta.second.get().name);
			table.NextColumn();

			isActive = false; // Later if this system supports layers, this will be overriden.
			auto getLayerMaskFunc = ST<ecs::RegisteredSystemsOperatingByLayer>::Get()->GetLayerMaskFunc(systemTypeMeta.first);
			gui::Disabled disabled{ !getLayerMaskFunc }; // Whether the buttons are enabled

			for (ENTITY_LAYER i{ static_cast<ENTITY_LAYER>(0) }; i < ENTITY_LAYER::TOTAL; ++i)
			{
				gui::SetID setId{ ++id };

				if (getLayerMaskFunc)
					isActive = getLayerMaskFunc().TestMask(i);
				if (gui::Checkbox("##", &isActive))
					// If this gets activated, this system should have support for layers
					getLayerMaskFunc().SetMask(i, isActive);
				table.NextColumn();
			}
		}
	}
}
