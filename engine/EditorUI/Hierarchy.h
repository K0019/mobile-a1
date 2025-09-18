/******************************************************************************/
/*!
\file   Hierarchy.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   02/11/2024

\author Chan Kuan Fu Ryan (75%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Kendrick Sim Hean Guan (25%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  Handles the rendering and logic of Hierarchy window through ImGui.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

/*****************************************************************//*!
\class Hierarchy
\brief
	Primary class for Hierarchy view.
*//******************************************************************/
#ifdef IMGUI_ENABLED
class Hierarchy
{
public:
	friend ST<Hierarchy>;

	/*****************************************************************//*!
	\brief
		Constructor which initialises variables.
	*//******************************************************************/
	Hierarchy();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	~Hierarchy() = default;

	/*****************************************************************//*!
	\brief
		Primary function for drawing the Hierarchy window.
	\param p_open
		Boolean pointer provided to allow this window to be closed.
	*//******************************************************************/
	void Draw();

	bool isOpen{ false }; // Whether the Hierarchy window is open

private:
	/*****************************************************************//*!
	\brief
		A recursive function which iterates an entities' children and
		calls itself for each child. Renders the entity in Hierarchy
		window. Returns true if this entity or any of children match the search filter
	\param entity
		Target entity.
	\param targetSceneIndex
		Current scene index.
	\param textFilter
		Filter for searching entities.
	*//******************************************************************/
	bool ShowEntity(ecs::EntityHandle entity, int targetSceneIndex, const gui::TextBoxWithFilter& textFilter);

	void HandleSceneDropTarget(int targetSceneIndex);

	/*****************************************************************//*!
	\brief
		Handles drag and drop functionality for parenting entities to
		other entities.
	\param entity
		Target entity.
	\param name
		Name of the entity.
	\param targetSceneIndex
		Current scene index.
	*//******************************************************************/
	void HandleDragAndDrop(ecs::EntityHandle entity, std::string const& name, int targetSceneIndex);

	bool IsEntityAncestorOf(ecs::EntityHandle potentialAncestor, ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Switches the entities' scene index.
	\param entity
		Target entity.
	\param sceneIndex
		Index of scene to switch to.
	*//******************************************************************/
	void SwitchEntityScene(ecs::EntityHandle entity, int sceneIndex);

	/*****************************************************************//*!
	\brief
		Handles renaming of entities.
	*//******************************************************************/
	void HandleEntityRenaming();

	/*****************************************************************//*!
	\brief
		Handles renaming of scenes.
	*//******************************************************************/
	void HandleSceneRenaming();

	/*****************************************************************//*!
	\brief
		Recursively searches for a Transform oointer in a set of Transform
		pointers.
	\param children
		The children set to search in.
	\param toFind
		Transform pointer to find.
	*//******************************************************************/
	bool FindElementInChildren(std::set<Transform*> children, Transform* toFind);

	/*****************************************************************//*!
	\brief
		Checks if entity is clicked or double-clicked. If single-clicked,
		it sets the selected entity in the Editor to the one selected in
		Hierarchy window. If double-clicked, it sends the
		"OnEntitySelected" message.
	\param entity
		Target entity.
	*//******************************************************************/
	void CheckSelection(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Checks if the background has been right-clicked. If so, opens a
		context menu to create a new scene.
	*//******************************************************************/
	void CheckBackgroundWindowContextMenu();

	/*****************************************************************//*!
	\brief
		Checks if a scene has been right-clicked. If so, opens a context 
		menu to rename or unload the scene.
	\param sceneIndex
		Current scene index.
	*//******************************************************************/
	bool CheckSceneContextMenu(int sceneIndex);

	/*****************************************************************//*!
	\brief
		Checks if an entity has been right-clicked. If so, opens a context
		menu to rename the entity.
	\param entity
		Target entity.
	*//******************************************************************/
	void CheckEntityContextMenu(ecs::EntityHandle entity);

		/*****************************************************************//*!
	\brief
		Checks if a entity or child entity matches the search filter.
	\param entity
		Target entity.
	\param textFilter
		Filter for searching entities.
	*//******************************************************************/
	bool DoesEntityOrChildrenMatchSearch(ecs::EntityHandle entity, const gui::TextBoxWithFilter& textFilter);

	// Amount of indent for leaf nodes
	static constexpr float LEAF_INDENT = 55.0f;

	// Threshold for double click in seconds
	static constexpr float DBL_CLICK_THRESHOLD = 0.3f;

	// Buffer size for renaming entities
	static constexpr int BUFFER_SIZE = 64;

	// Keep track of tree nodes that have already been expanded
	std::set<ecs::EntityHandle> expandedNodes;
	
	std::chrono::time_point<std::chrono::steady_clock> lastClickTime;
	ecs::EntityHandle lastClickedEntity;

	// Renaming
	ecs::EntityHandle entityToRename;	// Entity being renamed
	bool isRenamingEntity;				// If currently renaming an entity
	int sceneIndexToRename;				// Scene being renamed
	bool isRenamingScene;				// If currently renaming a scene
	std::string buffer;					// Buffer to store the new name
    // Available space tracking to ensure all scenes are visible
	float availableWindowHeight = 0.0f;
	float usedHeight = 0.0f;
	std::set<int> expandedSceneIndices;
};

#endif