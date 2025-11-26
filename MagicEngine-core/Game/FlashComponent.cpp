/******************************************************************************/
/*!
\file   GameCameraController.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	GameCameraController is an ECS component-system pair which takes control of
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the player, map bounds, and any other such effects to be
	implemented now or in the future.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "FlashComponent.h"
#include "Graphics/RenderComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Utilities/GameTime.h"

FlashComponent::FlashComponent()
	: flashTime{0.0f}
	, currentFlashTime{ 0.0f }
{
}

void FlashComponent::Flash()
{
	currentFlashTime = flashTime;
}

void FlashComponent::EditorDraw()
{
	gui::VarInput("Flash Time", &flashTime);

	const std::string* materialText{ ST<MagicResourceManager>::Get()->Editor_GetName(flashMaterial.GetHash()) };

	gui::TextUnformatted("Material");
	gui::TextBoxReadOnly("##", materialText ? materialText->c_str() : "");
	gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
		flashMaterial = hash;
		});
}

FlashComponentSystem::FlashComponentSystem()
	: System_Internal{ &FlashComponentSystem::UpdateFlashComponent }

{
}

void FlashComponentSystem::UpdateFlashComponent(FlashComponent& comp)
{
	ecs::EntityHandle thisEntity{ ecs::GetEntity(&comp) };
	if (ecs::CompHandle<RenderComponent> renderComp{ thisEntity->GetComp<RenderComponent>() })
	{
		bool currentFlashState = comp.currentFlashTime > 0.0f;
		if (currentFlashState)
			comp.currentFlashTime -= GameTime::Dt();
		if (currentFlashState != comp.isFlashing)
		{
			if (comp.defaultMaterials.size() == 0)
			{
				for (auto material : renderComp->GetMaterialsList())
					comp.defaultMaterials.push_back(material);
			}

			int matIndex = 0;
			for (auto& material : renderComp->GetMaterialsList())
			{
				material = currentFlashState ? comp.flashMaterial : comp.defaultMaterials[matIndex];
				++matIndex;
			}
			comp.isFlashing = currentFlashState;
		}
	}
}
