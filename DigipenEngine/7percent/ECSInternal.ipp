/******************************************************************************/
/*!
\file   ECSInternal.ipp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements template classes and functions declared
  in the internal interface header file for the Entity Component System (ECS).

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECSInternal.h"
#include <cassert>
#include <iostream>
#include "TypeID.h"
#include "Utilities.h"

namespace ecs {

	namespace internal {

#pragma region Entity_Internal

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::EntityComps_IteratorBlueprint(
			const MapIterType& iter, const MapIterType& endIter)
			: iter{ iter }
			, endIter{ endIter }
		{
			// If this component is not attached to this entity, increment until we find one that is.
			if (iter != endIter && iter->second & Entity_Internal::COMP_STATUS_ANY)
				TravelNext(true);
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		CompHash Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::GetCompHash()
		{
			return iter->first;
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		template<typename CompType>
		CompHandleType<CompType> Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::GetComp()
		{
			return reinterpret_cast<CompHandleType<CompType>>(GetCompArr(iter->first).GetComp(iter->second));
		}
		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		template<typename CompType>
		CompHandleType<const CompType> Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::GetComp() const
		{
			return reinterpret_cast<CompHandleType<const CompType>>(GetCompArr(iter->first).GetComp(iter->second));
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		bool Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::GetIsActive() const
		{
			const void* compAddr{ GetComp<const void>() };
			return GetCompArrFromCompAddr(compAddr)->GetIsCompActive(compAddr);
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>&
			Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::operator++()
		{
			TravelNext(true);
			return *this;
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>
			Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::operator++(int)
		{
			EntityComps_IteratorBlueprint copy{ *this };
			TravelNext(true);
			return *this;
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>&
			Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::operator--()
		{
			TravelNext(false);
			return *this;
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>
			Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::operator--(int)
		{
			EntityComps_IteratorBlueprint copy{ *this };
			TravelNext(false);
			return *this;
		}

		template<template<typename> typename CompHandleType_T, typename MapIterType_T, typename ValueType_T>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T> operator+(
			const Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& iter, int offset)
		{
			Entity_Internal::EntityComps_IteratorBlueprint copy{ iter };
			while (offset)
			{
				if (offset)
				{
					copy.TravelNext(true);
					--offset;
				}
				else
				{
					copy.TravelNext(false);
					++offset;
				}
			}
			return copy;
		}

		template<template<typename> typename CompHandleType_T, typename MapIterType_T, typename ValueType_T>
		bool operator==(const Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& a,
			const Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& b)
		{
			return a.iter == b.iter;
		}

		template<template<typename> typename CompHandleType_T, typename MapIterType_T, typename ValueType_T>
		bool operator!=(const Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& a,
			const Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& b)
		{
			return a.iter != b.iter;
		}

		template<template<typename> typename CompHandleType, typename MapIterType, typename ValueType>
		void Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, MapIterType, ValueType>::TravelNext(bool isIncrement)
		{
			// Increment/Decrement until we encounter an alive component
			do
			{
				if (isIncrement)
					++iter;
				else
					--iter;
			} while (iter != endIter && iter->second & Entity_Internal::COMP_STATUS_ANY);
		}

		template<template<typename> typename CompHandleType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::iterator> ecs::internal::Entity_Internal::Comps_User_Begin()
		{
			return EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::iterator>{ components.begin(), components.end() };
		}
		template<template<typename> typename CompHandleType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::const_iterator> Entity_Internal::Comps_User_Begin() const
		{
			return EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::const_iterator>{ components.begin(), components.end() };
		}

		template<template<typename> typename CompHandleType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::iterator> Entity_Internal::Comps_User_End()
		{
			return EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::iterator>{ components.end(), components.end() };
		}
		template<template<typename> typename CompHandleType>
		Entity_Internal::EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::const_iterator> Entity_Internal::Comps_User_End() const
		{
			return EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::const_iterator>{ components.end(), components.end() };
		}

#pragma endregion

#pragma region CompChangesBuffer

		template<typename T>
		uint32_t CompChangesBuffer::AddComp(InternalEntityHandle entity, T&& comp)
		{
			// TODO: Account for inactive.
			return GetCompArr<T, false>(compsToAdd).AddComp(entity, std::forward<T>(comp), true);
		}

#pragma endregion // CompChangesBuffer

#pragma region CompArr

		template<typename T>
		uint32_t CompArr::AddComp(InternalEntityHandle entKey, T&& comp, bool isInactive)
		{
			// 01/31/2025 Kendrick
			// Use the RawData* version to reduce code duplication.
			return AddComp(entKey, reinterpret_cast<RawData*>(&comp), isInactive);
		}

		/* INTERATOR SUPPORT */

		template <typename CompType, typename EntityHandleType, typename ValueType>
		CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::iterator_blueprint(
			uint32_t compStepSize, pointer ptrToCompBaseLoc)
			: ptr{ ptrToCompBaseLoc }
			, compStepSize{ compStepSize }
		{
		}

		template<typename CompType, typename EntityHandleType, typename ValueType>
		CompType& CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::operator*() const
		{
			return *reinterpret_cast<InternalCompHandle<CompType>>(ptr + CompArr::EntPtrSize);
		}

		template<typename CompType, typename EntityHandleType, typename ValueType>
		CompType* CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::operator->() const
		{
			return reinterpret_cast<InternalCompHandle<CompType>>(ptr + CompArr::EntPtrSize);
		}

		template <typename CompType, typename EntityHandleType, typename ValueType>
		InternalCompHandle<CompType> CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::GetComp()
		{
			return reinterpret_cast<InternalCompHandle<CompType>>(ptr + CompArr::EntPtrSize);
		}

		template <typename CompType, typename EntityHandleType, typename ValueType>
		EntityHandleType CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::GetEntity()
		{
			return *reinterpret_cast<EntityHandleType*>(ptr);
		}

		template<typename CompType, typename EntityHandleType, typename ValueType>
		bool CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::GetIsActive() const
		{
			return (*reinterpret_cast<CompArr**>(ptr - CompArr::CompArrPtrSize))->GetIsCompActive(GetComp());
		}

		template <typename CompType, typename EntityHandleType, typename ValueType>
		CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::operator++()
		{
			ptr += compStepSize;
			return *this;
		}
		template <typename CompType, typename EntityHandleType, typename ValueType>
		CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType> CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::operator++(int)
		{
			iterator_blueprint<CompType, EntityHandleType, ValueType> copy{ *this };
			++*this;
			return copy;
		}

		template <typename CompType, typename EntityHandleType, typename ValueType>
		CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::operator--()
		{
			ptr -= compStepSize;
			return *this;
		}
		template <typename CompType, typename EntityHandleType, typename ValueType>
		CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType> CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>::operator--(int)
		{
			iterator_blueprint<CompType, EntityHandleType, ValueType> copy{ *this };
			--*this;
			return copy;
		}

		template <typename CompType, typename EntityHandleType, typename ValueType>
		CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType> operator+(
			const CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& iter, int offset)
		{
			return CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>{ iter.compStepSize, iter.ptr + offset * iter.compStepSize };
		}

		template <typename CompType, typename EntityHandleType, typename ValueType>
		bool operator==(
			const CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& a,
			const CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& b)
		{
			return a.ptr == b.ptr;
		}
		template <typename CompType, typename EntityHandleType, typename ValueType>
		bool operator!=(
			const CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& a,
			const CompArr::iterator_blueprint<CompType, EntityHandleType, ValueType>& b)
		{
			return a.ptr != b.ptr;
		}


		template<typename CompType, typename EntityHandleType>
		CompArr::iterator_blueprint<CompType, EntityHandleType> CompArr::User_Begin()
		{
			return iterator_blueprint<CompType, EntityHandleType>{ compStepSize, arrRaw.data() + CompArrPtrSize };
		}
		template<typename CompType, typename EntityHandleType>
		CompArr::iterator_blueprint<CompType, EntityHandleType> CompArr::User_Begin_Active()
		{
			return iterator_blueprint<CompType, EntityHandleType>{ compStepSize, arrRaw.data() + firstActiveIndex * compStepSize + CompArrPtrSize };
		}
		template<typename CompType, typename EntityHandleType>
		CompArr::iterator_blueprint<CompType, EntityHandleType> CompArr::User_End()
		{
			return iterator_blueprint<CompType, EntityHandleType>{ compStepSize, arrRaw.data() + arrRaw.size() + CompArrPtrSize };
		}
		template<typename CompType, typename EntityHandleType>
		CompArr::iterator_blueprint<CompType, EntityHandleType> CompArr::User_Custom(InternalEntityHandle entityHandle)
		{
			return iterator_blueprint<CompType, EntityHandleType>{ compStepSize, reinterpret_cast<RawData*>(entityHandle) };
		}

#pragma endregion // CompArr

#pragma region System_Internal

		template<typename SysType, typename ...Args>
		template <typename ReturnType, typename ClassType>
		System_Internal<SysType, Args...>::System_Internal(ReturnType(ClassType::*memberFunc)(Args&...))
			: callProcessEntity{ [memberFunc](System_Internal* objPtr, Args&... args) -> ReturnType { return (reinterpret_cast<ClassType*>(objPtr)->*memberFunc)(args...); } }
		{
		}

		template<typename SysType, typename ...Args>
		bool System_Internal<SysType, Args...>::RegisterSystem()
		{
			CurrentPool::TypeMeta().RegisterSysType<SysType>();
			return true;
		}

		template<typename SysType, typename ...Args>
		void System_Internal<SysType, Args...>::Run()
		{
			// If this system runs on no components, we don't execute over any components and only let PostRun() execute on this system.
			if constexpr (sizeof...(Args) == 0)
				return;
			else
				// Execute on the components within the CompArr that has the least components, out of the components we want.
				RunOnCompArr(*GetCompArrWithLeastComps<Args...>());
		}

		template<typename SysType, typename ...Args>
		template <typename Predicate>
			requires std::is_same_v<Predicate, std::nullptr_t> || std::predicate<Predicate, internal::InternalEntityHandle>
		void System_Internal<SysType, Args...>::RunOnCompArr(CompArr& compArr, [[maybe_unused]] Predicate pred)
		{
			// TODO: Probably should revisit this code to clean up and remove the double map lookup of entities' components' indexes
			
			// For each entity with this component,
			for (CompArr::iterator compIter{ compArr.begin_active() }, endIter{ compArr.end() }; compIter != endIter; ++compIter)
			{
				// Check if this entity has the required components
				if constexpr (sizeof...(Args) > 1) // Trivial case of only 1 component needed -- no need to check existance of the component that's in the CompArr we're iterating
					if (!((compIter.GetEntity()->INTERNAL_GetHasComp(GetCompHash<Args>())) && ...))
						continue;

				// Check for any extra requirements for which entities are processed
				if constexpr (!std::is_same_v<Predicate, std::nullptr_t>)
					if (!pred(compIter.GetEntity()))
						continue;

				// Get each required component from the entity and pass that to the function ptr
				if constexpr (sizeof...(Args) <= 1) // With 1 or less comps, we can skip checking for and getting other comps
					callProcessEntity(this, *reinterpret_cast<InternalCompHandle<Args>>(compIter.GetComp())...);
				else // With 2 or more comps, we'll need to get the other comps with this GetComponent() function
					callProcessEntity(this, *reinterpret_cast<InternalCompHandle<Args>>(GetComponent(
						compIter.GetEntity(), compArr.GetCompHash(), compIter.GetComp(), GetCompHash<Args>()
					))...);
			}
		}

		template<typename SysType, typename ...Args>
		InternalCompHandle<RawData> System_Internal<SysType, Args...>::GetComponent(InternalEntityHandle entity, CompHash obtainedCompHash, InternalCompHandle<RawData> obtainedComp, CompHash desiredCompHash)
		{
			if (obtainedCompHash == desiredCompHash)
				return obtainedComp;

			return entity->INTERNAL_GetCompRaw(desiredCompHash);
		}

#pragma endregion // System_Internal

#pragma region SystemsManager

		template<typename SysType>
		InternalGenericSysHandle<SysType> SystemsManager::AddSystem(int layer, SysType&& system)
		{
			// Check if this system already exists
			if (hashToLayerMap.find(GetSysHash<SysType>()) != hashToLayerMap.end())
				return nullptr;

			// Add the system
			hashToLayerMap.emplace(GetSysHash<SysType>(), layer);
			InternalGenericSysHandle<SysType> systemHandle{ reinterpret_cast<InternalGenericSysHandle<SysType>>(
				GetSystemsMap(layer).emplace(GetSysHash<SysType>(), new std::remove_reference_t<SysType>{ std::forward<SysType>(system) }).first->second) };
			systemHandle->OnAdded();
			return systemHandle;
		}

		template<typename SysType>
		InternalGenericSysHandle<SysType> SystemsManager::GetSystem()
		{
			// Check if the system exists
			SysHashToLayerMapType::const_iterator layerIter{ hashToLayerMap.find(GetSysHash<SysType>()) };
			if (layerIter == hashToLayerMap.end())
				return nullptr;

			// Retrieve the system
			return reinterpret_cast<InternalGenericSysHandle<SysType>>(GetSystemsMap(layerIter->second).at(GetSysHash<SysType>()));
		}

		template<typename SysType>
		bool SystemsManager::RemoveSystem()
		{
			// Do nothing if this system isn't registered
			SysHashToLayerMapType::iterator layerIter{ hashToLayerMap.find(GetSysHash<SysType>()) };
			if (layerIter == hashToLayerMap.end())
				return false;

			// Call the system's OnRemoved()
			SysMapType& sysMap{ GetSystemsMap(layerIter->second) };
			System_Internal_Base* system{ sysMap.at(GetSysHash<SysType>()) };
			system->OnRemoved();

			// Remove the system
			delete sysMap.at(GetSysHash<SysType>());
			sysMap.erase(GetSysHash<SysType>());
			hashToLayerMap.erase(layerIter);
			return true;
		}

#pragma endregion // SystemsManager

#pragma region Type Meta

		template<typename CompType>
		void TypeMetaManager::RegisterCompType()
		{
			if (compTypeMetaMap.find(GetCompHash<CompType>()) != compTypeMetaMap.end())
				return;

			compTypeMetaMap.try_emplace(GetCompHash<CompType>(), CompTypeMeta{
				.hash = GetCompHash<CompType>(),
				.name = util::GetNiceTypeName<CompType>()
			});
		}

		template<typename SysType>
		void TypeMetaManager::RegisterSysType()
		{
			if (sysTypeMetaMap.find(GetSysHash<SysType>()) != sysTypeMetaMap.end())
				return;

			sysTypeMetaMap.try_emplace(GetSysHash<SysType>(), SysTypeMeta{
				.hash = GetSysHash<SysType>(),
				.name = util::GetNiceTypeName<SysType>()
			});
		}

#pragma endregion // Type Meta

#pragma region ECSPool

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::Entity_IteratorBlueprint(const EntContIterType& iterToEntity, const EntContIterType& endIter)
			: iter{ iterToEntity }
			, endIter{ endIter }
		{
			if (IsPointingToDeletedEntity())
				TravelNext(true);
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		EntityHandleType ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::operator*() const
		{
			// Cast is required if for example users want entities as "EntityHandle" but map stores entities as "InternalEntityHandle", etc.
			if constexpr (util::IsMapIterator_v<EntContIterType>)
				return reinterpret_cast<EntityHandleType>(&iter->second);
			else
				return reinterpret_cast<EntityHandleType>(*iter);
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		EntityHandleType ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::operator->() const
		{
			if constexpr (util::IsMapIterator_v<EntContIterType>)
				return reinterpret_cast<EntityHandleType>(&iter->second);
			else
				return reinterpret_cast<EntityHandleType>(*iter);
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		EntityHandleType ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::GetEntity()
		{
			if constexpr (util::IsMapIterator_v<EntContIterType>)
				return reinterpret_cast<EntityHandleType>(&iter->second);
			else
				return reinterpret_cast<EntityHandleType>(*iter);
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>&
			ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::operator++()
		{
			TravelNext(true);
			return *this;
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>
			ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::operator++(int)
		{
			Entity_IteratorBlueprint copy{ *this };
			TravelNext(true);
			return *this;
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>&
			ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::operator--()
		{
			TravelNext(false);
			return *this;
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>
			ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::operator--(int)
		{
			Entity_IteratorBlueprint copy{ *this };
			TravelNext(false);
			return *this;
		}

		template<typename EntityHandleType_T, typename EntContIterType_T, typename ValueType_T>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>
			operator+(const ECSPool::Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& iter, int offset)
		{
			ECSPool::Entity_IteratorBlueprint copy{ iter };
			while (offset)
			{
				if (offset)
				{
					copy.TravelNext(true);
					--offset;
				}
				else
				{
					copy.TravelNext(false);
					++offset;
				}
			}
			return copy;
		}

		template<typename EntityHandleType_T, typename EntContIterType_T, typename ValueType_T>
		bool operator==(const ECSPool::Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& a,
			const ECSPool::Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& b)
		{
			return a.iter == b.iter;
		}

		template<typename EntityHandleType_T, typename EntContIterType_T, typename ValueType_T>
		bool operator!=(const ECSPool::Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& a,
			const ECSPool::Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& b)
		{
			return a.iter != b.iter;
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		bool ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::IsPointingToDeletedEntity() const
		{
			if constexpr (util::IsMapIterator_v<EntContIterType>)
				return iter != endIter && iter->second.INTERNAL_GetIsMarkedForDeletion();
			else
				return iter != endIter && reinterpret_cast<InternalEntityHandle>(*iter)->INTERNAL_GetIsMarkedForDeletion();
		}

		template<typename EntityHandleType, typename EntContIterType, typename ValueType>
		void ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntContIterType, ValueType>::TravelNext(bool isIncrement)
		{
			do
			{
				if (isIncrement)
					++iter;
				else
					--iter;
			} while (IsPointingToDeletedEntity());
		}

		template <typename EntityHandleType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> ECSPool::Entity_User_Begin()
		{
			return ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator>{ entities.begin(), entities.end() };
		}

		template <typename EntityHandleType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> ECSPool::Entity_User_End()
		{
			return ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator>{ entities.end(), entities.end() };
		}

		template<typename EntityHandleType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> CurrentPool::Entity_User_Begin()
		{
			return activePool->Entity_User_Begin<EntityHandleType>();
		}

		template<typename EntityHandleType>
		ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> CurrentPool::Entity_User_End()
		{
			return activePool->Entity_User_End<EntityHandleType>();
		}

#pragma endregion // ECSPool

	}

	
#pragma region Internal Global Functions

	namespace internal {

		// Gets the hash of a component type
		template<typename T>
		constexpr CompHash GetCompHash()
		{
			return typeid(T).hash_code();
		}
		// Gets the hash of a system type
		template<typename T>
		constexpr SysHash GetSysHash()
		{
			return typeid(T).hash_code();
		}

		template<typename T>
		void ComponentCopyMethod(RawData* comp, RawData* dest)
		{
			new(dest) T{ *reinterpret_cast<T*>(comp) };
		}

		template <typename T>
		void ComponentMoveMethod(RawData* comp, RawData* dest)
		{
			new(dest) T{ std::move(*reinterpret_cast<T*>(comp)) };
		}

		template <typename T>
		void ComponentDestructorMethod(RawData* comp)
		{
			reinterpret_cast<T*>(comp)->~T();
		}

		template<typename T>
		void ComponentInformAttachedMethod(InternalEntityHandle entity)
		{
			if constexpr (std::is_base_of_v<IComponentCallbacks, T>)
				reinterpret_cast<T*>(entity->INTERNAL_GetCompRaw(GetCompHash<T>()))->OnAttached();
		}

		template<typename T>
		void ComponentInformDetachedMethod(InternalEntityHandle entity)
		{
			if constexpr (std::is_base_of_v<IComponentCallbacks, T>)
				reinterpret_cast<T*>(entity->INTERNAL_GetCompRaw(GetCompHash<T>()))->OnDetached();
		}

		// Get a component array when we know the component type
		template <typename T>
		CompArr& GetCompArr()
		{
			return GetCompArr<T>(CurrentPool::Comps());
		}
		template<typename T, bool DoComponentCallbacks>
		CompArr& GetCompArr(CompArrMapType& compArrPool)
		{
			CompHash compHash{ GetCompHash<T>() };
			auto iter = compArrPool.find(compHash);
			if (iter != compArrPool.end())
				return iter->second;

			// Let's register this type since it seems like it's the first time we've seen it
			CurrentPool::TypeMeta().RegisterCompType<T>();

			bool doCompCallbacks{ DoComponentCallbacks && CurrentPool::HasCompCallbacksEnabled() };

			return compArrPool.try_emplace(compHash, compHash, static_cast<uint32_t>(sizeof(T)),
				ComponentCopyMethod<T>, ComponentMoveMethod<T>, ComponentDestructorMethod<T>,
				doCompCallbacks ? ComponentInformAttachedMethod<T> : [](InternalEntityHandle) -> void {},
				doCompCallbacks ? ComponentInformDetachedMethod<T> : [](InternalEntityHandle) -> void {},
				ComponentInformAttachedMethod<T>,
				ComponentInformDetachedMethod<T>
				).first->second;
		}

		// Out of the given component types, get the CompArr with the least number of components
		template<typename ...Args>
		CompArr* GetCompArrWithLeastComps()
		{
			uint32_t compsCount{ (std::numeric_limits<uint32_t>::max)() };
			CompArr* arrWithMinComps{ nullptr };

			// For each CompArr, if it has fewer comps that what we're storing, overwrite our minCompArr with this one.
			(ReplaceIfFewerComps<Args>(&compsCount, &arrWithMinComps), ...);

			return arrWithMinComps;
		}

		template<typename CompType>
		void ReplaceIfFewerComps(uint32_t* compsCount, CompArr** arrToReplace)
		{
			CompArr* arr{ &GetCompArr<CompType>() };

			if (arr->GetNumComps() < *compsCount)
			{
				*compsCount = arr->GetNumComps();
				*arrToReplace = arr;
			}
		}

		template<size_t NumCompArrs>
		CompArr* GetCompArrWithLeastComps(const std::array<CompArr*, NumCompArrs>& compArrs)
		{
			if (compArrs.empty())
				return nullptr;

			CompArr* arrWithMinComps{ compArrs.front() };
			uint32_t compsCount{ arrWithMinComps->GetNumComps() };

			for (auto compArrIter{ compArrs.begin() + 1 }, endIter{ compArrs.end() }; compArrIter != endIter; ++compArrIter)
				if ((*compArrIter)->GetNumComps() < compsCount)
				{
					compsCount = (*compArrIter)->GetNumComps();
					arrWithMinComps = *compArrIter;
				}

			return arrWithMinComps;
		}

	}

#pragma endregion // Internal Global Functions

}