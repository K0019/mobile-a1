/******************************************************************************/
/*!
\file   LayersMatrix.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   01/15/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This header file declares an editor window class rendering an interface that allows
  the user to customize which entity layers collide with which other layers.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "GUICollection.h"

/*****************************************************************//*!
\class LayersMatrix
\brief
	Entity layers collision matrix window.
*//******************************************************************/
class LayersMatrix : public gui::Window
{
private:
	/*****************************************************************//*!
	\enum class CONTENT_TYPE
	\brief
		Identifies the tabs that the user can view.
	*//******************************************************************/
	enum class CONTENT_TYPE
	{
		COLLISION_MATRIX,
		SYSTEMS_LAYERS,
	};

public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	LayersMatrix();

protected:
	/*****************************************************************//*!
	\brief
		Draws the layer matrix.
	*//******************************************************************/
	void DrawContents() override;

private:
	/*****************************************************************//*!
	\brief
		Draws the collision layer matrix.
	*//******************************************************************/
	void DrawCollisionLayerMatrix();

	/*****************************************************************//*!
	\brief
		Draws the systems layers matrix.
	*//******************************************************************/
	void DrawSystemsLayersMatrix();

private:
	//! Which tab has the user selected.
	CONTENT_TYPE currentContent;

};

