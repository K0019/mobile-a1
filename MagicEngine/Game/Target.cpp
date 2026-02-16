#include "Target.h"
#include "Editor/Containers/GUICollection.h"

void PositionRandomizerComponent::OnStart()
{
	ecs::GetEntityTransform(this).SetWorldPosition(Vec3{
		util::RandomRangeFloat(-randomPosRange.x, randomPosRange.x),
		util::RandomRangeFloat(-randomPosRange.y, randomPosRange.y),
		ecs::GetEntityTransform(this).GetWorldPosition().z
	});
}

void PositionRandomizerComponent::EditorDraw()
{
	gui::VarDrag("Random Position Range", &randomPosRange, 0.5f);
}
