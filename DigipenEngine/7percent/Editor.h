/******************************************************************************/
/*!
\file   Editor.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the Editor class.
The Editor class is responsible for processing input and drawing the user interface for the game editor.
It also maintains the state of the editor, such as the selected entity and component.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Console.h"
#include "CustomViewport.h"
#include "EditorTweenModule.h"
#include "Gizmo.h"

class Editor : public gui::Window
{
#ifdef IMGUI_ENABLED
public:
	Editor();
	~Editor();

	/**
	* \brief Processes the user input for the editor.
	*/
	void ProcessInput();

	void DrawContainer() override;

	/**
	* \brief Draws the user interface for the editor.
	*/
	void DrawContents() override;

	/*****************************************************************//*!
	\brief
		Creates an entity and selects it.
	*//******************************************************************/
	void CreateEntityAndSelect();

	/*****************************************************************//*!
	\brief
		Unselects the selected entity.
	*//******************************************************************/
	void ForceUnselectEntity();

	/*****************************************************************//*!
	\brief
		Deletes the selected entity.
	*//******************************************************************/
	void DeleteSelectedEntity();

	void DrawSceneView();

	ecs::EntityHandle GetSelectedEntity();
	void SetSelectedEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Draws a border around the selected entity, if drawBoxes is true.
	*//******************************************************************/
	void DrawSelectedEntityBorder();

	void DrawGizmoInViewport(ImDrawList* drawList);

	void RenderGrid();

	bool m_drawPhysicsBoxes;
	bool m_drawVelocity;

private:
	/*****************************************************************//*!
	\brief
		Checks if the selected entity is valid, and if it isn't deselects it.
	\return
		True if the selected entity points to a valid entity. False otherwise.
	*//******************************************************************/
	bool CheckIsSelectedEntityValid();

	/*****************************************************************//*!
	\brief
		Draws the selected entity's name and layer.
	*//******************************************************************/
	void DrawEntityHeader();

	/*****************************************************************//*!
	\brief
		Draws the selected entity's components.
	*//******************************************************************/
	void DrawEntityComps();

	/*****************************************************************//*!
	\brief
		Draws the add component section.
	*//******************************************************************/
	void DrawAddComp();

	/*****************************************************************//*!
	\brief
		Draw buttons that interact with the selected entity.
	*//******************************************************************/
	void DrawEntityActionsButton();

	
private:
	Vector2 SnapToGrid(const Vector2& worldPos) const;

	float m_gridSize{ 128.0f };
	Vector2 m_gridOffset{ 0.0f,0.0f };
	bool m_showGrid{ false };
	bool m_snapToGrid{ false };
	Vector4 m_gridColor{ 1.0f,1.0f,1.0f,1.0f };
	ecs::EntityHandle selectedEntity{ nullptr };
	bool drawBoxes{ true };

	Gizmo m_gizmo;
	GizmoType m_currentGizmoType = GizmoType::None;

	EditorTweenModule editorTweenModule;
#endif
};
