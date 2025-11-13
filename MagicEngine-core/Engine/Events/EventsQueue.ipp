#include "EventsQueue.h"

namespace internal {

	template<typename EventType>
	IEventHandlerIntermediate<EventType>::IEventHandlerIntermediate(CallUserFunctionType callUserFunction)
		: callUserFunction{ callUserFunction }
	{
	}

	template<typename EventType>
	void IEventHandlerIntermediate<EventType>::ProcessEventIntermediate(const EventType& event)
	{
		callUserFunction(this, &event, 0, nullptr);
	}
	template<typename EventType>
	bool IEventHandlerIntermediate<EventType>::ProcessEventIntermediate(const EventType& event, size_t returnTypeHash, void* outReturnData)
	{
		return callUserFunction(this, &event, returnTypeHash, outReturnData);
	}

}

template<typename EventType, typename ReturnType>
IEventHandler<EventType, ReturnType>::IEventHandler()
	: internal::IEventHandlerIntermediate<EventType>{ [](internal::IEventHandlerBase* baseHandlerPtr, const void* eventData, size_t returnTypeHash, void* outReturnData) -> bool {
		auto eventHandlerPtr{ static_cast<IEventHandler<EventType, ReturnType>*>(baseHandlerPtr) };

		// Special value: 0 (new event was added)
		// Just call the user function and don't care about the return value
		if (returnTypeHash == 0)
		{
			eventHandlerPtr->ProcessEvent(*reinterpret_cast<const EventType*>(eventData));
			return true;
		}

		// We want the return value (which can only work when non-void,
		// so if we care about the return value but our event handler returns void, do nothing)
		if constexpr (!std::is_same_v<ReturnType, void>)
		{
			// If the return type doesn't match, don't call this handler
			if (typeid(ReturnType).hash_code() != returnTypeHash)
				return false;

			// Call the handler and write the result to the out data ptr
			*reinterpret_cast<ReturnType*>(outReturnData) = eventHandlerPtr->ProcessEvent(*reinterpret_cast<const EventType*>(eventData));
			return true;
		}
		else
			return false;
	} }
{
}

namespace internal {

	template<typename EventType, typename FuncType, typename ReturnType>
	EventHandlerWrappingFunction<EventType, FuncType, ReturnType>::EventHandlerWrappingFunction(FuncType&& func)
		: func{ std::forward<FuncType>(func) }
	{
	}

	template<typename EventType, typename FuncType, typename ReturnType>
	ReturnType EventHandlerWrappingFunction<EventType, FuncType, ReturnType>::ProcessEvent(const EventType& event)
	{
		return func(event);
	}

	template<typename EventType>
	const EventType& EventsBuffer<EventType>::AddEvent(EventType&& event)
	{
		queuedEvents.push_back(std::forward<EventType>(event));
		return queuedEvents.back();
	}

	template<typename EventType>
	const EventType* EventsBuffer<EventType>::GetEvent(int index) const
	{
		if (static_cast<size_t>(index) >= queuedEvents.size())
			return nullptr;
		return &queuedEvents[index];
	}

	template<typename EventType>
	const std::vector<EventType>& EventsBuffer<EventType>::GetEvents() const
	{
		return queuedEvents;
	}

	template <typename EventType>
	void EventsBuffer<EventType>::Clear()
	{
		queuedEvents.clear();
	}

}

template<typename EventType>
EventsReader<EventType>::EventsReader()
	: nextEventIndex{}
	, currentFrame{}
{
}

template<typename EventType>
const EventType* EventsReader<EventType>::ExtractEvent()
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
template<typename FuncType>
	requires std::regular_invocable<FuncType, const EventType&>
bool EventsReader<EventType>::ForEach(FuncType func)
{
	EventsReader<EventType> events{};
	while (auto event{ events.ExtractEvent() })
		func(*event);
	return events.nextEventIndex != 0;
}

template<typename EventType>
void EventsQueue::AddEventForThisFrame(EventType&& event)
{
	const EventType& storedEvent{ GetEventsBuffer<EventType>(GetCurrentBufferSet()).AddEvent(std::forward<EventType>(event)) };
	CallEventHandlers(storedEvent);
}
template<typename EventType>
void EventsQueue::AddEventForNextFrame(EventType&& event)
{
	GetEventsBuffer<EventType>(GetNextBufferSet()).AddEvent(std::forward<EventType>(event));
}

template <typename EventType, typename EventHandlerType>
EventHandlerHandle EventsQueue::AddEventHandler(EventHandlerType&& eventHandler)
{
	// Generate event handler handle
	EventHandlerHandle handle{ util::Rand_UID() };
	while (eventHandlerLookup.find(handle) != eventHandlerLookup.end())
		handle = util::Rand_UID();
	eventHandlerLookup.insert({ handle, typeid(EventType).hash_code() });
	eventHandler.INTERNAL_SetHandle(handle);

	// Add the event handler
	auto& eventSet{ GetEventHandlersSet<EventType>() };
	eventSet.eventHandlerIndexLookup.insert({ handle, eventSet.eventHandlers.size() });
	eventSet.eventHandlers.emplace_back(new EventHandlerType{ std::forward<EventHandlerType>(eventHandler) });

	return handle;
}

template<typename EventType, typename FuncType>
EventHandlerHandle EventsQueue::AddEventHandlerFunc(FuncType&& func)
{
	return AddEventHandler<EventType>(internal::EventHandlerWrappingFunction<EventType, FuncType>{ std::forward<FuncType>(func) });
}

template<typename DesiredReturnType, typename EventType>
std::optional<DesiredReturnType> EventsQueue::RequestValueFromEventHandlers(const EventType& event) const
{
	// Check if any event handlers exist for this EventType
	size_t eventTypeHash{ typeid(EventType).hash_code() };
	auto eventSetIter{ eventHandlerSets.find(eventTypeHash) };
	if (eventSetIter == eventHandlerSets.end())
		return std::nullopt;

	// Return the return value of the first event handler that returns one
	size_t returnTypeHash{ typeid(DesiredReturnType).hash_code() };
	DesiredReturnType returnVal{};
	for (const auto& eventHandler : eventSetIter->second.eventHandlers)
		if (static_cast<internal::IEventHandlerIntermediate<EventType>&>(*eventHandler).ProcessEventIntermediate(event, returnTypeHash, &returnVal))
			return returnVal;

	// No event handlers returned the desired return value type
	return std::nullopt;
}

template<typename EventType>
internal::EventsBuffer<EventType>& EventsQueue::GetEventsBuffer(EventsBuffersSetType& bufferSet)
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
EventsQueue::EventHandlersSet& EventsQueue::GetEventHandlersSet()
{
	// Return immediately if the struct was already initialized
	EventHandlersSet& eventHandlersSet{ eventHandlerSets[typeid(EventType).hash_code()] };
	if (eventHandlersSet.callEventHandlersToProcessEventBuffer)
		return eventHandlersSet;

	// The struct was just created, and we need to initialize the function
	eventHandlersSet.callEventHandlersToProcessEventBuffer = [](decltype(eventHandlersSet.eventHandlers)& eventHandlers, internal::EventsBufferBase& eventBuffers) -> void {
		for (const auto& event : static_cast<internal::EventsBuffer<EventType>&>(eventBuffers).GetEvents())
			for (const auto& eventHandler : eventHandlers)
				static_cast<internal::IEventHandlerIntermediate<EventType>&>(*eventHandler).ProcessEventIntermediate(event);
	};

	return eventHandlersSet;
}

template<typename EventType>
void EventsQueue::CallEventHandlers(const EventType& event)
{
	// Check if any event handlers exist for this EventType
	size_t eventTypeHash{ typeid(EventType).hash_code() };
	auto eventSetIter{ eventHandlerSets.find(eventTypeHash) };
	if (eventSetIter == eventHandlerSets.end())
		return;

	// Call the handlers
	for (const auto& eventHandler : eventSetIter->second.eventHandlers)
		static_cast<internal::IEventHandlerIntermediate<EventType>&>(*eventHandler).ProcessEventIntermediate(event);
}

template<typename EventType>
const EventType* EventsQueue::INTERNAL_GetEvent(int index)
{
	return GetEventsBuffer<EventType>(GetCurrentBufferSet()).GetEvent(index);
}
