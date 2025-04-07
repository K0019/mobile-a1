/******************************************************************************/
/*!
\file   ArmPivot.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	This file contains the declaration for Entity component ArmPivotComponent.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

/*****************************************************************//*!
\class WeaponComponent
\brief
	Identifies an entity as a Weapon.
*//******************************************************************/
class ArmPivotComponent
	: public IRegisteredComponent<ArmPivotComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Rotates the attached Entity to face towards the input posiiton.
	\param targetPosition
		The position to look towards.
	*//******************************************************************/
	void RotateTowards(const Vector2& targetPosition);

	property_vtable()
};
property_begin(ArmPivotComponent)
{
}
property_vend_h(ArmPivotComponent)


