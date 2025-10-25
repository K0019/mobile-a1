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

EventsQueue::EventsQueue()
	: activeBufferSetIndex{}
	, frameNum{}
{
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
