/******************************************************************************/
/*!
\file   Bar.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/27/2024

\author Chan Kuan Fu Ryan (50%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Matthew Chan Shao Jie (50%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This file contains the declaration for the Bar component, which handles
  displaying values in a bar form.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "RenderSystem.h"

/*****************************************************************//*!
\class BarComponent
\brief
	Identifies an entity as being a UI Bar.
*//******************************************************************/
class BarComponent : public IRegisteredComponent<BarComponent>
#ifdef IMGUI_ENABLED
	, IEditorComponent<BarComponent>
#endif
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	BarComponent();

	/*****************************************************************//*!
	\brief
		Sets the current value of the bar.
	\param value
		The new current value of the bar.
	*//******************************************************************/
	void SetCurr(float value);

	/*****************************************************************//*!
	\brief
		Gets the current value of the bar.
	\return
		The current value of the bar.
	*//******************************************************************/
	float GetCurr() const;

	/*****************************************************************//*!
	\brief
		Sets the maximum value of the bar.
	\param value
		The new max value of the bar.
	*//******************************************************************/
	void SetMax(float value);

	/*****************************************************************//*!
	\brief
		Updates the bar's transforms to reflect the current and maximum
		values.
	*//******************************************************************/
	void UpdateTransforms();

	/*****************************************************************//*!
	\brief
		Changes the color of the bar.
	\param color
		The new color of the bar.
	*//******************************************************************/
	void SetColor(const Vector4& value);

private:
#ifdef IMGUI_ENABLED
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	\param comp
		The component instance.
	*//******************************************************************/
	static void EditorDraw(BarComponent& comp);
#endif
	
	float maxValue;//! Bar maximum value
	float currValue;//! Bar current value

	property_vtable()
};
property_begin(BarComponent)
{
	property_var(maxValue),
	property_var(currValue)
}
property_vend_h(BarComponent)
