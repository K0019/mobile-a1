#pragma once
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "Assets/AssetHandle.h"
#include "Assets/Types/AssetTypes.h"

/*****************************************************************//*!
\class MaterialSwapperComponent
\brief
	Spawns a prefab on the first frame of the simulation.
*//******************************************************************/
class MaterialSwapperComponent
	: public IRegisteredComponent<MaterialSwapperComponent>
	, public IEditorComponent<MaterialSwapperComponent>
	, public IGameComponentCallbacks<MaterialSwapperComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	MaterialSwapperComponent();

	void ToggleMaterialSwap(bool swapped);

	// Serialized
	AssetHandle<ResourceMaterial> swapMaterial;

	// Not serialized
	std::vector<AssetHandle<ResourceMaterial>> defaultMaterials;

	bool inited;

private:
	/*****************************************************************//*!
	\brief
		Draws this component to inspector.
	*//******************************************************************/
	virtual void EditorDraw() override;

public:
	property_vtable()

};
property_begin(MaterialSwapperComponent)
{
	property_var(swapMaterial)
}
property_vend_h(MaterialSwapperComponent)

