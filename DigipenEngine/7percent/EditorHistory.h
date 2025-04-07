/******************************************************************************/
/*!
\file   EditorHistory.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/13/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file defining history events specifically for use in the editor.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "History.h"
#include "Console.h"

/*****************************************************************//*!
\class IHistoryEvent_HoldsEntity
\brief
	An interface for history events that reference entities. This ensures
	history events that store entity handles update their handles if an
	entity's handle has changed (perhaps due to respawning after deletion, etc).
	NOTE: This interface MUST be inherited publically.
*//******************************************************************/
class IHistoryEvent_HoldsEntity
{
public:
	/*****************************************************************//*!
	\brief
		Informs the history event to replace an entity handle if the signature matches.
	\param oldHandle
		The old entity handle to be replaced.
	\param newHandle
		The new entity handle.
	*//******************************************************************/
	virtual void OnEntityReplaced(ecs::EntityHandle oldHandle, ecs::EntityHandle newHandle) = 0;
};

/*****************************************************************//*!
\class HistoryEventECSBase
\brief
	A base history event class that does all the busy work to store an entity.
*//******************************************************************/
template <typename T>
class HistoryEventECSBase : public HistoryEventBaseT<T>, public IHistoryEvent_HoldsEntity
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity stored by this history event.
	*//******************************************************************/
	HistoryEventECSBase(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Checks if another history event is the same kind of action on the same object
		as this one.
	\param other
		The history event to check.
	\return
		True if the history event is the same kind of action. False otherwise.
	*//******************************************************************/
	virtual bool IsSameObject(const HistoryEventBase& other) const final;

	/*****************************************************************//*!
	\brief
		Undoes this event.
	\return HistoryEventBase*
		A pointer to the history event that will be stored in the redo queue.
		If the user redos, this history event will be undone and the returned
		history event will be stored in the undo queue. If nullptr, this
		history event itself will be stored in the redo queue.
	*//******************************************************************/
	virtual HistoryEventBase* Undo() final;

	/*****************************************************************//*!
	\brief
		Replaces the stored entity handle if the signature matches.
	\param oldHandle
		The old entity handle to be replaced.
	\param newHandle
		The new entity handle.
	*//******************************************************************/
	virtual void OnEntityReplaced(ecs::EntityHandle oldHandle, ecs::EntityHandle newHandle) override;

protected:
	/*****************************************************************//*!
	\brief
		The function that derived classes override to do the undo action.
	\return HistoryEventBase*
		A pointer to the history event that will be stored in the redo queue.
		If the user redos, this history event will be undone and the returned
		history event will be stored in the undo queue. If nullptr, this
		history event itself will be stored in the redo queue.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() = 0;

protected:
	//! The stored entity handle.
	ecs::EntityHandle targetEntity;

};

/*****************************************************************//*!
\class HistoryEvent_Translation
\brief
	The history event storing a translation event.
*//******************************************************************/
class HistoryEvent_Translation : public HistoryEventECSBase<HistoryEvent_Translation>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity that was translated.
	\param originalPos
		The original position of the entity.
	*//******************************************************************/
	HistoryEvent_Translation(ecs::EntityHandle entity, const Vector2& originalPos);

	/*****************************************************************//*!
	\brief
		Checks if this history event does nothing if undone.
	\return
		True if the history event does nothing. False otherwise.
	*//******************************************************************/
	virtual bool IsNonAction() const override;
	
protected:
	/*****************************************************************//*!
	\brief
		Undoes the translation.
	\return HistoryEventBase*
		This.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;

protected:
	//! The original position of the entity.
	Vector2 prevPos;
};

/*****************************************************************//*!
\class HistoryEvent_ZPos
\brief
	The history event storing a z position change event.
*//******************************************************************/
class HistoryEvent_ZPos : public HistoryEventECSBase<HistoryEvent_ZPos>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity that had its z position changed.
	\param originalZ
		The original z position of the entity.
	*//******************************************************************/
	HistoryEvent_ZPos(ecs::EntityHandle entity, float originalZ);

	/*****************************************************************//*!
	\brief
		Checks if this history event does nothing if undone.
	\return
		True if the history event does nothing. False otherwise.
	*//******************************************************************/
	virtual bool IsNonAction() const override;

protected:
	/*****************************************************************//*!
	\brief
		Undoes the z position change.
	\return HistoryEventBase*
		This.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;

protected:
	//! The previous z position of the entity.
	float prevZ;
};

/*****************************************************************//*!
\class HistoryEvent_Rotation
\brief
	Undoes the rotation.
*//******************************************************************/
class HistoryEvent_Rotation : public HistoryEventECSBase<HistoryEvent_Rotation>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity that had its rotation changed.
	\param originalRotation
		The original rotation of the entity.
	*//******************************************************************/
	HistoryEvent_Rotation(ecs::EntityHandle entity, float originalRotation);

	/*****************************************************************//*!
	\brief
		Checks if this history event does nothing if undone.
	\return
		True if the history event does nothing. False otherwise.
	*//******************************************************************/
	virtual bool IsNonAction() const override;

protected:
	/*****************************************************************//*!
	\brief
		Undoes the rotation.
	\return HistoryEventBase*
		This.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;

protected:
	//! The previous rotation.
	float prevRotation;

};

/*****************************************************************//*!
\class HistoryEvent_Scale
\brief
	Undoes the scale.
*//******************************************************************/
class HistoryEvent_Scale : public HistoryEventECSBase<HistoryEvent_Scale>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity that had its scale changed.
	\param originalScale
		The original rotation of the entity.
	*//******************************************************************/
	HistoryEvent_Scale(ecs::EntityHandle entity, const Vector2& originalScale);

	/*****************************************************************//*!
	\brief
		Checks if this history event does nothing if undone.
	\return
		True if the history event does nothing. False otherwise.
	*//******************************************************************/
	virtual bool IsNonAction() const override;

protected:
	/*****************************************************************//*!
	\brief
		Undoes the scaling.
	\return HistoryEventBase*
		This.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;

protected:
	//! The previous scale.
	Vector2 prevScale;
};

/*****************************************************************//*!
\class HistoryEvent_EntityCreate
\brief
	Undoes the creation of an entity.
*//******************************************************************/
class HistoryEvent_EntityCreate : public HistoryEventECSBase<HistoryEvent_EntityCreate>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity that was created.
	*//******************************************************************/
	HistoryEvent_EntityCreate(ecs::EntityHandle createdEntity);

protected:
	/*****************************************************************//*!
	\brief
		Undoes the entity creation.
	\return HistoryEventBase*
		A new HistoryEvent_EntityDelete targeting the deleted entity.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;

};

/*****************************************************************//*!
\class HistoryEvent_EntityDelete
\brief
	Undoes the deletion of an entity.
*//******************************************************************/
class HistoryEvent_EntityDelete : public HistoryEventBaseT<HistoryEvent_EntityDelete>, public IHistoryEvent_HoldsEntity
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity that was deleted.
	*//******************************************************************/
	HistoryEvent_EntityDelete(ecs::EntityHandle entityToDelete);
	
	// Disallow copying of this history event. (because we're handling the entity in the UNDO ecs pool, and we shouldn't need to copy that entity as well)
	HistoryEvent_EntityDelete(const HistoryEvent_EntityDelete&) = delete;

	/*****************************************************************//*!
	\brief
		Move constructor.
	\param other
		The history event to move.
	*//******************************************************************/
	HistoryEvent_EntityDelete(HistoryEvent_EntityDelete&& other) noexcept;

	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	virtual ~HistoryEvent_EntityDelete();

	/*****************************************************************//*!
	\brief
		Replaces the stored entity handle if the signature matches.
	\param oldHandle
		The old entity handle to be replaced.
	\param newHandle
		The new entity handle.
	*//******************************************************************/
	virtual void OnEntityReplaced(ecs::EntityHandle oldHandle, ecs::EntityHandle newHandle) override;

protected:
	/*****************************************************************//*!
	\brief
		Undoes the entity deletion.
	\return HistoryEventBase*
		A new HistoryEvent_EntityCreate targeting the restored entity.
	*//******************************************************************/
	virtual HistoryEventBase* Undo() override;

private:
	/*****************************************************************//*!
	\brief
		Saves the entity from the current pool to the UNDO pool.
	*//******************************************************************/
	ecs::EntityHandle SaveEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Moves the saved entity from the UNDO pool to the current pool.
	*//******************************************************************/
	ecs::EntityHandle RestoreEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Deletes the saved entity inside the UNDO pool.
	*//******************************************************************/
	void DeleteSavedEntity();

protected:
	//! Maps the saved entities' handles with their original handles so we can tell other events to update their entity handles.
	//! (needs to initialize before savedEntity - see constructor)
	std::map<ecs::EntityHandle, ecs::EntityHandle> savedToOriginalEntityMap;
	//! The saved entity.
	ecs::EntityHandle savedEntity;

	//! Handle to the parent of the original entity, for re-parenting upon undo
	ecs::EntityHandle originalParent;
};

/*****************************************************************//*!
\class HistoryEvent_CompAdd
\brief
	Undoes the addition of a component.
*//******************************************************************/
template <typename CompType>
class HistoryEvent_CompAdd : public HistoryEventECSBase<HistoryEvent_CompAdd<CompType>>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity to which the component was added to.
	\param compHash
		The hash of the component.
	*//******************************************************************/
	HistoryEvent_CompAdd(ecs::EntityHandle entity);

protected:
	/*****************************************************************//*!
	\brief
		Undoes the component addition.
	\return HistoryEventBase*
		A new HistoryEvent_CompRemove targeting the deleted component.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;
};

/*****************************************************************//*!
\class HistoryEvent_CompRemove
\brief
	Undoes the deletion of a component.
\tparam CompType
	The type of the component.
*//******************************************************************/
template <typename CompType>
class HistoryEvent_CompRemove : public HistoryEventECSBase<HistoryEvent_CompRemove<CompType>>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity from which the component was removed from.
	\param comp
		The component.
	*//******************************************************************/
	HistoryEvent_CompRemove(ecs::EntityHandle entity, const CompType& comp);

protected:
	/*****************************************************************//*!
	\brief
		Undoes the component deletion.
	\return HistoryEventBase*
		A new HistoryEvent_CompAdd targeting the restored component.
	*//******************************************************************/
	virtual HistoryEventBase* DoUndoAction() override;

private:
	//! The saved component, that will be restored when undone.
	CompType backupComp;
};

template <typename T>
HistoryEventECSBase<T>::HistoryEventECSBase(ecs::EntityHandle entity)
	: targetEntity{ entity }
{
}

template <typename T>
bool HistoryEventECSBase<T>::IsSameObject(const HistoryEventBase& other) const
{
	if (auto otherCasted{ dynamic_cast<const T*>(&other) })
		return otherCasted->targetEntity == targetEntity && HistoryEventBaseT<T>::IsSameObjectSpecial(*otherCasted);
	else
		return false;
}

template<typename T>
HistoryEventBase* HistoryEventECSBase<T>::Undo()
{
	if (!ecs::IsEntityHandleValid(targetEntity))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Cannot undo: Target entity is invalid!";
		return this;
	}

	return DoUndoAction();
}

template<typename T>
void HistoryEventECSBase<T>::OnEntityReplaced(ecs::EntityHandle oldHandle, ecs::EntityHandle newHandle)
{
	if (targetEntity == oldHandle)
		targetEntity = newHandle;
}

template<typename CompType>
HistoryEvent_CompAdd<CompType>::HistoryEvent_CompAdd(ecs::EntityHandle entity)
	: HistoryEventECSBase<HistoryEvent_CompAdd<CompType>>{ entity }
{
}

template<typename CompType>
HistoryEventBase* HistoryEvent_CompAdd<CompType>::DoUndoAction()
{
	auto targetEntity{ HistoryEventECSBase<HistoryEvent_CompAdd<CompType>>::targetEntity }; // For some reason the compiler cannot see targetEntity without this??
	HistoryEvent_CompRemove<CompType>* redoEvent{ new HistoryEvent_CompRemove<CompType>{ targetEntity, *targetEntity->GetComp<CompType>() } };
	targetEntity->RemoveCompNow<CompType>();
	return redoEvent;
}

template<typename CompType>
HistoryEvent_CompRemove<CompType>::HistoryEvent_CompRemove(ecs::EntityHandle entity, const CompType& comp)
	: HistoryEventECSBase<HistoryEvent_CompRemove<CompType>>{ entity }
	, backupComp{ comp }
{
}

template<typename CompType>
HistoryEventBase* HistoryEvent_CompRemove<CompType>::DoUndoAction()
{
	auto targetEntity{ HistoryEventECSBase<HistoryEvent_CompRemove<CompType>>::targetEntity }; // For some reason the compiler cannot see targetEntity without this??
	targetEntity->AddCompNow(std::move(backupComp));
	return new HistoryEvent_CompAdd<CompType>{ targetEntity };
}
