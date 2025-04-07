/******************************************************************************/
/*!
\file   EntityUID.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/22/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a component that stores a UID per entity that
  is persistent between level loads, for the purposes of making hard connections
  to entities.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "EntityUID.h"
#include "NameComponent.h"

EntityUIDComponent::EntityUIDComponent()
	: uid{ RegisterUID() }
{
}

EntityUIDComponent::EntityUIDComponent(const EntityUIDComponent&)
	: uid{ RegisterUID() } // We don't want to copy UIDs since we want all entities to have unique UID
{
}

EntityUIDComponent::EntityUIDComponent(EntityUIDComponent&& other) noexcept
	: uid{ other.uid }
{
	other.uid = 0;
}

EntityUIDComponent::~EntityUIDComponent()
{
	if (!uid)
		return;
	UnregisterUID();
}

EntityRefUID EntityUIDComponent::GetUID() const
{
	return uid;
}

EntityRefUID EntityUIDComponent::RegisterUID(EntityRefUID specifiedUid)
{
	if (ecs::GetCurrentPoolId() != ecs::POOL::DEFAULT)
		return 0;

	if (specifiedUid)
	{
		ST<EntityUIDLookup>::Get()->RegisterUID(specifiedUid);
		return specifiedUid;
	}
	else
		return ST<EntityUIDLookup>::Get()->GenerateAndRegisterNewUID();
}

void EntityUIDComponent::UnregisterUID()
{
	ST<EntityUIDLookup>::Get()->UnregisterUID(uid);
}

void EntityUIDComponent::Deserialize(Deserializer& deserializer)
{
	bool isDefaultPool{ ecs::GetCurrentPoolId() == ecs::POOL::DEFAULT };

	if (isDefaultPool)
		ST<EntityUIDLookup>::Get()->UnregisterUID(uid);
	deserializer.DeserializeVar("uid", &uid);
	if (isDefaultPool)
		ST<EntityUIDLookup>::Get()->RegisterUID(uid, ecs::GetEntity(this));
}

EntityRefUID EntityUIDLookup::GenerateAndRegisterNewUID()
{
	while (true)
	{
		EntityRefUID candidateUID{ util::Rand_UID() };
		if (uidToEntity.find(candidateUID) != uidToEntity.end())
			continue;
		uidToEntity.try_emplace(candidateUID, nullptr); // We'll get the entity handle later
		return candidateUID;
	}
}

void EntityUIDLookup::RegisterUID(EntityRefUID uid, ecs::EntityHandle entity)
{
	auto emplaceResult{ uidToEntity.try_emplace(uid, entity) };

	// If the UID failed to emplace, the UID already exists. Try to replace the entity handle.
	if (!emplaceResult.second && entity)
	{
		CONSOLE_LOG(LEVEL_WARNING) << "UID CLASH DETECTED! \"" << uid << "\". If entity reference issues occur, please try manually modifying this UID in the scene file.";
		emplaceResult.first->second = entity;
	}
}

void EntityUIDLookup::UnregisterUID(EntityRefUID uid)
{
	uidToEntity.erase(uid);
}

ecs::EntityHandle EntityUIDLookup::GetEntity(EntityRefUID uid)
{
	auto& uidToEntityMap{ ST<EntityUIDLookup>::Get()->uidToEntity };

	auto entityIter{ uidToEntityMap.find(uid) };
	// If the UID isn't registered, return nullptr
	if (entityIter == uidToEntityMap.end())
		return nullptr;

	if (ecs::IsEntityHandleValid(entityIter->second))
		return entityIter->second;
	// The handle is invalid, search for the entity with the specified UID.
	for (auto uidCompIter{ ecs::GetCompsBegin<EntityUIDComponent>() }, endCompIter{ ecs::GetCompsEnd<EntityUIDComponent>() }; uidCompIter != endCompIter; ++uidCompIter)
		if (uidCompIter->GetUID() == uid)
		{
			entityIter->second = uidCompIter.GetEntity();
			return entityIter->second;
		}
	// An entity with the requested uid doesn't exist.
	return nullptr;
}

EntityReference::EntityReference(ecs::EntityHandle entity)
	: entityHandle{ entity }
	, entityUid{ entity ? entity->GetComp<EntityUIDComponent>()->GetUID() : 0 }
{
}

EntityReference& EntityReference::operator=(ecs::EntityHandle otherEntity)
{
	entityHandle = otherEntity;
	entityUid = (entityHandle ? otherEntity->GetComp<EntityUIDComponent>()->GetUID() : 0);
	return *this;
}

EntityReference::operator bool() const
{
	return IsValidReference();
}

EntityReference::operator ecs::EntityHandle() const
{
	// TODO: Use messaging system to listen for entity handle changes etc.
	return GetEntity_LookupAlways();
}

ecs::Entity& EntityReference::operator*()
{
	return *GetEntity_LookupIfNull();
}
const ecs::Entity& EntityReference::operator*() const
{
	return *GetEntity_LookupIfNull();
}

ecs::EntityHandle EntityReference::operator->()
{
	return GetEntity_LookupIfNull();
}
ecs::ConstEntityHandle EntityReference::operator->() const
{
	return GetEntity_LookupIfNull();
}

bool EntityReference::IsValidReference() const
{
	if (entityUid == 0)
		return false;
	// Explicitly check whether the reference is valid
	return entityHandle = EntityUIDLookup::GetEntity(entityUid);
}

bool EntityReference::EditorDraw(const char* label)
{
	bool isValidEntity{ IsValidReference() };
	bool modified{ false };

	gui::TextBoxReadOnly(label,
		(isValidEntity ?
			ICON_FA_CUBE + entityHandle->GetComp<NameComponent>()->GetName() :
			std::string{ "No entity assigned." })
	);
	gui::PayloadTarget<ecs::EntityHandle>("ENTITY", [&entityHandle = entityHandle, &entityUid = entityUid, &modified](const ecs::EntityHandle& entity) -> void {
		entityHandle = entity;
		entityUid = entity->GetComp<EntityUIDComponent>()->GetUID();
		modified = true;
	});

	if (isValidEntity)
		if (gui::ItemContextMenu contextMenu{ "Assigned Entity" })
			if (gui::MenuItem("Clear"))
			{
				*this = nullptr;
				modified = true;
			}

	return modified;
}

ecs::EntityHandle EntityReference::GetEntity_LookupIfNull() const
{
	if (entityHandle)
		return entityHandle;
	return entityHandle = EntityUIDLookup::GetEntity(entityUid);
}

ecs::EntityHandle EntityReference::GetEntity_LookupAlways() const
{
	return entityHandle = EntityUIDLookup::GetEntity(entityUid);
}

void EntityReference::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);

	entityHandle = EntityUIDLookup::GetEntity(entityUid);
}
