/******************************************************************************/
/*!
\file   ECS.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements non-template functions declared in the
  interface header file for the Entity Component System (ECS).

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ECSSysLayers.h"
#include "ECS.h"

namespace ecs {

#pragma region Globals

#define X(name, callbacksEnabled) callbacksEnabled,
	constexpr bool compCallbacksEnabledForPool[]{ M_ECS_POOL };
#undef X

#pragma endregion

#pragma region Management

	void Initialize()
	{
		internal::CurrentPool::Init(compCallbacksEnabledForPool[0]);
	}

	void Shutdown()
	{
		internal::CurrentPool::Shutdown();
	}

#pragma endregion // Management

#pragma region Entities

	/*****************************************************************//*!
	\brief
		Broadcasts to messaging system that an entity was created. This was created to facilitate
		systems that require adding a component to all entities immediately when they are created.
	*//******************************************************************/
	void BroadcastEntityCreated(ecs::EntityHandle entity)
	{
		Messaging::BroadcastAll("OnEntityCreated", entity);
	}

	Entity::Entity(internal::EntityHash mapKey)
		: internal::Entity_Internal{ mapKey }
	{
	}

	bool Entity::RemoveComp(ecs::CompHash compHash)
	{
		internal::EntCompMapType::iterator compIndexIter;
		if (!CheckCanRemoveComp(compHash, compIndexIter))
			return false;

		// If this component is currently pending for addition, remove it from the buffer
		if (compIndexIter->second & COMP_STATUS_TO_ADD)
		{
			internal::CurrentPool::ChangesBuffer().RemoveCompBufferedForAddition(compHash, compIndexIter->second & COMP_STATUS_UNUSED_BITS);
			components.erase(compIndexIter);
			return true;
		}

		// Add component to remove buffer
		internal::CurrentPool::ChangesBuffer().RemoveComp(this, compHash);

		// Mark component as pending deletion from this entity
		compIndexIter->second |= COMP_STATUS_TO_REMOVE;

		return true;
	}

	bool Entity::RemoveCompNow(ecs::CompHash compHash)
	{
		internal::EntCompMapType::iterator compIndexIter;
		if (!CheckCanRemoveComp(compHash, compIndexIter))
			return false;

		// If this component is currently pending for addition, remove it from the buffer
		if (compIndexIter->second & COMP_STATUS_TO_ADD)
			internal::CurrentPool::ChangesBuffer().RemoveCompBufferedForAddition(compHash, compIndexIter->second & COMP_STATUS_UNUSED_BITS);
		else
		{
			// Remove component from CompArr
			internal::CompArr& compArr{ internal::GetCompArr(compHash) };
			// We already have a handle to the component to remove. Don't make CompArr search for us to remove the component from us.
			compArr.RemoveComp(compIndexIter->second, false);
		}

		// Unregister component from this entity
		components.erase(compIndexIter);

		return true;
	}

	bool Entity::HasComp(CompHash compHash)
	{
		return components.contains(compHash);
	}

	CompHandle<void> Entity::GetComp(CompHash compHash)
	{
		return reinterpret_cast<CompHandle<void>>(INTERNAL_GetCompRaw(compHash));
	}

	ConstCompHandle<void> Entity::GetComp(CompHash compHash) const
	{
		return reinterpret_cast<ConstCompHandle<void>>(INTERNAL_GetCompRaw(compHash));
	}

	EntityCompsIterator Entity::Comps_Begin()
	{
		return Comps_User_Begin<CompHandle>();
	}
	ConstEntityCompsIterator Entity::Comps_Begin() const
	{
		return Comps_User_Begin<ConstCompHandle>();
	}
	
	EntityCompsIterator Entity::Comps_End()
	{
		return Comps_User_End<CompHandle>();
	}
	ConstEntityCompsIterator Entity::Comps_End() const
	{
		return Comps_User_End<ConstCompHandle>();
	}

	void Entity::SetActive(bool active)
	{
		// TODO: Track which components were originally inactive. Also track whether the whole entity is set to be inactive, independent of the components' activeness.

		for (const auto& compIndexIter : components)
			internal::CurrentPool::ChangesBuffer().ChangeCompActiveness(this, compIndexIter.first, !active);

		// Design wise, this should be recursive
		for (Transform* child : GetTransform().GetChildren())
			child->GetEntity()->SetActive(active);
	}

	EntityHandle Entity::GetHandle()
	{
		return this;
	}
	ConstEntityHandle Entity::GetHandle() const
	{
		return this;
	}

	EntityHash Entity::GetHash() const
	{
		return INTERNAL_GetMapKey();
	}

	Transform& Entity::GetTransform()
	{
		return transform;
	}
	const Transform& Entity::GetTransform() const
	{
		return transform;
	}


	/*****************************************************************//*!
	\brief
		Creates an empty entity. If a parent is specified, the new entity is parented to it.
		This is a version of CreateEntity() that does not broadcast its creation, to allow
		for code paths that need to work on the entity before broadcasting creation.
	\param parent
		Optional parameter specifying the entity that the new empty entity will be parented to.
		If nullptr, the new entity is unparented.
	\return
		EntityHandle to the newly created entity.
	*//******************************************************************/
	EntityHandle CreateEntity_NoBroadcast(EntityHandle parent)
	{
		return reinterpret_cast<EntityHandle>(internal::CurrentPool::Entities().CreateEntity(reinterpret_cast<internal::InternalEntityHandle>(parent)));
	}
	/*****************************************************************//*!
	\brief
		Creates an empty entity, copying the values of the provided Transform into its own Transform.
		If the provided Transform is parented to another entity, the new entity will also be parented to that entity.
		Child entities attached to the provided transform are ignored.
		This is a version of CreateEntity() that does not broadcast its creation, to allow
		for code paths that need to work on the entity before broadcasting creation.
	\param transformCopy
		The Transform whose values will be copied into the new empty entity.
	\return
		EntityHandle to the newly created entity.
	*//******************************************************************/
	EntityHandle CreateEntity_NoBroadcast(Transform& transformCopy)
	{
		return reinterpret_cast<EntityHandle>(internal::CurrentPool::Entities().CreateEntity(transformCopy));
	}

	EntityHandle CreateEntity(EntityHandle parent)
	{
		ecs::EntityHandle entity{ CreateEntity_NoBroadcast(parent) };
		BroadcastEntityCreated(entity);
		return entity;
	}
	EntityHandle CreateEntity(Transform& transformCopy)
	{
		ecs::EntityHandle entity{ CreateEntity_NoBroadcast(transformCopy) };
		BroadcastEntityCreated(entity);
		return entity;
	}

	EntityHandle CloneEntity(EntityHandle entity, bool recursive)
	{
		EntityHandle entityClone{ CreateEntity_NoBroadcast(entity->GetTransform()) };
		reinterpret_cast<internal::InternalEntityHandle>(entity)->INTERNAL_CloneCompsToEntity(
			reinterpret_cast<internal::InternalEntityHandle>(entityClone));

		if (recursive)
			for (Transform* childTransform : entity->GetTransform().GetChildren())
				CloneEntity(childTransform->GetEntity(), recursive)->GetTransform().SetParent(entityClone->GetTransform());

		BroadcastEntityCreated(entityClone);
		return entityClone;
	}
	EntityHandle CloneEntityNow(EntityHandle entity, bool recursive)
	{
		EntityHandle entityClone{ CreateEntity_NoBroadcast(entity->GetTransform()) };
		reinterpret_cast<internal::InternalEntityHandle>(entity)->INTERNAL_CloneCompsToEntityNow(
			reinterpret_cast<internal::InternalEntityHandle>(entityClone));

		// Flush here because the CompArr vector could reallocate after cloning children.
		internal::CurrentPool::ChangesBuffer().FlushComponentCallbacks();

		if (recursive)
			for (Transform* childTransform : entity->GetTransform().GetChildren())
				CloneEntityNow(childTransform->GetEntity(), recursive)->GetTransform().SetParent(entityClone->GetTransform());

		BroadcastEntityCreated(entityClone);
		return entityClone;
	}

	void DeleteEntity(EntityHandle entity, bool recursive)
	{
		internal::InternalEntityHandle internalEntity{ reinterpret_cast<internal::InternalEntityHandle>(entity) };
		if (internalEntity->INTERNAL_GetIsMarkedForDeletion())
			return;

		internal::CurrentPool::ChangesBuffer().DeleteEntity(internalEntity);

		if (recursive)
		{
			const auto& children{ entity->GetTransform().GetChildren() };
			for (Transform* childTransform : children)
				DeleteEntity(childTransform->GetEntity(), recursive);
		}
	}

	void DeleteEntityNow(EntityHandle entity, bool recursive)
	{
		// TODO: Make a flow to delete entities immediately without buffering
		DeleteEntity(entity, recursive);
		FlushChanges();
	}

	void FlushChanges()
	{
		internal::CurrentPool::ChangesBuffer().FlushChanges();
	}

	EntityHandle GetEntity(void* component)
	{
		return reinterpret_cast<EntityHandle>(internal::GetEntityFromCompAddr(component));
	}
	ConstEntityHandle GetEntity(const void* component)
	{
		return reinterpret_cast<ConstEntityHandle>(internal::GetEntityFromCompAddr(component));
	}

	EntityHandle GetEntity(ecs::EntityHash hash)
	{
		return reinterpret_cast<EntityHandle>(internal::CurrentPool::Entities().GetEntity(hash));
	}

	Transform& GetEntityTransform(void* component)
	{
		return GetEntity(component)->GetTransform();
	}
	const Transform& GetEntityTransform(const void* component)
	{
		return GetEntity(component)->GetTransform();
	}

	bool IsEntityHandleValid(EntityHandle entity)
	{
		return entity && internal::CurrentPool::Entities().CheckValidHandle(reinterpret_cast<internal::InternalEntityHandle>(entity));
	}

	EntityIterator GetEntitiesBegin()
	{
		return internal::CurrentPool::Entity_User_Begin<EntityHandle>();
	}

	EntityIterator GetEntitiesEnd()
	{
		return internal::CurrentPool::Entity_User_End<EntityHandle>();
	}

#pragma endregion // Entities

#pragma region Components

	bool GetCompActive(const void* component)
	{
		return internal::GetCompArrFromCompAddr(component)->GetIsCompActive(component);
	}

	void SetCompActive(void* component, bool isActive)
	{
		internal::CurrentPool::ChangesBuffer().ChangeCompActiveness(
			reinterpret_cast<internal::InternalEntityHandle>(GetEntity(component)),
			internal::GetCompArrFromCompAddr(component)->GetCompHash(),
			!isActive
		);
	}

	const CompTypeMeta* GetCompMeta(CompHash compHash)
	{
		return internal::CurrentPool::TypeMeta().GetCompTypeMeta(compHash);
	}

#pragma endregion // Components
	
#pragma region Systems

	std::vector<SysHandle<internal::System_Internal_Base>> GetAllSystems()
	{
		return internal::CurrentPool::Systems().GetAllSystems();
	}

	void RunSystems(ECS_LAYER layer)
	{
		internal::CurrentPool::Systems().RunSystems(static_cast<int>(layer));
	}
	
	void RemoveSystemsInLayer(ECS_LAYER layer)
	{
		internal::CurrentPool::Systems().RemoveSystemsInLayer(static_cast<int>(layer));
	}

	void RemoveAllSystems()
	{
		internal::CurrentPool::Systems().RemoveAllSystems();
	}

	const SysTypeMeta* GetSysMeta(SysHash sysHash)
	{
		return internal::CurrentPool::TypeMeta().GetSysTypeMeta(sysHash);
	}

	SysTypeMetaIterator GetSysMetaBegin()
	{
		return internal::CurrentPool::TypeMeta().GetAllSysTypeMeta().begin();
	}

	SysTypeMetaIterator GetSysMetaEnd()
	{
		return internal::CurrentPool::TypeMeta().GetAllSysTypeMeta().end();
	}

#pragma endregion // Systems

#pragma region Pools

	void SwitchToPool(POOL id)
	{
		if (id == GetCurrentPoolId())
			return;

		internal::CurrentPool::SwitchToPool_CreateIfNotExist(static_cast<int>(id), compCallbacksEnabledForPool[static_cast<int>(id)]);

		Messaging::BroadcastAll("OnECSPoolSwitched", id);
	}

	POOL GetCurrentPoolId()
	{
		return static_cast<POOL>(internal::CurrentPool::GetId());
	}

	void DeletePool(POOL id)
	{
		Messaging::BroadcastAll("OnECSPoolDeletion", id);

		// Disallow negative id and deleting the default pool
		if (static_cast<int>(id) <= 0)
			return;
		internal::CurrentPool::DeletePool(static_cast<int>(id));
	}

	EntityHandle CloneEntityToPoolNow(EntityHandle entity, POOL id, bool recursive)
	{
		// Create a new entity in the destination pool
		POOL currID{ GetCurrentPoolId() };
		SwitchToPool(id);
		EntityHandle entityClone{ CreateEntity_NoBroadcast(nullptr) };

		// Copy the world transform
		entityClone->GetTransform().SetWorld(entity->GetTransform());

		// Clone components
		reinterpret_cast<internal::InternalEntityHandle>(entity)->INTERNAL_CloneCompsToEntityNow(
			reinterpret_cast<internal::InternalEntityHandle>(entityClone),
			internal::CurrentPool::Comps(static_cast<int>(currID)),
			internal::CurrentPool::Comps() // current pool's component array
		);

		// Clone child entities
		if (recursive)
		{
			SwitchToPool(currID);
			for (Transform* childTransform : entity->GetTransform().GetChildren())
				CloneEntityToPoolNow(childTransform->GetEntity(), id, recursive)->GetTransform().SetParent(entityClone->GetTransform());
			SwitchToPool(id);
		}

		internal::CurrentPool::ChangesBuffer().FlushComponentCallbacks();
		BroadcastEntityCreated(entityClone);
		SwitchToPool(currID);
		return entityClone;
	}

#pragma endregion // Pools
	
}
