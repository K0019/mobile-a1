/******************************************************************************/
/*!
\file   EventsQueue.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides a buffer to queue events for event handlers to later pull.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

namespace internal {

	class EventsBufferBase
	{
	public:
		virtual ~EventsBufferBase() = default;
		virtual void Clear() = 0;
	};

	// Saves the event objects into a vector
	template <typename EventType>
	class EventsBuffer : public EventsBufferBase
	{
	public:
		void AddEvent(EventType&& event);
		const EventType* GetEvent(int index) const;
		void Clear() override;

	private:
		std::vector<EventType> queuedEvents;

	};

}

// Create this as a variable to obtain events from EventsQueue.
template <typename EventType>
class EventsReader
{
public:
	EventsReader();

	// Reads next event, if there is one available
	const EventType* ExtractEvent();

private:
	//! Tracks the next event index for the next extraction
	int nextEventIndex;
	//! Tracks whether to reset the current index
	int currentFrame;
};

// EventsQueue singleton, use this to add events.
class EventsQueue
{
private:
	EventsQueue();
	friend ST<EventsQueue>;

	using EventsBuffersSetType = std::unordered_map<size_t, UPtr<internal::EventsBufferBase>>;

public:
	template <typename EventType>
	void AddEventForThisFrame(EventType&& event);
	template <typename EventType>
	void AddEventForNextFrame(EventType&& event);

	void NewFrame();

private:
	EventsBuffersSetType& GetCurrentBufferSet();
	EventsBuffersSetType& GetNextBufferSet();

	template <typename EventType>
	internal::EventsBuffer<EventType>& GetEventsBuffer(EventsBuffersSetType& bufferSet);

public:
	int INTERNAL_GetCurrentFrameNum() const;
	template <typename EventType>
	const EventType* INTERNAL_GetEvent(int index);

private:
	// Double buffer, for adding events to the current or next frame.
	EventsBuffersSetType buffersSet[2];
	int activeBufferSetIndex;
	int frameNum;

};

#include "EventsQueue.ipp"
