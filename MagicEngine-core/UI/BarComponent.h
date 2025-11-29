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
#include "Editor/IEditorComponent.h"
#include "UI/IUIComponent.h"
#include "Game/IGameComponentCallbacks.h"

class BarComponent
	: public IRegisteredComponent<BarComponent>
	, public IEditorComponent<BarComponent>
	, public IUIComponent<BarComponent>
{
public:
	BarComponent();

	void SetPercentageFilled(float percent);

	void OnAttached() override;
	void EditorDraw() override;

private:
	bool modifyUV;

public:
	property_vtable()
};
property_begin(BarComponent)
{
	property_var(modifyUV)
}
property_vend_h(BarComponent)
