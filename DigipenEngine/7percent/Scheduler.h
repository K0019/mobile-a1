/******************************************************************************/
/*!
\file   Scheduler.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   02/10/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a scheduler that can schedule functions to be
  called at a later point in time.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

#pragma region Interface

/*****************************************************************//*!
\class ScheduledTask
\brief
	A scheduled task that executes a function when it is time to do so.
*//******************************************************************/
class ScheduledTask
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\tparam T
		A function type. Must require no parameters.
	\param delay
		The delay until calling the function.
	\param func
		The function to call.
	*//******************************************************************/
	template <typename T>
		requires std::regular_invocable<T>
	ScheduledTask(float delay, T func);

	/*****************************************************************//*!
	\brief
		Updates the scheduling of this scheduled task, and executes the function
		if it is time to do so.
	\param dt
		Delta time.
	\return
		A pointer to a scheduled task.
		If this, the task has not executed due to the delay.
		If another task, this task has executed and there is a new task queued after this one.
		If nullptr, this task has executed and there are no more tasks after this.
	*//******************************************************************/
	ScheduledTask* Update(float dt);

	/*****************************************************************//*!
	\brief
		Attach a scheduled task that will be executed after this scheduled task.
	\tparam T
		A function type. Must require no parameters.
	\param delay
		The delay until calling the function.
	\param func
		The function to call.
	\return
		The chained scheduled task.
	*//******************************************************************/
	template <typename T>
		requires std::regular_invocable<T>
	ScheduledTask& Chain(float delay, T func);

	/*****************************************************************//*!
	\brief
		Gets the task scheduled after this task.
	\return
		The task scheduled after this task.
	*//******************************************************************/
	ScheduledTask* GetNextTask();

private:
	//! The time delay until executing the function.
	float remainingDelay;
	//! The function to execute.
	std::function<void()> func;
	//! The next scheduled task queued up after this scheduled task.
	ScheduledTask* nextTask;

};

/*****************************************************************//*!
\class ScheduledTaskUserWrapper
\brief
	A wrapper around ScheduledTask to only provide user with necessary interactions.
*//******************************************************************/
class ScheduledTaskUserWrapper
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param task
		The task to wrap.
	*//******************************************************************/
	ScheduledTaskUserWrapper(ScheduledTask& coroutine);

	/*****************************************************************//*!
	\brief
		Schedule another task after the execution of this one.
	\tparam T
		A function type. Must require no parameters.
	\param func
		The function to call.
	\return
		The next scheduled task.
	*//******************************************************************/
	template <typename T>
		requires std::regular_invocable<T>
	ScheduledTaskUserWrapper Then(T func);

	/*****************************************************************//*!
	\brief
		Schedule another task after the execution of this one.
	\tparam T
		A function type. Must require no parameters.
	\param delay
		The delay until executing the next task.
	\param func
		The function to call.
	\return
		The next scheduled task.
	*//******************************************************************/
	template <typename T>
		requires std::regular_invocable<T>
	ScheduledTaskUserWrapper Then(float delay, T func);

private:
	//! The task we're wrapping.
	ScheduledTask& task;
	//! Flag to ensure we don't schedule more than 1 task after us.
	bool nextIsScheduled;
};

/*****************************************************************//*!
\class Scheduler
\brief
	Container of all scheduled tasks.
*//******************************************************************/
class Scheduler
{
public:
	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~Scheduler();

	/*****************************************************************//*!
	\brief
		Schedules a task.
	\tparam T
		A function type. Must require no parameters.
	\param func
		The function to call.
	*//******************************************************************/
	template <typename T>
		requires std::regular_invocable<T>
	ScheduledTaskUserWrapper Add(T func);

	/*****************************************************************//*!
	\brief
		Schedules a task.
	\tparam T
		A function type. Must require no parameters.
	\param delay
		The delay until calling the function.
	\param func
		The function to call.
	*//******************************************************************/
	template <typename T>
		requires std::regular_invocable<T>
	ScheduledTaskUserWrapper Add(float delay, T func);

	/*****************************************************************//*!
	\brief
		Updates all tasks.
	\param dt
		Delta time.
	*//******************************************************************/
	void Update(float dt);

private:
	/*****************************************************************//*!
	\brief
		Deletes all chained tasks until the specified task.
	\param begin
		The first task to delete.
	\param end
		1 past the last task to delete.
	*//******************************************************************/
	void DeleteTasks(ScheduledTask* begin, ScheduledTask* end);

private:
	//! Tasks scheduled for future.
	std::vector<ScheduledTask> tasks;
	//! Tasks added before the next update, so we don't crash if tasks schedule more tasks.
	std::vector<ScheduledTask> buffer;
	
};

#pragma endregion // Interface

#pragma region Definition

template<typename T>
	requires std::regular_invocable<T>
ScheduledTask::ScheduledTask(float delay, T func)
	: remainingDelay{ delay }
	, func{ func }
	, nextTask{ nullptr }
{
}

template<typename T>
	requires std::regular_invocable<T>
ScheduledTask& ScheduledTask::Chain(float delay, T func)
{
	nextTask = new ScheduledTask{ delay, func };
	return *nextTask;
}

template<typename T>
	requires std::regular_invocable<T>
ScheduledTaskUserWrapper ScheduledTaskUserWrapper::Then(T func)
{
	assert(!nextIsScheduled); // You've called Then() on the same task twice!
	nextIsScheduled = true;
	return ScheduledTaskUserWrapper{ task.Chain(0.0f, func) };
}

template<typename T>
	requires std::regular_invocable<T>
ScheduledTaskUserWrapper ScheduledTaskUserWrapper::Then(float delay, T func)
{
	assert(!nextIsScheduled); // You've called Then() on the same task twice!
	nextIsScheduled = true;
	return ScheduledTaskUserWrapper{ task.Chain(delay, func) };
}

template <typename T>
	requires std::regular_invocable<T>
ScheduledTaskUserWrapper Scheduler::Add(T func)
{
	return ScheduledTaskUserWrapper{ Add(0.0f, func) };
}

template<typename T>
	requires std::regular_invocable<T>
ScheduledTaskUserWrapper Scheduler::Add(float delay, T func)
{
	return ScheduledTaskUserWrapper{ buffer.emplace_back(delay, func) };
}

#pragma endregion // Definition
