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

	class IEventHandlerBase
	{
	protected:
		IEventHandlerBase();

	public:
		virtual ~IEventHandlerBase() = default;

		size_t INTERNAL_GetHandle() const;
		void INTERNAL_SetHandle(size_t newHandle);

	private:
		size_t handle;
	};

	template <typename EventType>
	class IEventHandlerIntermediate : public IEventHandlerBase
	{
	protected:
		using CallUserFunctionType = bool (*)(IEventHandlerBase* baseHandlerPtr, const void* eventData, size_t returnTypeHash, void* outReturnData);

		IEventHandlerIntermediate(CallUserFunctionType callUserFunction);

	public:
		void ProcessEventIntermediate(const EventType& event);
		bool ProcessEventIntermediate(const EventType& event, size_t returnTypeHash, void* outReturnData);

	private:
		CallUserFunctionType callUserFunction;

	};

}

// Inherit this for creating event handlers that get informed when a new event is added.
template <typename EventType, typename ReturnType = void>
class IEventHandler : public internal::IEventHandlerIntermediate<EventType>
{
protected:
	IEventHandler();

public:
	virtual ReturnType ProcessEvent(const EventType& event) = 0;

};
using EventHandlerHandle = size_t;

// Use this class to store an EventHandlerHandle that automatically deletes it from the event queue when it goes out of scope
class AutoEventHandler
{
public:
	AutoEventHandler(EventHandlerHandle handle = 0);
	AutoEventHandler(const AutoEventHandler&) = delete;
	~AutoEventHandler();

	void Assign(EventHandlerHandle newHandle);
	void Dispose();

private:
	EventHandlerHandle handle;
};

namespace internal {

	template <typename EventType, typename FuncType, typename ReturnType = std::invoke_result_t<FuncType, const EventType&>>
	class EventHandlerWrappingFunction : public IEventHandler<EventType, ReturnType>
	{
	public:
		EventHandlerWrappingFunction(FuncType&& func);
		virtual ReturnType ProcessEvent(const EventType& event) override;

	private:
		std::function<ReturnType(const EventType&)> func;
	};

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
		const EventType& AddEvent(EventType&& event);
		const EventType* GetEvent(int index) const;
		const std::vector<EventType>& GetEvents() const;

		void Clear() override;

	private:
		std::vector<EventType> queuedEvents;

	};

}

// Create this as a variable to pull events from EventsQueue.
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

	struct EventHandlersSet
	{
		std::vector<UPtr<internal::IEventHandlerBase>> eventHandlers;
		std::unordered_map<EventHandlerHandle, size_t> eventHandlerIndexLookup;

		// For calling event handlers on stored events when we don't know the EventType
		void (*callEventHandlersToProcessEventBuffer)(decltype(eventHandlers)&, internal::EventsBufferBase&){};
	};

public:
	// Add a new event here
	template <typename EventType>
	void AddEventForThisFrame(EventType&& event);
	template <typename EventType>
	void AddEventForNextFrame(EventType&& event);

	// Add a new event handler here
	template <typename EventType, typename EventHandlerType>
	EventHandlerHandle AddEventHandler(EventHandlerType&& eventHandler);
	template <typename EventType, typename FuncType>
	EventHandlerHandle AddEventHandlerFunc(FuncType&& func);
	// Delete an event handler
	void DeleteEventHandler(EventHandlerHandle handle);
	// Request for a return value from event handlers
	template <typename DesiredReturnType, typename EventType>
	std::optional<DesiredReturnType> RequestValueFromEventHandlers(const EventType& event) const;

	void NewFrame();

private:
	EventsBuffersSetType& GetCurrentBufferSet();
	EventsBuffersSetType& GetNextBufferSet();

	template <typename EventType>
	internal::EventsBuffer<EventType>& GetEventsBuffer(EventsBuffersSetType& bufferSet);
	template <typename EventType>
	EventHandlersSet& GetEventHandlersSet();

	template <typename EventType>
	void CallEventHandlers(const EventType& event);

public:
	int INTERNAL_GetCurrentFrameNum() const;
	template <typename EventType>
	const EventType* INTERNAL_GetEvent(int index);

private:
	// Double buffer, for adding events to the current or next frame.
	EventsBuffersSetType buffersSet[2];
	int activeBufferSetIndex;
	int frameNum;

	// Event handlers
	std::unordered_map<size_t, EventHandlersSet> eventHandlerSets;
	std::unordered_map<EventHandlerHandle, size_t> eventHandlerLookup;

};

#include "EventsQueue.ipp"
