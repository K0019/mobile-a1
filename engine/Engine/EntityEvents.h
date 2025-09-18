/******************************************************************************/
/*!
\file   EntityEvents.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/22/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a component class that provides a standardized interface
  for broadcasting events to other components on the same class, at a faster speed and
  an easier interface than the global messaging system.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\brief
	A component that provides a mini messaging system per entity that
	other components can subscribe to.
*//******************************************************************/
class EntityEventsComponent : private IHiddenComponent<EntityEventsComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Subscribes to a certain event local to the attached entity.
	\tparam CompType
		The type of the component listening for the event.
	\tparam Args
		The arguments types of the event.
	\param eventName
		The name of the event.
	\param targetComp
		The component listening for the event.
	\param func
		The function to call when the event occurs.
	*//******************************************************************/
	template <typename CompType, typename ...Args>
	void Subscribe(const std::string& eventName, ecs::CompHandle<CompType> targetComp, void(CompType::*func)(Args...));

	/*****************************************************************//*!
	\brief
		Unsubscribes from a certain event local to the attached entity.
	\tparam CompType
		The type of the component listening for the event.
	\tparam Args
		The arguments types of the event.
	\param eventName
		The name of the event.
	\param targetComp
		The component listening for the event.
	\param func
		The function that was subscribed to the event.
	*//******************************************************************/
	template <typename CompType, typename ...Args>
	void Unsubscribe(const std::string& eventName, ecs::CompHandle<CompType> targetComp, void(CompType::*func)(Args...));

	/*****************************************************************//*!
	\brief
		Fires an event.
	\tparam Args
		The arguments types of the event.
	\param eventName
		The name of the event.
	\param params
		The arguments of the event.
	*//******************************************************************/
	template <typename ...Args>
	void BroadcastAll(const std::string& eventName, const Args&... params);

private:
	/*****************************************************************//*!
	\brief
		Generates a hash for an entity and function combination.
	*//******************************************************************/
	static uint64_t GenerateIdentifier(ecs::EntityHandle entity, size_t functionHash);

private:
	//! Local version of the events list.
	Messaging::Internal::EventsList eventsList;

};

template<typename CompType, typename ...Args>
void EntityEventsComponent::Subscribe(const std::string& eventName, ecs::CompHandle<CompType> targetComp, void(CompType::*func)(Args...))
{
	ecs::EntityHandle targetEntity{ ecs::GetEntity(targetComp) };
	eventsList.AddSubscriber(eventName, Messaging::Internal::Subscriber{
		GenerateIdentifier(targetEntity, typeid(func).hash_code()),
		std::function<void(Args...)>{ [targetEntity, func](Args... params) -> void {
			(targetEntity->GetComp<CompType>()->*func)(params...);
		} }
	});
}

template<typename CompType, typename ...Args>
void EntityEventsComponent::Unsubscribe(const std::string& eventName, ecs::CompHandle<CompType> targetComp, void(CompType::*func)(Args...))
{
	eventsList.RemoveSubscriber(eventName, GenerateIdentifier(ecs::GetEntity(targetComp), typeid(func).hash_code()));
}

template<typename ...Args>
void EntityEventsComponent::BroadcastAll(const std::string& eventName, const Args&... params)
{
	eventsList.BroadcastAll(eventName, params...);
}
