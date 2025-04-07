/******************************************************************************/
/*!
\file   ComponentLookupWorkaround.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   01/31/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This file is here to provide dummy components to make searching for objects easier for the moment.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "IRegisteredComponent.h"
#pragma region Arm Pivot
/*****************************************************************//*!
\class ArmPivot
\brief
	Exists to exist so that the Player can get the arm pivot entity easily
*//******************************************************************/
class ArmPivot : public IRegisteredComponent<ArmPivot>
{
private:
	property_vtable()
};
property_begin(ArmPivot)
{
}
property_vend_h(ArmPivot)
#pragma endregion
#pragma region Raycast Debug
/*****************************************************************//*!
\class RaycastDebug
\brief
	Exists to exist
*//******************************************************************/
class RaycastDebug : public IRegisteredComponent<RaycastDebug>
{
private:
	property_vtable()
};
property_begin(RaycastDebug)
{
}
property_vend_h(RaycastDebug)
/*****************************************************************//*!
\class PlayerArm
\brief
	Exists to exist
*//******************************************************************/
class PlayerArm : public IRegisteredComponent<PlayerArm>
{
private:
	property_vtable()
};
property_begin(PlayerArm)
{
}
property_vend_h(PlayerArm)

/*****************************************************************//*!
\class Sliderbar
\brief
	Exists to exist
*//******************************************************************/
class SliderBar : public IRegisteredComponent<SliderBar>
{
private:
	property_vtable()
};
property_begin(SliderBar)
{
}
property_vend_h(SliderBar)



#pragma endregion
#pragma once
