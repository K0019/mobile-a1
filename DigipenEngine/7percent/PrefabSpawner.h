#pragma once
#include "IGameComponentCallbacks.h"
/*****************************************************************//*!
\class PrefabSpawnComponent
\brief
	Spawns a prefab on the first frame of the simulation.
*//******************************************************************/
class PrefabSpawnComponent
	: public IRegisteredComponent<PrefabSpawnComponent>
#ifdef IMGUI_ENABLED
	, public IEditorComponent<PrefabSpawnComponent>
#endif
	, public IGameComponentCallbacks<PrefabSpawnComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	PrefabSpawnComponent();

	// Serialised strings do not load in at this stage, when loading in the
	// scene using ST<SceneManager>!
	void OnStart() override;

	/*****************************************************************//*!
	\brief
		Spawns a prefab and deletes itself.
	*//******************************************************************/
	void Init();

	bool inited;

private:
	/*****************************************************************//*!
	\brief
		Draws this component to inspector.
	*//******************************************************************/
	static void EditorDraw(PrefabSpawnComponent& comp);

private:
	//! The prefab's name
	std::string PrefabName;

public:
	property_vtable()

};
property_begin(PrefabSpawnComponent)
{
	property_var(PrefabName)
}
property_vend_h(PrefabSpawnComponent)

class PrefabSpawnSystem : public ecs::System<PrefabSpawnSystem, PrefabSpawnComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	PrefabSpawnSystem();

private:
	/*****************************************************************//*!
	\brief
		Required to initialise prefabs.
	\param comp
		Component to update.
	*//******************************************************************/
	void UpdatePrefabSpawn(PrefabSpawnComponent& comp);
};