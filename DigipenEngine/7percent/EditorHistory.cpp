/******************************************************************************/
/*!
\file   EditorHistory.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/13/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file defining history events specifically for use in the editor.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "EditorHistory.h"

HistoryEvent_Translation::HistoryEvent_Translation(ecs::EntityHandle entity, const Vector2& originalPos)
	: HistoryEventECSBase{ entity }
	, prevPos{ originalPos }
{
}

bool HistoryEvent_Translation::IsNonAction() const
{
	return prevPos == targetEntity->GetTransform().GetLocalPosition();
}

HistoryEventBase* HistoryEvent_Translation::DoUndoAction()
{
	Vector2 redoPos{ targetEntity->GetTransform().GetLocalPosition() };
	targetEntity->GetTransform().SetLocalPosition(prevPos);
	prevPos = redoPos;
	return this;
}

HistoryEvent_ZPos::HistoryEvent_ZPos(ecs::EntityHandle entity, float originalZ)
	: HistoryEventECSBase{ entity }
	, prevZ{ originalZ }
{
}

bool HistoryEvent_ZPos::IsNonAction() const
{
	return std::fabs(prevZ - targetEntity->GetTransform().GetZPos()) <= std::numeric_limits<float>::epsilon();
}

HistoryEventBase* HistoryEvent_ZPos::DoUndoAction()
{
	float redoZ{ targetEntity->GetTransform().GetZPos() };
	targetEntity->GetTransform().SetZPos(prevZ);
	prevZ = redoZ;
	return this;
}

HistoryEvent_Rotation::HistoryEvent_Rotation(ecs::EntityHandle entity, float originalRotation)
	: HistoryEventECSBase{ entity }
	, prevRotation{ originalRotation }
{
}

bool HistoryEvent_Rotation::IsNonAction() const
{
	return prevRotation == targetEntity->GetTransform().GetLocalRotation();
}

HistoryEventBase* HistoryEvent_Rotation::DoUndoAction()
{
	float redoRotation{ targetEntity->GetTransform().GetLocalRotation() };
	targetEntity->GetTransform().SetLocalRotation(prevRotation);
	prevRotation = redoRotation;
	return this;
}

HistoryEvent_Scale::HistoryEvent_Scale(ecs::EntityHandle entity, const Vector2& originalScale)
	: HistoryEventECSBase{ entity }
	, prevScale{ originalScale }
{
}

bool HistoryEvent_Scale::IsNonAction() const
{
	return prevScale == targetEntity->GetTransform().GetLocalScale();
}

HistoryEventBase* HistoryEvent_Scale::DoUndoAction()
{
	Vector2 redoScale{ targetEntity->GetTransform().GetLocalScale() };
	targetEntity->GetTransform().SetLocalScale(prevScale);
	prevScale = redoScale;
	return this;
}

HistoryEvent_EntityCreate::HistoryEvent_EntityCreate(ecs::EntityHandle createdEntity)
	: HistoryEventECSBase{ createdEntity }
{
}

HistoryEventBase* HistoryEvent_EntityCreate::DoUndoAction()
{
	HistoryEvent_EntityDelete* deleteEvent{ new HistoryEvent_EntityDelete{ targetEntity } };
	ecs::DeleteEntityNow(targetEntity);
	return deleteEvent;
}

HistoryEvent_EntityDelete::HistoryEvent_EntityDelete(ecs::EntityHandle entityToDelete)
	// Make a copy of the entity inside a separate ecs pool
	: savedEntity{ SaveEntity(entityToDelete) }
{
	if (Transform* parentTransform{ entityToDelete->GetTransform().GetParent() })
		originalParent = parentTransform->GetEntity();
	else
		originalParent = nullptr;
}

HistoryEvent_EntityDelete::HistoryEvent_EntityDelete(HistoryEvent_EntityDelete&& other) noexcept
	: savedEntity{ other.savedEntity }
	, savedToOriginalEntityMap{ std::move(other.savedToOriginalEntityMap) }
	, originalParent{ other.originalParent }
{
	other.savedEntity = other.originalParent = nullptr;
}

HistoryEvent_EntityDelete::~HistoryEvent_EntityDelete()
{
	if (savedEntity)
		DeleteSavedEntity();
}

void HistoryEvent_EntityDelete::OnEntityReplaced(ecs::EntityHandle oldHandle, ecs::EntityHandle newHandle)
{
	if (originalParent == oldHandle)
		originalParent = newHandle;
}

HistoryEventBase* HistoryEvent_EntityDelete::Undo()
{
	ecs::EntityHandle restoredEntity{ RestoreEntity(savedEntity) };

	// Reparent if able
	if (originalParent)
		if (ecs::IsEntityHandleValid(originalParent))
			restoredEntity->GetTransform().SetParent(originalParent->GetTransform());
		else
			CONSOLE_LOG(LEVEL_ERROR) << "Undo: The restored entity's original parent does not exist anymore! The restored entity will be unparented.";

	DeleteSavedEntity();

	return new HistoryEvent_EntityCreate{ restoredEntity };
}

ecs::EntityHandle HistoryEvent_EntityDelete::SaveEntity(ecs::EntityHandle entity)
{
	// Clone the entity (without children) and save the new/original handle pair
	ecs::EntityHandle clonedEntity{ ecs::CloneEntityToPoolNow(entity, ecs::POOL::UNDO, false) };
	savedToOriginalEntityMap.try_emplace(clonedEntity, entity);

	// Do the same for children, and parent them to the cloned entity
	for (Transform* childTransform : entity->GetTransform().GetChildren())
		SaveEntity(childTransform->GetEntity())->GetTransform().SetParent(clonedEntity->GetTransform());

	return clonedEntity;
}

ecs::EntityHandle HistoryEvent_EntityDelete::RestoreEntity(ecs::EntityHandle entity)
{
	// Clone the entity (without children) back into the default pool
	ecs::POOL prevID{ ecs::GetCurrentPoolId() };
	ecs::SwitchToPool(ecs::POOL::UNDO);
	ecs::EntityHandle restoredEntity{ ecs::CloneEntityToPoolNow(entity, ecs::POOL::DEFAULT, false) };
	ecs::SwitchToPool(prevID);

	// Do the same for children and parent them to the cloned entity
	for (Transform* childTransform : entity->GetTransform().GetChildren())
		RestoreEntity(childTransform->GetEntity())->GetTransform().SetParent(restoredEntity->GetTransform());

	// Update all history events that were pointing to the original entity to point to the new entity
	ST<History>::Get()->ForEachLoggedEvent([originalEntity = savedToOriginalEntityMap.at(entity), restoredEntity](HistoryEventBase* event) -> void {
		if (IHistoryEvent_HoldsEntity* holdsEntityInterface{ dynamic_cast<IHistoryEvent_HoldsEntity*>(event) })
			holdsEntityInterface->OnEntityReplaced(originalEntity, restoredEntity);
	});

	return restoredEntity;
}

void HistoryEvent_EntityDelete::DeleteSavedEntity()
{
	ecs::POOL prevID{ ecs::GetCurrentPoolId() };
	ecs::SwitchToPool(ecs::POOL::UNDO);
	ecs::DeleteEntity(savedEntity);
	ecs::FlushChanges();
	ecs::SwitchToPool(prevID);
	savedEntity = nullptr;
}
