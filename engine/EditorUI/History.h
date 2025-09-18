/******************************************************************************/
/*!
\file   History.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/13/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a stack that tracks events as a history.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "GameSettings.h"

/*****************************************************************//*!
\class HistoryEventBase
\brief
	The base class for history events.
*//******************************************************************/
class HistoryEventBase
{
public:
	/*****************************************************************//*!
	\brief
		Destructor. Marked virtual for correct destruction.
	*//******************************************************************/
	virtual ~HistoryEventBase() = default;

	/*****************************************************************//*!
	\brief
		Checks if another history event is the same kind of action on the same object
		as this one.
	\param other
		The history event to check.
	\return
		True if the history event is the same kind of action. False otherwise.
	*//******************************************************************/
	virtual bool IsSameObject(const HistoryEventBase& other) const = 0;

	/*****************************************************************//*!
	\brief
		Checks if this history event does nothing if undone.
	\return
		True if the history event does nothing. False otherwise.
	*//******************************************************************/
	virtual bool IsNonAction() const;

	/*****************************************************************//*!
	\brief
		Undoes this event.
	\return HistoryEventBase*
		A pointer to the history event that will be stored in the redo queue.
		If the user redos, this history event will be undone and the returned
		history event will be stored in the undo queue. If nullptr, this
		history event itself will be stored in the redo queue.
	*//******************************************************************/
	virtual HistoryEventBase* Undo() = 0;

protected:
	/*****************************************************************//*!
	\brief
		Casts a history event to a specified type.
	\param other
		The history event to cast.
	\return
		The casted history event.
	*//******************************************************************/
	template <typename T>
	static const T* Cast(const HistoryEventBase& other);

};

/*****************************************************************//*!
\class HistoryEventBaseT
\tparam T
	The type of the history event.
\brief
	The base template that specialized history events derive from.
*//******************************************************************/
template <typename T>
class HistoryEventBaseT : public HistoryEventBase
{
public:
	/*****************************************************************//*!
	\brief
		Checks if another history event is the same kind of action on the same object
		as this one.
	\param other
		The history event to check.
	\return
		True if the history event is the same kind of action. False otherwise.
	*//******************************************************************/
	virtual bool IsSameObject(const HistoryEventBase& other) const override;

protected:
	/*****************************************************************//*!
	\brief
		A function available for child classes to override to specify whether
		this history event is the same as another history event.
	\param other
		The other history event.
	*//******************************************************************/
	virtual bool IsSameObjectSpecial(const T& other) const;

};

/*****************************************************************//*!
\class History
\brief
	Tracks all history events in a queue.
*//******************************************************************/
class History
{
public:
	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~History();

	/*****************************************************************//*!
	\brief
		Logs a one time event to history.
	\tparam T
		The type of the event.
	\param event
		The event.
	*//******************************************************************/
	template <typename T>
	void OneEvent(T&& event);

	/*****************************************************************//*!
	\brief
		Logs an ongoing event to history.
	\tparam T
		The type of the event.
	\param event
		The event.
	*//******************************************************************/
	template <typename T>
	void IntermediateEvent(T&& event);

	/*****************************************************************//*!
	\brief
		Undoes one event.
	*//******************************************************************/
	void UndoOne();

	/*****************************************************************//*!
	\brief
		Redoes one event.
	*//******************************************************************/
	void RedoOne();

	/*****************************************************************//*!
	\brief
		Clears the history queue.
	*//******************************************************************/
	void Clear();

	/*****************************************************************//*!
	\brief
		Executes a function for each event in the queue.
	\param func
		The function to execute.
	*//******************************************************************/
	void ForEachLoggedEvent(const std::function<void(HistoryEventBase*)>& func);

private:
	friend ST<History>;

	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	History() = default;

	/*****************************************************************//*!
	\brief
		Adds an event to the history queue. If the queue gets too large, removes
		the oldest event.
	\tparam T
		The event's class type.
	\param event
		The event.
	*//******************************************************************/
	template <typename T>
	void PushUndoEvent(T&& event);

	/*****************************************************************//*!
	\brief
		Undoes one history event from a queue, and pushes the history event
		that will undo this undo into another queue.
	\param fromQueue
		The queue from which a history event will be undone.
	\param toQueue
		The queue to which the history event that will undo this undoing will
		be pushed into.
	*//******************************************************************/
	void PerformOneUndo(std::deque<HistoryEventBase*>& fromQueue, std::deque<HistoryEventBase*>& toQueue);

	/*****************************************************************//*!
	\brief
		Clears the redo history queue.
	*//******************************************************************/
	void ClearRedoHistory();

private:
	//! The queue storing the events in history.
	std::deque<HistoryEventBase*> undoQueue;
	std::deque<HistoryEventBase*> redoQueue;

};

template<typename T>
void History::OneEvent(T&& event)
{
	ClearRedoHistory();
	PushUndoEvent(std::forward<T>(event));
}

template<typename T>
void History::IntermediateEvent(T&& event)
{
	// If the previous event is the same kind as this event and the same target, don't do anything (because we want to keep the oldest values to undo towards)
	if (!undoQueue.empty() && undoQueue.back()->IsSameObject(event))
		return;
	// Otherwise, create a new event
	else
		OneEvent(std::forward<T>(event));
}

template<typename T>
void History::PushUndoEvent(T&& event)
{
	// If the last event is a non-action, we can replace it.
	if (!undoQueue.empty() && undoQueue.back()->IsNonAction())
	{
		delete undoQueue.back();
		undoQueue.pop_back();
	}
	// If the undo queue is full, delete the oldest event.
	else if (undoQueue.size() >= ST<GameSettings>::Get()->m_editorHistoryMax)
	{
		delete undoQueue.front();
		undoQueue.pop_front();
	}

	undoQueue.push_back(new std::decay_t<T>{ std::forward<T>(event) });
}

template<typename T>
const T* HistoryEventBase::Cast(const HistoryEventBase& other)
{
	return dynamic_cast<T>(&other);
}

template<typename T>
bool HistoryEventBaseT<T>::IsSameObject(const HistoryEventBase& other) const
{
	if (auto otherCasted{ dynamic_cast<const T*>(&other) })
		return IsSameObjectSpecial(*otherCasted);
	else
		return false;
}

template<typename T>
bool HistoryEventBaseT<T>::IsSameObjectSpecial(const T&) const
{
	return true;
}
