/******************************************************************************/
/*!
\file   Scheduler.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   02/10/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a scheduler that can schedule functions to be
  called at a later point in time.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Scheduler.h"

ScheduledTask* ScheduledTask::Update(float dt)
{
	remainingDelay -= dt;
	if (remainingDelay > 0.0f)
		return this;

	func();

	if (nextTask)
		return nextTask->Update(-remainingDelay);
	else
		return nullptr;
}

ScheduledTask* ScheduledTask::GetNextTask()
{
	return nextTask;
}

ScheduledTaskUserWrapper::ScheduledTaskUserWrapper(ScheduledTask& task)
	: task{ task }
	, nextIsScheduled{ false }
{
}

Scheduler::~Scheduler()
{
	for (ScheduledTask& task : tasks)
		DeleteTasks(task.GetNextTask(), nullptr);
	for (ScheduledTask& task : buffer)
		DeleteTasks(task.GetNextTask(), nullptr);
}

void Scheduler::Update(float dt)
{
	std::move(buffer.begin(), buffer.end(), std::back_inserter(tasks));
	buffer.clear();

	for (auto taskIter{ tasks.begin() }; taskIter != tasks.end();)
	{
		ScheduledTask* upcomingTaskPtr{ taskIter->Update(dt) };
		// If the task has not run yet due to delay
		if (upcomingTaskPtr == taskIter._Ptr)
		{
			++taskIter;
			continue;
		}
		
		// If there is a new queued task, need to buffer it
		if (upcomingTaskPtr)
		{
			buffer.emplace_back(std::move(*upcomingTaskPtr));
			delete upcomingTaskPtr;
			// Note that while nextTaskPtr's object is now invalid, all tasks before nextTaskPtr are still valid, and the last task before nextTaskPtr still has the same address to nextTaskPtr.
			// This allows DeleteTasks to run safely.
		}

		DeleteTasks(taskIter->GetNextTask(), upcomingTaskPtr);
		taskIter = tasks.erase(taskIter);
	}
}

void Scheduler::DeleteTasks(ScheduledTask* begin, ScheduledTask* end)
{
	while (begin != end)
	{
		ScheduledTask* nextTask{ begin->GetNextTask() };
		delete begin;
		begin = nextTask;
	}
}
