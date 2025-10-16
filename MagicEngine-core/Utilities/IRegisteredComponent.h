/******************************************************************************/
/*!
\file   IRegisteredComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (70%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\author Matthew Chan Shao Jie (30%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This is the interface file for components that are registered to a global list of components.
  This interface also requires components to have a method to serialize and deserialize them to/from file.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <unordered_map>
#include <string>
#include "Serializer.h"
#include "EditorHistory.h"
#include "IHiddenComponent.h"

namespace rj = rapidjson;

#pragma region Interface

/*****************************************************************//*!
\class IRegisteredComponent
\brief
	Components that inherit from this class are registered to a global list of components.
\tparam CompType
	The type of the component.
*//******************************************************************/
template <typename CompType>
class IRegisteredComponent : public ISerializeable
{
private:
	/*****************************************************************//*!
	\brief
		Registers the component type into RegisteredComponents.
	\return
		Dummy bool.
	*//******************************************************************/
	static bool RegisterComponent();

	//! Calls RegisterComponent() once when this static is initialized at startup
	inline static bool isRegistered{ RegisterComponent() };
private:
	// Methods specialized to the specified component type

	/*****************************************************************//*!
	\brief
		Gets the metadata of this component type stored in ecs.
	\return
		A pointer to the metadata of this component type.
	*//******************************************************************/
	static const ecs::CompTypeMeta* GetMetaData();

	/*****************************************************************//*!
	\brief
		Default constructs this component type and attaches it to the specified entity.
	\param entity
		The entity to attach the component to.
	*//******************************************************************/
	static void ConstructDefaultAndAttachTo(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Default constructs this component type and attaches it immediately to the specified entity.
	\param entity
		The entity to attach the component to.
	*//******************************************************************/
	static void* ConstructDefaultAndAttachNowTo(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Saves a component add event to history, so it can be undone.
	\param entity
		The entity to attach the component to.
	*//******************************************************************/
	static void SaveHistory_CompAdd(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Saves a component remove event to history, so it can be undone.
	\param entity
		The entity from which the component is removed.
	\param compPtr
		The component that is being removed.
	*//******************************************************************/
	static void SaveHistory_CompRemove(ecs::EntityHandle entity, const void* compPtr);

};

/*****************************************************************//*!
\struct RegisteredComponentData
\brief
	A structure that stores data as necessary for registration of a component type.
*//******************************************************************/
struct RegisteredComponentData
{
public:
	//! A nice name for this component
	std::string name;
	//! Obtain ecs metadata of this component (could be nullptr if a component of this type has not been added into ecs yet)
	const ecs::CompTypeMeta* (*GetMetaData)();
	//! Whether this component is hidden from the editor (inherits from IHiddenComponent)
	bool isEditorHidden;

	//! Default construct the component and attach it to the specified entity.
	void (*ConstructDefaultAndAttachTo)(ecs::EntityHandle entity);
	//! Default construct the component and attach it immediately to the specified entity.
	void* (*ConstructDefaultAndAttachNowTo)(ecs::EntityHandle entity);

	//! Save component add event to history
	void (*SaveHistory_CompAdd)(ecs::EntityHandle entity);
	//! Save component remove event to history
	void (*SaveHistory_CompRemove)(ecs::EntityHandle entity, const void* compPtr);

	//! The function that serializes the component to file.
	void(*SerializeFuncPtr)(const void* compPtr, Serializer& writer);
	//! The function that deserializes the component from file.
	void (*DeserializeFuncPtr)(void* compPtr, Deserializer& entry);

};

/*****************************************************************//*!
\class RegisteredComponents
\brief
	The interface to query for registered components types.
*//******************************************************************/
class RegisteredComponents
{
public:
	using ConstIterator = std::unordered_map<ecs::CompHash, RegisteredComponentData>::const_iterator;

	/*****************************************************************//*!
	\brief
		Gets a registered component's data.
	\tparam CompType
		The component type.
	\return
		A pointer to the component type's data. If the type is not a registered component type, returns nullptr.
	*//******************************************************************/
	template <typename CompType>
	static const RegisteredComponentData* GetData();

	/*****************************************************************//*!
	\brief
		Gets a registered component's data.
	\param compHash
		The hash of the component type.
	\return
		A pointer to the component type's data. If the type is not a registered component type, returns nullptr.
	*//******************************************************************/
	static const RegisteredComponentData* GetData(ecs::CompHash compHash);

	/*****************************************************************//*!
	\brief
		Gets an iterator to the beginning of all registered component types' data.
	\return
		A const iterator to the beginning of all registered component types' data.
	*//******************************************************************/
	static ConstIterator Begin();

	/*****************************************************************//*!
	\brief
		Gets an iterator to 1 past the end of all registered component types' data.
	\return
		A const iterator to 1 past the end of all registered component types' data.
	*//******************************************************************/
	static ConstIterator End();

public:
	/*****************************************************************//*!
	\brief
		For internal use, to register a component type on startup.
	\param compHash
		The hash of the component type.
	\param data
		The data of the component type.
	*//******************************************************************/
	static void RegisterComponent(ecs::CompHash compHash, RegisteredComponentData&& data);

private:
	//! All component type data is stored in this map, that maps component type hashes to component type data.
	std::unordered_map<ecs::CompHash, RegisteredComponentData> dataMap;

};

#pragma endregion // Interface


#pragma region Definition

template<typename CompType>
bool IRegisteredComponent<CompType>::RegisterComponent()
{
	RegisteredComponents::RegisterComponent(ecs::GetCompHash<CompType>(), RegisteredComponentData{
		.name = util::GetNiceTypeName<CompType>(),
		.GetMetaData = IRegisteredComponent<CompType>::GetMetaData,
		.isEditorHidden = std::is_base_of_v<IHiddenComponent<CompType>, CompType>,
		.ConstructDefaultAndAttachTo = IRegisteredComponent<CompType>::ConstructDefaultAndAttachTo,
		.ConstructDefaultAndAttachNowTo = IRegisteredComponent<CompType>::ConstructDefaultAndAttachNowTo,
		.SaveHistory_CompAdd = IRegisteredComponent<CompType>::SaveHistory_CompAdd,
		.SaveHistory_CompRemove = IRegisteredComponent<CompType>::SaveHistory_CompRemove,
		.SerializeFuncPtr = [](const void* compPtr, Serializer& writer) -> void {
			reinterpret_cast<const CompType*>(compPtr)->Serialize(writer);
		},
		.DeserializeFuncPtr = [](void* compPtr, Deserializer& reader) -> void {
			reinterpret_cast<CompType*>(compPtr)->Deserialize(reader);
		}
		});

	return true;
}

template<typename CompType>
const ecs::CompTypeMeta* IRegisteredComponent<CompType>::GetMetaData()
{
	return ecs::GetCompMeta<CompType>();
}

template<typename CompType>
void IRegisteredComponent<CompType>::ConstructDefaultAndAttachTo(ecs::EntityHandle entity)
{
	entity->AddComp(CompType{});
}

template<typename CompType>
void* IRegisteredComponent<CompType>::ConstructDefaultAndAttachNowTo(ecs::EntityHandle entity)
{
	return entity->AddCompNow(CompType{});
}

template<typename CompType>
void IRegisteredComponent<CompType>::SaveHistory_CompAdd(ecs::EntityHandle entity)
{
	ST<History>::Get()->OneEvent(HistoryEvent_CompAdd<CompType>{ entity });
}

template<typename CompType>
void IRegisteredComponent<CompType>::SaveHistory_CompRemove(ecs::EntityHandle entity, const void* compPtr)
{
	ST<History>::Get()->OneEvent(HistoryEvent_CompRemove<CompType>{ entity, *reinterpret_cast<const CompType*>(compPtr) });
}

template<typename CompType>
const RegisteredComponentData* RegisteredComponents::GetData()
{
	return GetData(ecs::GetCompHash<CompType>());
}

#pragma endregion // Definition
