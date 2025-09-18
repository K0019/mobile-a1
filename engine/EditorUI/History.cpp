/******************************************************************************/
/*!
\file   History.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/13/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a stack that tracks events as a history.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "History.h"

bool HistoryEventBase::IsNonAction() const
{
	return false;
}

History::~History()
{
	Clear();
}

void History::UndoOne()
{
	PerformOneUndo(undoQueue, redoQueue);
}

void History::RedoOne()
{
	PerformOneUndo(redoQueue, undoQueue);
}

void History::Clear()
{
	ClearRedoHistory();
	for (; !undoQueue.empty(); undoQueue.pop_front())
		delete undoQueue.front();
}

void History::ForEachLoggedEvent(const std::function<void(HistoryEventBase*)>& func)
{
	for (auto event : redoQueue)
		func(event);
	for (auto event : undoQueue)
		func(event);
}

void History::PerformOneUndo(std::deque<HistoryEventBase*>& fromQueue, std::deque<HistoryEventBase*>& toQueue)
{
	if (fromQueue.empty())
		return;
	
	HistoryEventBase* redoEvent{ fromQueue.back()->Undo() };
	if (redoEvent != fromQueue.back())
		delete fromQueue.back();
	toQueue.push_back(redoEvent);
	fromQueue.pop_back();
}

void History::ClearRedoHistory()
{
	for (; !redoQueue.empty(); redoQueue.pop_front())
		delete redoQueue.front();
}
