/******************************************************************************/
/*!
\file   ArmPivot.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	This file contains the definition for functions in ArmPivotComponent.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "ArmPivot.h"

ArmPivotComponent::ArmPivotComponent()
#ifdef IMGUI_ENABLED
	: REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw)
#endif
{
}

#ifdef IMGUI_ENABLED
void ArmPivotComponent::EditorDraw(ArmPivotComponent& comp)
{
	UNREFERENCED_PARAMETER(comp);
}
#endif

void ArmPivotComponent::RotateTowards(const Vector2& targetPosition)
{
	Transform& thisTransform = ecs::GetEntityTransform(this);

	Vector2 directionToAim = targetPosition - thisTransform.GetWorldPosition();

	if (/*ecs::CompHandle<FlipperComponent>*/ Transform * flipper{ thisTransform.GetParent() })
	{
		flipper->SetLocalScale({ math::Sign(directionToAim.x),1.0f });
	}

	thisTransform.SetLocalRotation(math::ToDegrees(atan2(directionToAim.y, directionToAim.x * math::Sign(thisTransform.GetWorldScale().x))));
}