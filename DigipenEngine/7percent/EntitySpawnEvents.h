/******************************************************************************/
/*!
\file   EntitySpawnEvents.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/22/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a class that registers callback functions listening
  for entity creation events.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\class EntitySpawnEvents
\brief
	Executes functions whenever entities are created or destroyed.
*//******************************************************************/
class EntitySpawnEvents
{
	using CallbackFuncSig = void(*)(ecs::EntityHandle);

public:
	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~EntitySpawnEvents();

private:
	friend ST<EntitySpawnEvents>;

	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	EntitySpawnEvents();

	/*****************************************************************//*!
	\brief
		Registers a function to be called when an entity is created.
	\param func
		The function to be called.
	*//******************************************************************/
	void RegisterEvent(CallbackFuncSig func);

	/*****************************************************************//*!
	\brief
		Registers a function that attaches a component to an entity when it is created.
	\tparam T
		The component type to be attached.
	*//******************************************************************/
	template <typename T>
	void RegisterAttachEvent();

	/*****************************************************************//*!
	\brief
		Unregisters a function that is called when an entity is created.
	\param func
		The function that is called.
	*//******************************************************************/
	void UnregisterEvent(CallbackFuncSig func);

	/*****************************************************************//*!
	\brief
		Unregisters a function that attaches a component to an entity when it is created.
	\tparam T
		The component type to be attached.
	*//******************************************************************/
	template <typename T>
	void UnregisterAttachEvent();

	/*****************************************************************//*!
	\brief
		Unregisters all functions that were registered by EntitySpawnEvents.
	*//******************************************************************/
	void UnregisterAll();

private:
	/*****************************************************************//*!
	\brief
		Returns a function that attaches a component to an entity.
	\tparam T
		The type of component to attach.
	\return
		The function attaching a component to an entity when called.
	*//******************************************************************/
	template <typename T>
	static CallbackFuncSig GenerateAttachFunc();

private:
	//! The set of registered functions via this class.
	std::set<CallbackFuncSig> registeredFunctions;

	//! The name of the event used for entity creations
	const std::string entityCreatedEventName;

};

template<typename T>
void EntitySpawnEvents::RegisterAttachEvent()
{
	RegisterEvent(GenerateAttachFunc<T>());
}

template<typename T>
void EntitySpawnEvents::UnregisterAttachEvent()
{
	UnregisterEvent(GenerateAttachFunc<T>());
}

template<typename T>
EntitySpawnEvents::CallbackFuncSig EntitySpawnEvents::GenerateAttachFunc()
{
	return [](ecs::EntityHandle entity) -> void {
		// In case the entity was cloned, in which case the component is already attached.
		if (!entity->GetComp<T>())
			entity->AddCompNow(T{});
	};
}
