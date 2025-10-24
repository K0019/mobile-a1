#include "EventsQueue.h"

namespace internal {

	template<typename EventType>
	void EventsBuffer<EventType>::AddEvent(EventType&& event)
	{
		queuedEvents.push_back(std::forward<EventType>(event));
	}

	template<typename EventType>
	const EventType* EventsBuffer<EventType>::GetEvent(int index) const
	{
		if (static_cast<size_t>(index) >= queuedEvents.size())
			return nullptr;
		return &queuedEvents[index];
	}

	template <typename EventType>
	void EventsBuffer<EventType>::Clear()
	{
		queuedEvents.clear();
	}

}

template<typename EventType>
EventGetter<EventType>::EventGetter()
	: nextEventIndex{}
	, currentFrame{}
{
}

template<typename EventType>
const EventType* EventGetter<EventType>::ExtractEvent()
{
	// Reset next event index if the frame has changed.
	if (ST<EventsQueue>::Get()->INTERNAL_GetCurrentFrameNum() != currentFrame)
	{
		currentFrame = ST<EventsQueue>::Get()->INTERNAL_GetCurrentFrameNum();
		nextEventIndex = 0;
	}

	// Get the event at the next event index, then increment the index
	auto event{ ST<EventsQueue>::Get()->INTERNAL_GetEvent<EventType>(nextEventIndex) };
	if (event)
		++nextEventIndex;
	return event;
}

template<typename EventType>
void EventsQueue::AddEventForThisFrame(EventType&& event)
{
	GetEventsBuffer<EventType>(GetCurrentBufferSet()).AddEvent(std::forward<EventType>(event));
}
template<typename EventType>
void EventsQueue::AddEventForNextFrame(EventType&& event)
{
	GetEventsBuffer<EventType>(GetNextBufferSet()).AddEvent(std::forward<EventType>(event));
}

template<typename EventType>
internal::EventsBuffer<EventType>& EventsQueue::GetEventsBuffer(EventsBufferSetType& bufferSet)
{
	// Get and return if the buffer already exists
	size_t typeHash{ typeid(EventType).hash_code() };
	auto eventBufferIter{ bufferSet.find(typeHash) };
	if (eventBufferIter != bufferSet.end())
		return static_cast<internal::EventsBuffer<EventType>&>(*eventBufferIter->second);

	// Create the buffer and return
	auto result{ bufferSet.insert({ typeHash, std::make_unique<internal::EventsBuffer<EventType>>() }) };
	return static_cast<internal::EventsBuffer<EventType>&>(*result.first->second);
}

template<typename EventType>
const EventType* EventsQueue::INTERNAL_GetEvent(int index)
{
	return GetEventsBuffer<EventType>(GetCurrentBufferSet()).GetEvent(index);
}
