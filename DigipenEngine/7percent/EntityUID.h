/******************************************************************************/
/*!
\file   EntityUID.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/22/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the interface file for a component that stores a UID per entity that
  is persistent between level loads, for the purposes of making hard connections
  to entities.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

//! The type of the UID
using EntityRefUID = uint64_t;

/*****************************************************************//*!
\class EntityIDComponent
\brief
	A component uniquely identifying entities.

	The purpose of this is to provide a way to link to specific entities within a scene,
	due to the volitility of entity hashes changing each scene load.
*//******************************************************************/
class EntityUIDComponent
	: public IRegisteredComponent<EntityUIDComponent>
	, public IHiddenComponent<EntityUIDComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	EntityUIDComponent();

	/*****************************************************************//*!
	\brief
		Copy constructor. This actually creates a whole new UID because entities
		should not ever share UID.
	\param copy
		The UID component to copy. (but does not copy)
	*//******************************************************************/
	EntityUIDComponent(const EntityUIDComponent& copy);

	/*****************************************************************//*!
	\brief
		Move constructor.
	\param other
		The UID component to move.
	*//******************************************************************/
	EntityUIDComponent(EntityUIDComponent&& other) noexcept;

	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~EntityUIDComponent();

	/*****************************************************************//*!
	\brief
		Gets the UID of this component.
	\return
		The UID of this component.
	*//******************************************************************/
	EntityRefUID GetUID() const;

private:
	/*****************************************************************//*!
	\brief
		If currently the default pool, registers the provided UID or generates one.
	\param specifiedUid
		A custom UID. If 0, generates a random UID.
	\return
		Registered UID. 0 if not the default pool.
	*//******************************************************************/
	EntityRefUID RegisterUID(EntityRefUID specifiedUid = 0);

	/*****************************************************************//*!
	\brief
		Unregisters this component from the lookup class.
	*//******************************************************************/
	void UnregisterUID();

public:
	// Serialization is trivial so it doesn't need to be explicitly implemented.
	// Deserialization requires subscription of the UID.
	/*****************************************************************//*!
	\brief
		Deserializes this component.
	\param deserializer
		The deserializer object.
	*//******************************************************************/
	void Deserialize(Deserializer& deserializer) override;

private:
	//! The UID of the entity
	EntityRefUID uid;

	property_vtable()
};
property_begin(EntityUIDComponent)
{
	property_var(uid)
}
property_vend_h(EntityUIDComponent)

/*****************************************************************//*!
\class EntityUIDLookup
\brief
	A central class that tracks all UID components in a lookup map.
*//******************************************************************/
class EntityUIDLookup
{
public:
	/*****************************************************************//*!
	\brief
		Generates a new UID and registers it to the lookup map.
	\return
		A new UID.
	*//******************************************************************/
	EntityRefUID GenerateAndRegisterNewUID();

	/*****************************************************************//*!
	\brief
		Associates a UID with an entity.
	\param uid
		The UID.
	\param entity
		The entity.
	*//******************************************************************/
	void RegisterUID(EntityRefUID uid, ecs::EntityHandle entity = nullptr);

	/*****************************************************************//*!
	\brief
		Unregisters a UID from the lookup map.
	\param uid
		The UID.
	*//******************************************************************/
	void UnregisterUID(EntityRefUID uid);

	/*****************************************************************//*!
	\brief
		Gets the entity associated with the specified UID.
	\param uid
		The UID.
	\return
		The entity associated with the UID. If an entity with the UID could not
		be found, returns nullptr.
	*//******************************************************************/
	static ecs::EntityHandle GetEntity(EntityRefUID uid);

private:
	//! The map of entity uid to entity handles (handles may not be valid)
	std::unordered_map<EntityRefUID, ecs::EntityHandle> uidToEntity;

};

/*****************************************************************//*!
\class EntityReference
\brief
	Serializeable references to an entity via UID.
*//******************************************************************/
class EntityReference : public ISerializeable
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param entity
		The entity to reference.
	*//******************************************************************/
	EntityReference(ecs::EntityHandle entity = nullptr);

	/*****************************************************************//*!
	\brief
		Assignment operator.
	\param entity
		The entity to reference.
	\return
		this.
	*//******************************************************************/
	EntityReference& operator=(ecs::EntityHandle otherEntity);

	/*****************************************************************//*!
	\brief
		Bool cast operator.
	\return
		Whether this entity reference is valid.
	*//******************************************************************/
	operator bool() const;

	/*****************************************************************//*!
	\brief
		ecs::EntityHandle cast operator.
	\return
		Gets the entity. nullptr if invalid.
	*//******************************************************************/
	operator ecs::EntityHandle() const;

	/*****************************************************************//*!
	\brief
		Dereference operator.
	\return
		Gets the entity. WILL CRASH IF INVALID!
	*//******************************************************************/
	ecs::Entity& operator*();

	/*****************************************************************//*!
	\brief
		Dereference operator.
	\return
		Gets the entity. WILL CRASH IF INVALID!
	*//******************************************************************/
	const ecs::Entity& operator*() const;

	/*****************************************************************//*!
	\brief
		Arrow operator.
	\return
		Gets the entity. WILL CRASH IF INVALID!
	*//******************************************************************/
	ecs::EntityHandle operator->();

	/*****************************************************************//*!
	\brief
		Arrow operator.
	\return
		Gets the entity. WILL CRASH IF INVALID!
	*//******************************************************************/
	ecs::ConstEntityHandle operator->() const;

	/*****************************************************************//*!
	\brief
		Explicitly checks whether this entity reference is valid.
	\return
		Whether this entity reference is valid.
	*//******************************************************************/
	bool IsValidReference() const;

	/*****************************************************************//*!
	\brief
		Draws this entity reference to the inspector window.
	\param label
		Label of this entity reference.
	\return
		Whether this entity reference was updated.
	*//******************************************************************/
	bool EditorDraw(const char* label);

private:
	/*****************************************************************//*!
	\brief
		Gets the entity, looks up the global mapping if null.
	\return
		The entity. nullptr if invalid.
	*//******************************************************************/
	ecs::EntityHandle GetEntity_LookupIfNull() const;

	/*****************************************************************//*!
	\brief
		Looks up the global mapping and returns the retrieved entity.
	\return
		The entity. nullptr if invalid.
	*//******************************************************************/
	ecs::EntityHandle GetEntity_LookupAlways() const;

public:
	/*****************************************************************//*!
	\brief
		Deserializes this entity reference from disk.
	\param reader
		The deserializer.
	*//******************************************************************/
	void Deserialize(Deserializer& reader) override;

private:
	//! The handle to the entity.
	mutable ecs::EntityHandle entityHandle;
	//! The UID of the entity.
	EntityRefUID entityUid;

public:
	property_vtable()
};
property_begin(EntityReference)
{
	property_var(entityUid)
}
property_vend_h(EntityReference)
