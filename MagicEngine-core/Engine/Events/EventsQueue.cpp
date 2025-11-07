/******************************************************************************/
/*!
\file   EventsQueue.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides a buffer to queue events for event handlers to later pull.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Events/EventsQueue.h"

namespace internal {

	IEventHandlerBase::IEventHandlerBase()
		: handle{}
	{
	}

	size_t IEventHandlerBase::INTERNAL_GetHandle() const
	{
		return handle;
	}

	void IEventHandlerBase::INTERNAL_SetHandle(size_t newHandle)
	{
		handle = newHandle;
	}

}

AutoEventHandler::AutoEventHandler(EventHandlerHandle handle)
	: handle{ handle }
{
}

AutoEventHandler::~AutoEventHandler()
{
	Dispose();
}

void AutoEventHandler::Assign(EventHandlerHandle newHandle)
{
	Dispose();
	handle = newHandle;
}

void AutoEventHandler::Dispose()
{
	if (handle && ST<EventsQueue>::IsInitialized())
		ST<EventsQueue>::Get()->DeleteEventHandler(handle);
}

EventsQueue::EventsQueue()
	: activeBufferSetIndex{}
	, frameNum{}
{
}

void EventsQueue::DeleteEventHandler(EventHandlerHandle handle)
{
	// Find the event handler set containing this event handler
	auto eventHandlerHashIter{ eventHandlerLookup.find(handle) };
	if (eventHandlerHashIter == eventHandlerLookup.end())
		return;

	// Get the index of the event handler within the vector
	auto& eventHandlerSet{ eventHandlerSets.at(eventHandlerHashIter->second) };
	size_t eventHandlerIndex{ eventHandlerSet.eventHandlerIndexLookup.at(handle) };

	// If the event handler is not the last in the vector, swap with the last
	if (eventHandlerIndex < eventHandlerSet.eventHandlers.size() - 1)
	{
		EventHandlerHandle lastEventHandlerHandle{ eventHandlerSet.eventHandlers.back()->INTERNAL_GetHandle() };
		std::swap(eventHandlerSet.eventHandlers[eventHandlerIndex], eventHandlerSet.eventHandlers.back());
		eventHandlerSet.eventHandlerIndexLookup[handle] = eventHandlerIndex;
	}

	// Delete the event handler
	eventHandlerSet.eventHandlers.pop_back();
	eventHandlerSet.eventHandlerIndexLookup.erase(handle);
}

void EventsQueue::NewFrame()
{
	// Clear events in current frame
	for (auto& [_, buffer] : GetCurrentBufferSet())
		buffer->Clear();

	// Swap buffers
	++frameNum;
	activeBufferSetIndex = (~activeBufferSetIndex) & 1;

	// Call event handlers
	auto& currentBufferSet{ GetCurrentBufferSet() };
	for (auto& eventHandlerIter : eventHandlerSets)
	{
		// Skip this set of event handlers if there are no events
		auto bufferIter{ currentBufferSet.find(eventHandlerIter.first) };
		if (bufferIter == currentBufferSet.end())
			continue;

		// Execute
		eventHandlerIter.second.callEventHandlersToProcessEventBuffer(eventHandlerIter.second.eventHandlers, *bufferIter->second);
	}
}

EventsQueue::EventsBuffersSetType& EventsQueue::GetCurrentBufferSet()
{
	return buffersSet[activeBufferSetIndex];
}

EventsQueue::EventsBuffersSetType& EventsQueue::GetNextBufferSet()
{
	return buffersSet[(~activeBufferSetIndex) & 1];
}

int EventsQueue::INTERNAL_GetCurrentFrameNum() const
{
	return frameNum;
}
