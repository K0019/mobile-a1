#pragma once
#include "ECS/ECS.h"
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"

class PositionRandomizerComponent
	: public IRegisteredComponent<PositionRandomizerComponent>
	, public IEditorComponent<PositionRandomizerComponent>
	, public IGameComponentCallbacks<PositionRandomizerComponent>
{
public:
	void OnStart() override;
	void EditorDraw() override;

private:
	Vec2 randomPosRange;

public:
	property_vtable()
};
property_begin(PositionRandomizerComponent)
{
	property_var(randomPosRange)
}
property_vend_h(PositionRandomizerComponent)