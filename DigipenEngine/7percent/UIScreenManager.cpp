/******************************************************************************/
/*!
\file   UIScreenManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   03/09/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a management system for which UI screen is shown
  at a time.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/


#include "pch.h"
#include "NameComponent.h"
#include "UIScreenManager.h"

UIScreenManagerComponent::UIScreenManagerComponent() :
#ifdef IMGUI_ENABLED
	REGISTER_DRAW_FUNCTION_TO_EDITOR(UIScreenManagerComponent::EditorDraw),
#endif
	currScreen{}
	, prevScreen{}
{
}

void UIScreenManagerComponent::SwitchToScreen(const std::string& name)
{
	// Assume the first screen in the list is the one showing when we're doing a screen switch for the first time.
	if (!currScreen)
		currScreen = prevScreen = screens[0];

	for (EntityReference& screen : screens)
		if (screen->GetComp<NameComponent>()->GetName() == name)
		{
			// If the selected screen is already being shown, do nothing.
			if (currScreen == screen)
				return;

			// Switch to the new screen.
			currScreen->SetActive(false);
			prevScreen = currScreen;
			currScreen = screen;
			currScreen->SetActive(true);
			return;
		}
}

void UIScreenManagerComponent::SwitchToPrevScreen()
{
	// If we've not initiated any screen switch previously, do nothing.
	if (!prevScreen || currScreen == prevScreen)
		return;

	currScreen->SetActive(false);
	std::swap(currScreen, prevScreen);
	currScreen->SetActive(true);
}

#ifdef IMGUI_ENABLED
void UIScreenManagerComponent::EditorDraw(UIScreenManagerComponent& comp)
{
	gui::VarContainer("Screens", &comp.screens, [](EntityReference& screenEntity) -> bool {
		return screenEntity.EditorDraw("");
	});
}
#endif

void UIScreenManagerComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("screens", screens);
}

void UIScreenManagerComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("screens", &screens);
}
