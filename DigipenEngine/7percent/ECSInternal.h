/******************************************************************************/
/*!
\file   ECSInternal.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the internal interface file for the Entity Component System (ECS), which
  contains classes and functions that implement ECS and that are not meant to be
  used directly by user functions.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <typeinfo>
#include <exception>
#include <functional>
#include <array>
#include <iterator>
#include <memory>

#include "Transform.h"

namespace ecs {

	// Forward declares
	namespace internal {
		class Entity_Internal;
		class IComponentCallbacks;
		class CompModifyTask;
		class CompArr;
		class System_Internal_Base;
		class SystemsManager;
		struct CompTypeMeta;
		struct SysTypeMeta;
		class TypeMetaManager;
		class ECSPool;
	}

	namespace internal {
		// The handle to a component
		template <typename T>
		using InternalCompHandle = T*;
		// The hash type of a component
		using CompHash = size_t;

		// The handle to an entity's internal data
		using InternalEntityHandle = internal::Entity_Internal*;
		// A const version of a handle to an entity's internal data
		using ConstInternalEntityHandle = const internal::Entity_Internal*;
		// The hash type of an entity hash
		using EntityHash = size_t;

		// The handle to a system
		template <typename T>
		using InternalGenericSysHandle = T*;
		// The hash type of a system
		using SysHash = size_t;

		// Type to reference anonymous data
		using RawData = uint8_t;

		// The map type storing ECSPools
		using PoolsMapType = std::map<int, internal::ECSPool>;

		// The container type storing components' raw data
		using CompContType = std::vector<RawData>;
		// The map type storing component arrays
		using CompArrMapType = std::unordered_map<CompHash, internal::CompArr>;
		// The map type storing entity hashes to entities
		using EntMapType = std::unordered_map<EntityHash, internal::Entity_Internal>;
		// The set type storing valid entity handles
		using ValidEntHandleSetType = std::unordered_set<InternalEntityHandle>;
		// The container type that entities use to store components that are attached to them.
		//   ComponentTypeHash -> Indexes in CompArrs
		using EntCompMapType = std::unordered_map<CompHash, uint32_t>;
		// The set type storing components' indexes to be removed for a particular component type
		template <typename T>
		using CompIndexSetType = std::set<T>;
		// The container type storing components' indexes to be removed
		//   ComponentTypeHash -> Set of indexes to be removed
		using ModifyCompSetType = std::unordered_map<EntityHash, CompIndexSetType<CompModifyTask>>;
		// The container type storing entities to be removed
		using RemoveEntContType = std::vector<InternalEntityHandle>;

		// The map type storing systems
		using SysMapType = std::unordered_map<SysHash, internal::System_Internal_Base*>;
		// The map type storing layer to systems
		using SysLayerMapType = std::map<int, SysMapType>;
		// The map type storing systems hashes to the layer that they are in
		using SysHashToLayerMapType = std::unordered_map<SysHash, int>;


		// The map type storing component types
		using CompTypeMetaMap = std::unordered_map<CompHash, CompTypeMeta>;
		// The map type storing system types
		using SysTypeMetaMap = std::unordered_map<SysHash, SysTypeMeta>;
	}

	namespace internal {
		// The signature of the function to call the copy constructor of a component
		using CompCopySig = void(*)(RawData* comp, RawData* dest);
		// The signature of the function to call the move constructor of a component
		using CompMoveSig = void(*)(RawData* comp, RawData* dest);
		// The signature of the function to call the destructor of a component
		using CompDestroySig = void(*)(RawData* comp);
		// The signature of the function to call when a component is attached to an entity
		// Note: These function take the entity because the location of where the component is at may become different while buffering callbacks.
		using CompInformAttachedSig = void(*)(InternalEntityHandle entity);
		// The signature of the function to call when a component is detached from an entity
		using CompInformDetachedSig = void(*)(InternalEntityHandle entity);
	}

	/* INTERNAL CLASSES */

	namespace internal {

#pragma region Entity

		/*****************************************************************//*!
		\class Entity_Internal
		\brief
			This is the base class of ecs::Entity, encapsulating the logic of how components are tracked within the entity
			and interfaces to modify these components.

			Components are tracked using a map within this class, mapping component type hashes to the index of the component
			within the respective compArrs.
			Transforms of entities are also stored within this class.
		*//******************************************************************/
		class Entity_Internal
		{
		public:
			/*****************************************************************//*!
			\brief
				Constructs a standalone empty entity.
			\param mapKey
				The UID of this entity.
			*//******************************************************************/
			Entity_Internal(EntityHash mapKey);

			/*****************************************************************//*!
			\brief
				Constructs an empty entity parented to the specified entity.
			\param mapKey
				The UID of this entity.
			\param parentEntity
				The entity to parent this constructed entity to.
			*//******************************************************************/
			Entity_Internal(EntityHash mapKey, InternalEntityHandle parentEntity);

			/*****************************************************************//*!
			\brief
				Constructs an empty entity, copying the specified transform.
				If the transform is parented to another entity, this constructed entity will also be parented to that entity.
				The transform's children are ignored in this construction.
			\param mapKey
				The UID of this entity.
			\param transformCopy
				The transform to copy the values of.
			*//******************************************************************/
			Entity_Internal(EntityHash mapKey, Transform& transformCopy);

			// Disallow simple copying of entities.
			Entity_Internal(const Entity_Internal&) = delete;

			/* INTERNAL */
		public:
			/*****************************************************************//*!
			\brief
				Copies components that are fully attached to this entity to another entity.
				These components are buffered for addition instead of attaching immediately. For immediate changes, use INTERNAL_CloneCompsToEntityNow().
			\param entity
				The entity to copy components to.
			*//******************************************************************/
			void INTERNAL_CloneCompsToEntity(InternalEntityHandle entity) const;

			/*****************************************************************//*!
			\brief
				Copies components that are fully attached to this entity to another entity.
				These components are attached immediately.
			\param entity
				The entity to copy components to.
			*//******************************************************************/
			void INTERNAL_CloneCompsToEntityNow(InternalEntityHandle entity) const;
			/*****************************************************************//*!
			\brief
				Copies components that are fully attached to this entity to another entity whose components are stored in the
				provided component array map. This is to facilitate copying entities between pools.
				These components are attached immediately.
			\param entity
				The entity to copy components to.
			\param srcCompArrMap
				The map of component arrays that this entity stores its components in.
			\param destCompArrMap
				The map of component arrays that the provided entity stores its components in.
			*//******************************************************************/
			void INTERNAL_CloneCompsToEntityNow(InternalEntityHandle entity, CompArrMapType& srcCompArrMap, CompArrMapType& destCompArrMap) const;

			/*****************************************************************//*!
			\brief
				Change the index of a component that is registered to this entity.
				The last 2 bits of the index is used to track whether the component is pending addition or removal.
			\param compHash
				The component type hash.
			\param newIndex
				The index of the component within the compArr, excluding the last 2 bits which are used to track the component's status.
			\param overrideFlags
				If true, sets the component status to the last 2 bits of newIndex. Otherwise, the last 2 bits are kept unmodified no matter the bit states in newIndex.
			*//******************************************************************/
			void INTERNAL_ChangeCompIndex(CompHash compHash, uint32_t newIndex, bool overrideFlags = false);

			/*****************************************************************//*!
			\brief
				Unregisters a component from this entity.
			\param compHash
				The component type hash.
			*//******************************************************************/
			void INTERNAL_RemoveComp(CompHash compHash);

			/*****************************************************************//*!
			\brief
				Returns a const iterator to the first component entry in the components map of this entity.
			\return
				A const iterator to the first component entry in the components map of this entity.
			*//******************************************************************/
			EntCompMapType::const_iterator INTERNAL_CompsBegin() const;

			/*****************************************************************//*!
			\brief
				Returns a const iterator to 1 past the last component entry in the components map of this entity.
			\return
				A const iterator to 1 past the last component entry in the components map of this entity.
			*//******************************************************************/
			EntCompMapType::const_iterator INTERNAL_CompsEnd() const;

			/*****************************************************************//*!
			\brief
				Marks this entity for deletion. All components in this entity are also marked for removal.
			*//******************************************************************/
			void INTERNAL_MarkForDeletion();

			/*****************************************************************//*!
			\brief
				Checks if this entity is marked for deletion.
			\return
				True if this entity is marked for deletion. False otherwise.
			*//******************************************************************/
			bool INTERNAL_GetIsMarkedForDeletion() const;

			/*****************************************************************//*!
			\brief
				For shutdown purposes: Marks all components of this entity as pending removal.
			*//******************************************************************/
			void INTERNAL_MarkAllCompsRemoved();

			/*****************************************************************//*!
			\brief
				Gets this entity's UID, which can be used as the key to this entity in ECSPool::entities.
			\return
				This entity's UID.
			*//******************************************************************/
			EntityHash INTERNAL_GetMapKey() const;

			/*****************************************************************//*!
			\brief
				Checks if this entity has a component of the specified component type hash, no matter if it has a status or not.
			\param compHash
				The component type.
			\return
				True if this entity has the component registered within the components map. False otherwise.
			*//******************************************************************/
			bool INTERNAL_GetHasComp(CompHash compHash) const;

			/*****************************************************************//*!
			\brief
				Gets the index of a component attached to this entity.
				If the component doesn't exist, or has a status, returns max value of uint32_t.
			\param compHash
				The component type.
			\return
				A pointer to the component as RawData. If the component doesn't exist, or has a status (not fully attached), returns nullptr.
			*//******************************************************************/
			uint32_t INTERNAL_GetCompIndex(CompHash compHash) const;

			/*****************************************************************//*!
			\brief
				Gets the component with the specified component type hash that is fully attached to this entity.
				If the component doesn't exist, or has a status, returns nullptr.
			\param compHash
				The component type.
			\return
				A pointer to the component as RawData. If the component doesn't exist, or has a status (not fully attached), returns nullptr.
			*//******************************************************************/
			RawData* INTERNAL_GetCompRaw(CompHash compHash) const;

			/*****************************************************************//*!
			\brief
				Gets the byte offset of the member variable transform.
				This exists so class Transform can implement GetEntity().
				// Note: Implemented here since constexpr requires this to be inlined.
			\return
				The bytes offset of member variable transform.
			*//******************************************************************/
			static constexpr size_t INTERNAL_GetTransformVarByteOffset()
			{
				return offsetof(Entity_Internal, transform);
			}

		protected:
			/*****************************************************************//*!
			\brief
				Checks if a component of the specified component type hash can be attached to this entity.
				Returns false if a component of the same type is already attached, or is pending addition or removal to/from this entity.
			\param compHash
				The component type.
			\return
				True if a component of the specified component type hash can be attached to this entity. False otherwise.
			*//******************************************************************/
			bool CheckCanAddComp(CompHash compHash);
			// Check if a component type is available for deletion
			/*****************************************************************//*!
			\brief
				Checks if a component of the specified component type hash can be removed to this entity.
				Returns false if a component of the same type does not exist on this entity, or is pending removal from this entity.
			\param compHash
				The component type.
			\param outCompIndexIter
				If this entity is not marked for deletion, an iterator to the component entry within this entity with the specified component type hash will be written to this variable.
				This variable will equal the end of the component map if a component with the specified component type hash does not exist on this entity.
			\return
				True if a component of the specified component type hash can be removed to this entity. False otherwise.
			*//******************************************************************/
			bool CheckCanRemoveComp(CompHash compHash, EntCompMapType::iterator& outCompIndexIter);

		public:
			//! Bitflag to test if a component is pending addition.
			static constexpr uint32_t COMP_STATUS_TO_ADD{ 0x80000000 }; // Last bit of 32bits
			//! Bitflag to test if a component is pending removal.
			static constexpr uint32_t COMP_STATUS_TO_REMOVE{ 0x40000000 }; // Second last bit of 32bits
			//! Bitmask to test if a component has any status.
			static constexpr uint32_t COMP_STATUS_ANY{ 0xC0000000 }; // Last 2 bits of 32bits
			//! Bitmask for obtaining the index of a component in the compArr.
			static constexpr uint32_t COMP_STATUS_UNUSED_BITS{ ~COMP_STATUS_ANY }; // All bits unused in storing status flags

		protected:
			//! This entity's UID.
			EntityHash mapKey;

			//! Map of component type hashes to the index of the component within the respective compArr that's storing the components' data.
			//! The last 2 bits of the index is reserved for tracking the component's status, with the remaining bits used for tracking the index of the component within the respective compArrs.
			EntCompMapType components;

			//! Whether this entity is marked for deletion
			bool isMarkedForDeletion;

			//! The transform of this entity.
			Transform transform;


			/* ITERATOR SUPPORT */
		public:
			/*****************************************************************//*!
			\struct EntityComps_IteratorBlueprint
			\brief
				This is the implementation for an iterator that iterates all fully attached components within this entity.
			\tparam CompHandleType
				The type of the handle to components that is used in this iterator. This is exposed to optionally allow const implementations.
			\tparam MapIterType
				The type of the iterator to the map that holds component type hashes to index within compArr. This is exposed to optionally allow const implementations.
			\tparam ValueType
				The type of the data that this iterator iterates.
			*//******************************************************************/
			template <template<typename> typename CompHandleType, typename MapIterType, typename ValueType = void*>
			struct EntityComps_IteratorBlueprint
			{
				// note: these types may not be correct... further research needed if we want to support <algorithm> library
				using iterator_category = std::bidirectional_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = ValueType;
				using pointer = ValueType*;
				using reference = ValueType&;

				/*****************************************************************//*!
				\brief
					Constructs an iterator to the component index map with the provided iterators to the map.
				\param iter
					The component index that this iterator will point to. If the component has a status,
					the iterator will increment until it encounters a fully attached component or reaches the end of the map.
				\param endIter
					An iterator to 1 past the end of the component index map. This is required for this iterator to
					know when it can't increment anymore while searching for a fully attached component.
				*//******************************************************************/
				EntityComps_IteratorBlueprint(const MapIterType& iter, const MapIterType& endIter);

				/*****************************************************************//*!
				\brief
					Gets the component type hash of the component that this iterator is pointing to.
				\return
					The component type hash of the component that this iterator is pointing to.
				*//******************************************************************/
				CompHash GetCompHash();

				/*****************************************************************//*!
				\brief
					Accesses the component itself that this iterator is pointing to.
				\tparam CompType
					The type to interpret the component as.
				\return
					A handle to the component.
				*//******************************************************************/
				// Access the component
				template <typename CompType>
				CompHandleType<CompType> GetComp();

				/*****************************************************************//*!
				\brief
					Accesses the component itself that this iterator is pointing to.
				\tparam CompType
					The type to interpret the component as.
				\return
					A handle to the component.
				*//******************************************************************/
				// Access the component
				template <typename CompType>
				CompHandleType<const CompType> GetComp() const;
				
				/*****************************************************************//*!
				\brief
					Gets whether the component pointed to by this iterator is active or inactive.
				\return
					True if the component is active. False otherwise.
				*//******************************************************************/
				bool GetIsActive() const;

				/*****************************************************************//*!
				\brief
					Prefix increments this iterator to the next fully attached component within the map.
				\return
					A reference to this iterator.
				*//******************************************************************/
				EntityComps_IteratorBlueprint& operator++();

				/*****************************************************************//*!
				\brief
					Postfix increments this iterator to the next fully attached component within the map.
				\return
					A temporary of the original state of this iterator.
				*//******************************************************************/
				EntityComps_IteratorBlueprint operator++(int);

				/*****************************************************************//*!
				\brief
					Prefix decrements this iterator to the previous fully attached component within the map.
				\return
					A reference to this iterator.
				*//******************************************************************/
				EntityComps_IteratorBlueprint& operator--();

				/*****************************************************************//*!
				\brief
					Postfix decrements this iterator to the previous fully attached component within the map.
				\return
					A temporary of the original state of this iterator.
				*//******************************************************************/
				EntityComps_IteratorBlueprint operator--(int);

				/*****************************************************************//*!
				\brief
					Increments or decrements an iterator by the specified amount.
				\tparam CompHandleType_T
					The iterator's CompHandleType template parameter type.
				\tparam MapIterType_T
					The iterator's MapIterType template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param iter
					The iterator to increment/decrement.
				\param offset
					The number of times to increment/decrement the iterator by.
				\return
					A temporary of this iterator incremented/decremented by the specified number of times.
				*//******************************************************************/
				template <template<typename> typename CompHandleType_T, typename MapIterType_T, typename ValueType_T>
				friend EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T> operator+(
					const EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& iter, int offset);

				/*****************************************************************//*!
				\brief
					Tests if 2 iterators point to the same component index within this entity.
				\tparam CompHandleType_T
					The iterator's CompHandleType template parameter type.
				\tparam MapIterType_T
					The iterator's MapIterType template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param a
					Left-hand side iterator.
				\param b
					Right-hand side iterator.
				\return
					True if both iterators point to the same component index within this entity. False otherwise.
				*//******************************************************************/
				template <template<typename> typename CompHandleType_T, typename MapIterType_T, typename ValueType_T>
				friend bool operator==(
					const EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& a,
					const EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& b);

				/*****************************************************************//*!
				\brief
					Tests if 2 iterators do not point to the same component index within this entity
				\tparam CompHandleType_T
					The iterator's CompHandleType template parameter type.
				\tparam MapIterType_T
					The iterator's MapIterType template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param a
					Left-hand side iterator.
				\param b
					Right-hand side iterator.
				\return
					True if both iterators do not point to the same component index within this entity. False otherwise.
				*//******************************************************************/
				template <template<typename> typename CompHandleType_T, typename MapIterType_T, typename ValueType_T>
				friend bool operator!=(
					const EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& a,
					const EntityComps_IteratorBlueprint<CompHandleType_T, MapIterType_T, ValueType_T>& b);

			private:
				/*****************************************************************//*!
				\brief
					Continually increments or decrements this iterator until a fully attached component is found,
					or the end of the map is reached.
				\param isIncrement
					If true, increments this iterator. Otherwise, decrements this iterator.
				*//******************************************************************/
				void TravelNext(bool isIncrement);

			private:
				//! The component index map iterator pointing to the current component index.
				MapIterType iter;
				//! The component index map iterator pointing to 1 past the end.
				MapIterType endIter;
			};

		public:
			/*****************************************************************//*!
			\brief
				Creates an iterator to the first component that is fully attached to this entity with a simplified template parameter options list.
			\tparam CompHandleType
				The iterator's CompHandleType template parameter type.
			\return
				A component iterator to the first fully attached component in this entity.
			*//******************************************************************/
			template<template<typename> typename CompHandleType>
			EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::iterator> Comps_User_Begin();

			/*****************************************************************//*!
			\brief
				Creates a const iterator to the first component that is fully attached to this entity with a simplified template parameter options list.
			\tparam CompHandleType
				The iterator's CompHandleType template parameter type.
			\return
				A const component iterator to the first fully attached component in this entity.
			*//******************************************************************/
			template<template<typename> typename CompHandleType>
			EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::const_iterator> Comps_User_Begin() const;

			/*****************************************************************//*!
			\brief
				Creates an iterator to 1 past the last component that is fully attached to this entity with a simplified template parameter options list.
			\tparam CompHandleType
				The iterator's CompHandleType template parameter type.
			\return
				A component iterator to 1 past the last fully attached component in this entity.
			*//******************************************************************/
			template<template<typename> typename CompHandleType>
			EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::iterator> Comps_User_End();

			/*****************************************************************//*!
			\brief
				Creates a const iterator to 1 past the last component that is fully attached to this entity with a simplified template parameter options list.
			\tparam CompHandleType
				The iterator's CompHandleType template parameter type.
			\return
				A const component iterator to 1 past the last fully attached component in this entity.
			*//******************************************************************/
			template<template<typename> typename CompHandleType>
			EntityComps_IteratorBlueprint<CompHandleType, EntCompMapType::const_iterator> Comps_User_End() const;

		};

#pragma endregion // Entity

#pragma region Components

		/*****************************************************************//*!
		\class IComponentCallbacks
		\brief
			Provides an interface for components to be notified of certain ECS events.
		*//******************************************************************/
		class IComponentCallbacks
		{
		public:
			/*****************************************************************//*!
			\brief
				Called when the component is fully attached to an entity.
			*//******************************************************************/
			virtual void OnAttached() {};

			/*****************************************************************//*!
			\brief
				Called when the component is about to be detached from an entity.
			*//******************************************************************/
			virtual void OnDetached() {};
		};

		/*****************************************************************//*!
		\class CompModifyTask
		\brief
			Stores information about a component that is pending moving within the CompArr
			from active to inactive, or removal from the CompArr.

			Instead of storing component indexes, we store the entities that have the
			component that needs to be modified, because the CompArr could change component
			indexes while we're iterating through the modify tasks.
		*//******************************************************************/
		class CompModifyTask
		{
		public:
			/*****************************************************************//*!
			\enum class TYPE
				The type of a task.
			*//******************************************************************/
			enum class TYPE
			{
				REMOVE,			// Remove the component
				SET_ACTIVE,		// Set the component to be active
				SET_INACTIVE	// Set the component to be inactive
			};

			CompModifyTask(InternalEntityHandle entity, TYPE type);

			/*****************************************************************//*!
			\brief
				Gets the index of the component tracked by this task.
			\param compHash
				The hash of the component.
			\return
				The index of the component.
			*//******************************************************************/
			uint32_t GetCompIndex(CompHash compHash) const;

			/*****************************************************************//*!
			\brief
				Gets the type of the task.
			\return
				The type of the task.
			*//******************************************************************/
			TYPE GetType() const;

			/*****************************************************************//*!
			\brief
				Sets the type of the task.
			\param type
				The new type of the task.
			*//******************************************************************/
			void SetType(TYPE newType);

			/*****************************************************************//*!
			\brief
				Compares whether this task's index is smaller than another task.
			\param other
				The other task.
			\return
				True if this task's index is smaller than the other task. False otherwise.
			*//******************************************************************/
			bool operator<(const CompModifyTask& other) const;

		private:
			//! The entity holding the component to be moved/removed.
			InternalEntityHandle entity;
			//! The task type.
			TYPE type;
		};

		/*****************************************************************//*!
		\class CompChangesBuffer
		\brief
			This class buffers additions and removals of components, so that iterators do not get invalidated
			if component additions/removals are called while systems, etc. are iterating through components.
		*//******************************************************************/
		class CompChangesBuffer
		{
		public:
			CompChangesBuffer() = default;
			// Disallow copying of this buffer.
			CompChangesBuffer(const CompChangesBuffer&) = delete;

			/*****************************************************************//*!
			\brief
				Add a component to be added to ECSPool at a later point in time.
			\tparam T
				The component type.
			\param entity
				A handle to the entity to add the component to.
			\param comp
				The component object. This will be moved, so it is unsafe to use after this call.
			\return
				The index to the added component within this buffer class.
			*//******************************************************************/
			template <typename T>
			uint32_t AddComp(InternalEntityHandle entity, T&& comp);

			/*****************************************************************//*!
			\brief
				Add a component to be removed from the ECSPool at a later point in time.
			\param entity
				The entity that holds the component to be removed.
			\param compType
				The component type's hash.
			*//******************************************************************/
			void RemoveComp(InternalEntityHandle entity, CompHash compType);

			/*****************************************************************//*!
			\brief
				Remove a component tohatl is buffered for addition within this buffer class.
				This does not inform the entity of the removal.
			\param compType
				The component type's hash.
			\param index
				The component's index within the compArr within this buffer class.
			*//******************************************************************/
			void RemoveCompBufferedForAddition(CompHash compType, uint32_t index);

			/*****************************************************************//*!
			\brief
				Buffers setting whether a component is active or inactive.
			\param entity
				The entity that holds the component.
			\param compType
				The component type's hash.
			\param isInactive
				Whether to set the component to inactive or active.
			*//******************************************************************/
			void ChangeCompActiveness(InternalEntityHandle entity, CompHash compType, bool isInactive);

			/*****************************************************************//*!
			\brief
				Buffers a clone of a component as an addition in this buffer.
				This does not inform the entity of the addition.
			\param srcArr
				The compArr that contains the component to be cloned.
			\param index
				The index of the component to be cloned within the srcArr.
			\param entityOwner
				The entity to attached the cloned component to.
			\return
				The index to the added component within this buffer class.
			*//******************************************************************/
			uint32_t CloneComp(CompArr& srcArr, uint32_t index, InternalEntityHandle entityOwner);

			/*****************************************************************//*!
			\brief
				Buffers an entity deletion.
			\param entity
				The entity to be deleted.
			*//******************************************************************/
			void DeleteEntity(InternalEntityHandle entity);

			/*****************************************************************//*!
			\brief
				Buffers a component callback.
				Note: Assuming that we're only buffering attached component callbacks, since detached callbacks should run immediately before the component is destroyed
			\param callback
				The callback function to call.
			\param entity
				The entity that has the component to call the callback on.
			*//******************************************************************/
			void AddComponentCallback(CompInformAttachedSig callback, InternalEntityHandle entity);

			/*****************************************************************//*!
			\brief
				Flushes pending additions and removals of components and deletions of entities to ECSPool, and component callbacks.
			*//******************************************************************/
			void FlushChanges();

			/*****************************************************************//*!
			\brief
				Flushes component callbacks.
			*//******************************************************************/
			void FlushComponentCallbacks();

			/*****************************************************************//*!
			\brief
				For shutdown purposes: Clear and reset the buffer. Doesn't make any changes to outside pools, components or entities.
			*//******************************************************************/
			void ClearAndReset();

		private:
			/*****************************************************************//*!
			\brief
				Gets the set that stores the indexes of the components to be removed for a component type from compsToModify.
				Creates it if it doesn't exist yet.
			*//******************************************************************/
			CompIndexSetType<CompModifyTask>& GetModifySet(CompHash compType);
			
		private:
			//! A map of component type hashes to compArrs that store components buffered for addition into ECSPool.
			CompArrMapType compsToAdd;
			//! A map of component type hashes to sets of index that store the indexes of components buffered for removal from ECSPool.
			ModifyCompSetType compsToModify;
			//! A vector containing handles to entities to be removed.
			RemoveEntContType entitiesToRemove;

			//! A vector containing pending component callbacks
			//! Note: Assuming that we're only buffering attached component callbacks, since detached callbacks should run immediately before the component is destroyed
			std::vector<std::pair<CompInformAttachedSig, InternalEntityHandle>> componentCallbacksQueue;

		};

		/*****************************************************************//*!
		\class CompArr
		\brief
			This class holds the memory of all components of a single type, and pointers to
			the entities that each component is attached to.
		*//******************************************************************/
		class CompArr
		{
		public:
			/*****************************************************************//*!
			\brief
				Constructs a compArr that stores components of the specified component type hash,
				and the specified parameters that tells it how to handle the components' data.
			\param compHash
				The component type hash.
			\param compSize
				The size of the component in bytes.
			\param copyFunc
				The method that copies a component.
			\param moveFunc
				The method that moves a component.
			\param destroyFunc
				The method that calls the component's destructor.
			\param informAttachedFunc
				The method that informs a component that it was attached to an entity.
			\param informDetachedFunc
				The method that informs a component that it was detached from an entity.
			\param trueInformAttachedFunc
				The method that always informs a component that it was attached to an entity.
				Used for cloning a CompArr.
			\param trueInformDetachedFunc
				The method that always informs a component that it was attached to an entity.
				Used for cloning a CompArr.
			*//******************************************************************/
			CompArr(CompHash compHash, uint32_t compSize, CompCopySig copyFunc, CompMoveSig moveFunc, CompDestroySig destroyFunc,
				CompInformAttachedSig informAttachedFunc, CompInformDetachedSig informDetachedFunc,
				CompInformAttachedSig trueInformAttachedFunc, CompInformDetachedSig trueInformDetachedFunc);

			/*****************************************************************//*!
			\brief
				Destroys this compArr, calling the component's destructor on all stored components.
			*//******************************************************************/
			~CompArr();

			// Disallow copying compArrs.
			CompArr(const CompArr&) = delete;

			/*****************************************************************//*!
			\brief
				Adds a component into this CompArr.
			\tparam
				The type of the component.
			\param entPtr
				The entity that this component is attached to.
			\param comp
				The component to be added into this CompArr.
			\param isInactive
				Whether the component is inactive.
			\return
				The index within this CompArr that the component was inserted into.
			*//******************************************************************/
			template <typename T>
			uint32_t AddComp(InternalEntityHandle entPtr, T&& comp, bool isInactive);

			/*****************************************************************//*!
			\brief
				Adds a component into this CompArr.
			\param entPtr
				The entity that this component is attached to.
			\param comp
				The component to be added into this CompArr.
				Note that this component will be moved into this CompArr.
			\param isInactive
				Whether the component is inactive.
			\return
				The index within this CompArr that the component was inserted into.
			*//******************************************************************/
			uint32_t AddComp(InternalEntityHandle entPtr, RawData* comp, bool isInactive);

			/*****************************************************************//*!
			\brief
				Destroys and removes a component from this CompArr.
			\param index
				The index of the component in this CompArr to be destroyed.
			\param informOwnerEntity
				Whether to tell the entity that owns the component of the removal.
			*//******************************************************************/
			void RemoveComp(uint32_t index, bool informOwnerEntity = true);

			/*****************************************************************//*!
			\brief
				Clone a component from this CompArr into specified CompArr, marking the specified entity as the owner of the clone.
				The entity is not informed of the addition.
			\param index
				The index of the component in this CompArr to be cloned into the other CompArr.
			\param entityOwner
				The entity that the cloned component will be attached to.
			\param destArr
				The CompArr to copy the cloned component into.
			\return
				The index of the cloned component in the other CompArr.
			*//******************************************************************/
			uint32_t CloneComp(uint32_t index, InternalEntityHandle entityOwner, CompArr& destArr);

			/*****************************************************************//*!
			\brief
				Sets whether a component is active or inactive.
			\param compAddr
				The address of the component in this CompArr.
			\param setActive
				Whether to set the component to be inactive or active.
			*//******************************************************************/
			void SetCompActiveness(void* compAddr, bool setInactive);

			/*****************************************************************//*!
			\brief
				Sets whether a component is active or inactive.
			\param index
				The index of the component in this CompArr.
			\param setActive
				Whether to set the component to be inactive or active.
			*//******************************************************************/
			void SetCompActiveness(uint32_t index, bool setInactive);

			/*****************************************************************//*!
			\brief
				Gets a pointer to the component at the specified index within this CompArr.
			\param index
				The index of the component in this CompArr to get.
			\return
				A pointer to the component at the specified index.
			*//******************************************************************/
			RawData* GetComp(uint32_t index);

			/*****************************************************************//*!
			\brief
				Gets a pointer to the component at the specified index within this CompArr.
			\param index
				The index of the component in this CompArr to get.
			\return
				A pointer to the component at the specified index.
			*//******************************************************************/
			const RawData* GetComp(uint32_t index) const;

			/*****************************************************************//*!
			\brief
				Transfer components stored in another CompArr to this CompArr.
			\param other
				The CompArr to transfer components from.
			*//******************************************************************/
			void TransferCompsFrom(CompArr& other);

			/*****************************************************************//*!
			\brief
				Constructs a copy of this CompArr's values and methods of how to handle its components in a CompArr map pool,
				but does not copy or move component data to the other CompArr.

				There has arisen a need for a cloning of a CompArr without component data to be placed into a
				different pool of CompArrs. However, returning a clone by value seems impossible with deleted copy
				constructor, no way (that I could find) to invoke copy elison when interacting with a return by value
				function and an unordered_map, and no simple way to return multiple values together. I'm also making
				a design decision to not expose each of the 7 variables required for cloning as getters. As such,
				this function exists to satisfy this specific need.
				Return value is the result of unordered_map.emplace().
			\param compArrPool
				The CompArr map pool to place the CompArr copy into.
			\return
				The result of unordered_map.emplace() into the CompArr map pool.
			*//******************************************************************/
			std::pair<CompArrMapType::iterator, bool> CloneWithoutCompDataIntoPool(CompArrMapType& compArrPool) const;

			/*****************************************************************//*!
			\brief
				For shutdown: Delete all components. This doesn't inform the entities.
			*//******************************************************************/
			void RemoveAllComps();

			/*****************************************************************//*!
			\brief
				Get the hash of the components that this CompArr contains.
			\return
				The component type hash of the components that this CompArr contains.
			*//******************************************************************/
			CompHash GetCompHash() const;

			/*****************************************************************//*!
			\brief
				Get the number of components stored in this CompArr.
			\return
				The number of components stored in this CompArr.
			*//******************************************************************/
			uint32_t GetNumComps() const;

			/*****************************************************************//*!
			\brief
				Gets whether a component stored at a certain address within this CompArr
				is active or inactive.
			\param compAddr
				The address of the component.
			\return
				True if the component is active. False if inactive.
			*//******************************************************************/
			bool GetIsCompActive(const void* compAddr) const;

			/*****************************************************************//*!
			\brief
				Gets whether a component stored at a certain index within this CompArr
				is active or inactive.
			\param index
				The index of the component.
			\return
				True if the component is active. False if inactive.
			*//******************************************************************/
			bool GetIsCompActive(uint32_t index) const;

		private:
			/*****************************************************************//*!
			\brief
				Expands/Contracts the vector size to fit the specified number of components.
				If the requested size is greater than the allocated memory, moves all components
				to the new memory via calling the move constructor (so components' memory are
				moved correct, avoiding troubles with STL due to errors with the allocater).
			\param numComps
				The requested number of components to store.
			*//******************************************************************/
			void SetArraySize(uint32_t numComps);

			/*****************************************************************//*!
			\brief
				Inserts the pointer to this CompArr within the array starting from startIndex
				and ending 1 before endIndex.
			\param startIndex
				The starting index.
			\param endIndex
				1 past the end index.
			*//******************************************************************/
			void InsertCompArrPtr(uint32_t startIndex, uint32_t endIndex);

			/*****************************************************************//*!
			\brief
				Resets the entity owner of a component at the specified index to the specified entity.
			\param index
				The index of the component within this CompArr whose entity owner is to be reset.
			\param entPtr
				The entity that will own the specified component.
			*//******************************************************************/
			void SetEntityPtr(uint32_t index, InternalEntityHandle entPtr);

			/*****************************************************************//*!
			\brief
				Resets the entity owner of a component at the specified index of the specified vector to the specified entity.
			\param index
				The index within the vector where the entity pointer is at.
			\param entPtr
				The entity that will own the specified component.
			\param arr
				The vector storing the entity pointers and components.
			*//******************************************************************/
			void SetEntityPtr(size_t index, InternalEntityHandle entPtr, CompContType* arr);

			/*****************************************************************//*!
			\brief
				Resizes the array, shuffles compoennts around as needed, and returns
				an index to the free space where a new component can be inserted into.
			\param isInactive
				Whether the component to be inserted is inactive or active.
			\return
				The index of the free space.
			*//******************************************************************/
			uint32_t ExpandArrayForComp(bool isInactive);

			/*****************************************************************//*!
			\brief
				Resizes the array, shuffles compoennts around as needed, and returns
				indexes to the beginning of free space where new active/inactive components can be inserted into.
			\param numActiveToAdd
				The number of active components to allocate for.
			\param numInactiveToAdd
				The number of inactive components to allocate for.
			\return
				A pair of indexes, first is the first active index, second is the first inactive index.
			*//******************************************************************/
			std::pair<uint32_t, uint32_t> ExpandArrayForComp(uint32_t numActiveToAdd, uint32_t numInactiveToAdd);

			/*****************************************************************//*!
			\brief
				Moves a component and its entity pointer to a destination index.
			\param srcIndex
				The current index of the component.
			\param destIndex
				The index to move the component to.
			*//******************************************************************/
			void MoveComp(uint32_t srcIndex, uint32_t destIndex);

			/*****************************************************************//*!
			\brief
				Gets the entity owner of a comonent in this CompArr at the specified index.
			\param index
				The index of the component within this CompArr.
			\return
				The entity that owns the specified component.
			*//******************************************************************/
			InternalEntityHandle GetEntity(uint32_t index);

		public:
			//! The number of bytes of the pointer that points to the entity owner of each component.
			static constexpr uint32_t EntPtrSize{ sizeof(InternalEntityHandle) };
			//! The number of bytes of the pointer that points to this CompArr.
			static constexpr uint32_t CompArrPtrSize{ sizeof(CompArr*) };

			//! The number of bytes reserved before component data for each component.
			static constexpr uint32_t ReservedBytes{ CompArrPtrSize + EntPtrSize };

		private:
			// Components are stored packed together with each element as:
			// 8bytes CompArrPtr + 8bytes EntityPtr + ?bytes Component
			// Inactive components are placed to the left, active to the right.

			//! The container that stores all components and entity owner pointers, with the format:
			//! 8bytes CompArrPtr + 8bytes EntityPtr + ?bytes Component
			CompContType arrRaw;
			//! The hash of the component type that this CompArr stores.
			const CompHash compHash;
			//! The number of bytes of each component.
			const uint32_t compSize;
			//! The number of bytes to jump by along arrRaw to get to the next component.
			//! Equal to CompArrPtrSize + EntPtrSize + compSize
			const uint32_t compStepSize;

			//! A count of the number of inactive components stored within this CompArr.
			uint32_t numInactive;
			//! Because the first active component's index matches the number of inactive components, this exists for easier reading.
			uint32_t& firstActiveIndex;

			//! The method to copy a component.
			const CompCopySig callCopyFunc;
			//! The method to move a component.
			const CompMoveSig callMoveFunc; // If we are moving components internally, remember to call the destructor!
			//! The method to call the destructor of a component.
			const CompDestroySig callDestructorFunc;
			//! The method to inform a component that it was attached to an entity. May be empty if the pool has component callbacks disabled.
			const CompInformAttachedSig callInformAttachedFunc;
			//! The method to inform a component that it was detached from an entity. May be empty if the pool has component callbacks disabled.
			const CompInformDetachedSig callInformDetachedFunc;

			//! The method to inform a component that is was attached to an entity. Always calls the component callback. Used for cloning comp arr without data.
			const CompInformAttachedSig trueInformAttachedFunc;
			//! The method to inform a component that is was attached to an entity. Always calls the component callback. Used for cloning comp arr without data.
			const CompInformAttachedSig trueInformDetachedFunc;

			//! Temporary space to store component data while doing shuffling operations.
			const std::unique_ptr<RawData[]> tempCompSpace;


			/* ITERATOR SUPPORT */
		public:
			/*****************************************************************//*!
			\struct iterator_blueprint
			\brief
				This is the implementation for an iterator that iterates all components within a CompArr.
			\tparam CompType
				The type of the components within the CompArr.
			\tparam EntityHandleType
				The type of the handle to the entity that owns each component within the CompArr. This is exposed to support class types inheriting from Entity_Internal.
			\tparam ValueType
				The type of the data that this iterator iterates.
			*//******************************************************************/
			template <typename CompType, typename EntityHandleType, typename ValueType = RawData>
			struct iterator_blueprint
			{
				// note: these types may not be correct... further research needed if we want to support <algorithm> library
				using iterator_category = std::bidirectional_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = ValueType;
				using pointer = ValueType*;
				using reference = ValueType&;

				/*****************************************************************//*!
				\brief
					Constructs an iterator to the CompArr.
				\param compStepSize
					The number of bytes to step over to increment/decrement to the next/previous component.
				\param ptrToCompBaseLoc
					A pointer to the memory location of the entity that owns the component.
				*//******************************************************************/
				iterator_blueprint(uint32_t compStepSize, pointer ptrToCompBaseLoc);

				/*****************************************************************//*!
				\brief
					Dereferences the iterator, accessing the component pointed to by this iterator.
				\return
					Reference to the component.
				*//******************************************************************/
				CompType& operator*() const;

				/*****************************************************************//*!
				\brief
					Dereferences the iterator, accessing the component pointed to by this iterator.
				\return
					Pointer to the component.
				*//******************************************************************/
				CompType* operator->() const;

				/*****************************************************************//*!
				\brief
					Gets a handle to the component pointed to by this iterator.
				\return
					A handle to the component.
				*//******************************************************************/
				InternalCompHandle<CompType> GetComp();

				/*****************************************************************//*!
				\brief
					Gets a handle to the entity that the component pointed to by this iterator is attached to.
				\return
					A handle to the entity.
				*//******************************************************************/
				EntityHandleType GetEntity();

				/*****************************************************************//*!
				\brief
					Gets whether the component pointed to by this iterator is active or inactive.
				\return
					True if the component is active. False otherwise.
				*//******************************************************************/
				bool GetIsActive() const;

				/*****************************************************************//*!
				\brief
					Prefix increments this iterator to the next component within the CompArr.
				\return
					A reference to this iterator.
				*//******************************************************************/
				iterator_blueprint& operator++();

				/*****************************************************************//*!
				\brief
					Postfix increments this iterator to the next component within the CompArr.
				\return
					A temporary of the original state of this iterator.
				*//******************************************************************/
				iterator_blueprint operator++(int);

				/*****************************************************************//*!
				\brief
					Prefix decrements this iterator to the previous component within the CompArr.
				\return
					A reference to this iterator.
				*//******************************************************************/
				iterator_blueprint& operator--();

				/*****************************************************************//*!
				\brief
					Postfix increments this iterator to the previous component within the CompArr.
				\return
					A temporary of the original state of this iterator.
				*//******************************************************************/
				iterator_blueprint operator--(int);

				/*****************************************************************//*!
				\brief
					Increments or decrements an iterator by the specified amount.
				\tparam CompType_T
					The iterator's CompType template parameter type.
				\tparam EntityHandleType_T
					The iterator's EntityHandleType template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param iter
					The iterator to increment/decrement.
				\param offset
					The number of times to increment/decrement the iterator by.
				\return
					A temporary of this iterator incremented/decremented by the specified number of times.
				*//******************************************************************/
				template <typename CompType_T, typename EntityHandleType_T, typename ValueType_T>
				friend iterator_blueprint<CompType_T, EntityHandleType_T, ValueType_T> operator+(
					const iterator_blueprint<CompType_T, EntityHandleType_T, ValueType_T>& iter, int offset);

				/*****************************************************************//*!
				\brief
					Tests if 2 iterators point to the same component within this CompArr.
				\tparam CompType_T
					The iterator's CompType template parameter type.
				\tparam EntityHandleType_T
					The iterator's EntityHandleType template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param a
					Left-hand side iterator.
				\param b
					Right-hand side iterator.
				\return
					True if both iterators point to the same component within this CompArr. False otherwise.
				*//******************************************************************/
				template <typename CompType_T, typename EntityHandleType_T, typename ValueType_T>
				friend bool operator==(
					const iterator_blueprint<CompType_T, EntityHandleType_T, ValueType_T>& a,
					const iterator_blueprint<CompType_T, EntityHandleType_T, ValueType_T>& b);

				/*****************************************************************//*!
				\brief
					Tests if 2 iterators do not point to the same component within this CompArr.
				\tparam CompType_T
					The iterator's CompType template parameter type.
				\tparam EntityHandleType_T
					The iterator's EntityHandleType template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param a
					Left-hand side iterator.
				\param b
					Right-hand side iterator.
				\return
					True if both iterators do not point to the same component within this CompArr. False otherwise.
				*//******************************************************************/
				template <typename CompType_T, typename EntityHandleType_T, typename ValueType_T>
				friend bool operator!=(
					const iterator_blueprint<CompType_T, EntityHandleType_T, ValueType_T>& a,
					const iterator_blueprint<CompType_T, EntityHandleType_T, ValueType_T>& b);

			private:
				//! The pointer to the entity that owns the component that we're pointing to.
				pointer ptr;
				//! The number of bytes to offset the pointer to get to the next/previous component.
				uint32_t compStepSize;
			};
			
			using iterator = iterator_blueprint<RawData, InternalEntityHandle>;
			using const_iterator = iterator_blueprint<const RawData, InternalEntityHandle, const RawData>;

			/*****************************************************************//*!
			\brief
				Creates an iterator to the first component in this CompArr that accesses the component data as RawData and entities as InternalEntityHandle.
			\return
				An iterator to the first component in this CompArr.
			*//******************************************************************/
			iterator begin();

			/*****************************************************************//*!
			\brief
				Creates a const iterator to the first component in this CompArr that accesses the component data as const RawData and entities as InternalEntityHandle.
			\return
				A const iterator to the first component in this CompArr.
			*//******************************************************************/
			const_iterator begin() const;

			/*****************************************************************//*!
			\brief
				Creates an iterator to the first active component in this CompArr that accesses the component data as RawData and entities as InternalEntityHandle.
			\return
				An iterator to the first active component in this CompArr.
			*//******************************************************************/
			iterator begin_active();

			/*****************************************************************//*!
			\brief
				Creates a const iterator to the first active component in this CompArr that accesses the component data as const RawData and entities as InternalEntityHandle.
			\return
				A const iterator to the first active component in this CompArr.
			*//******************************************************************/
			const_iterator begin_active() const;

			/*****************************************************************//*!
			\brief
				Creates an iterator to 1 past the last component in this CompArr that accesses the component data as RawData and entities as InternalEntityHandle.
			\return
				An iterator to 1 past the component in this CompArr.
			*//******************************************************************/
			iterator end();

			/*****************************************************************//*!
			\brief
				Creates a const iterator to 1 past the last component in this CompArr that accesses the component data as const RawData and entities as InternalEntityHandle.
			\return
				A const iterator to 1 past the last component in this CompArr.
			*//******************************************************************/
			const_iterator end() const;

			/*****************************************************************//*!
			\brief
				Creates an iterator to the first component in this CompArr with a simplified template parameter options list.
			\return
				An iterator to the first component in this CompArr.
			*//******************************************************************/
			template <typename CompType, typename EntityHandleType>
			iterator_blueprint<CompType, EntityHandleType> User_Begin();

			/*****************************************************************//*!
			\brief
				Creates an iterator to the first active component in this CompArr with a simplified template parameter options list.
			\return
				An iterator to the first active component in this CompArr.
			*//******************************************************************/
			template <typename CompType, typename EntityHandleType>
			iterator_blueprint<CompType, EntityHandleType> User_Begin_Active();

			/*****************************************************************//*!
			\brief
				Creates an iterator to 1 past the last component in this CompArr with a simplified template parameter options list.
			\return
				An iterator to 1 past the last component in this CompArr.
			*//******************************************************************/
			template <typename CompType, typename EntityHandleType>
			iterator_blueprint<CompType, EntityHandleType> User_End();

			/*****************************************************************//*!
			\brief
				Creates an iterator to 1 past the last component in this CompArr with a simplified template parameter options list.
			\return
				An iterator to 1 past the last component in this CompArr.
			*//******************************************************************/
			template <typename CompType, typename EntityHandleType>
			iterator_blueprint<CompType, EntityHandleType> User_Custom(InternalEntityHandle entityHandle);

		};

		/*****************************************************************//*!
		\class CompArrNotFoundException
		\brief
			This class is thrown by non-template GetCompArr() if it can't find the CompArr specified.
		*//******************************************************************/
		class CompArrNotFoundException : public std::exception {};

#pragma endregion // Components

#pragma region Systems

		/*****************************************************************//*!
		\class System_Internal_Base
		\brief
			This class exists so we can store all systems as 1 type no matter their requested components.
			It is the base class of ecs::System<>
		*//******************************************************************/
		class System_Internal_Base
		{
		public:
			// Default destructor, explicitly specified for virutalization.
			virtual ~System_Internal_Base() = default;

			/*****************************************************************//*!
			\brief
				Called by ECS when the system is fully added into ecs.
				User implementations may override this.
			*//******************************************************************/
			virtual void OnAdded() {}

			/*****************************************************************//*!
			\brief
				Called by ECS when the system is removed from ecs.
				User implementations may override this.
			*//******************************************************************/
			virtual void OnRemoved() {}

			/*****************************************************************//*!
			\brief
				Called by ECS to executes the system on each entity whose components meet the system's requirements.
				User implementations should only override this if they have a special implementation for getting required entities for processing.
			*//******************************************************************/
			virtual void Run() = 0;

			/*****************************************************************//*!
			\brief
				Called by ECS before entities start being processed by this system.
				User implementations may override this.
			\return
				True if the system should execute. False otherwise.
				User implementations may use this to control whether a system runs or not.
			*//******************************************************************/
			virtual bool PreRun() { return true; }

			/*****************************************************************//*!
			\brief
				Called by ECS once all entities have been processed by this system.
				User implementations may override this.
			*//******************************************************************/
			virtual void PostRun() {}

		protected:
			/*****************************************************************//*!
			\brief
				Dummy function that does nothing. This is for compatibility with System_Internal's constructor, where
				in the case of a System requiring no components, this is used to satisfy the constructor's parameters.
			*//******************************************************************/
			void DummyFunc() {};
		};

		/*****************************************************************//*!
		\class System_Internal
		\brief
			This class should be inherited by user systems class types to conform to this ecs's expectations of system types.
			This class implements the algorithm to gather required components from each entity that meets the requirements and call the user function to process those components.
		\tparam SysType
			The system type.
		\tparam Args
			The components that the system requires before it processes the entity's components.
			If 2 or more components are specified, entities must have all components before being processed by this system.
			If there are 0 components, no entities are passed to this system for processing.
		*//******************************************************************/
		template <typename SysType, typename... Args>
		class System_Internal : public System_Internal_Base
		{
		private:
			/*****************************************************************//*!
			\brief
				Registers the system type into RegisteredSystems.
			\return
				Dummy bool.
			*//******************************************************************/
			static bool RegisterSystem();

			//! Calls RegisterSystem() once when this static is initialized at startup
			inline static bool isRegistered{ RegisterSystem() };

		public:
			/*****************************************************************//*!
			\brief
				Constructor. User-defined systems requiring 1 or more components will need to pass a member function that takes those components as references.
				e.g. System<PhysicsComp, CollisionComp> // a system requiring PhysicsComp and CollisionComp
					 will require a function defined as void FuncName(PhysicsComp&, CollisionComp&)
					 and pass it as &ClassName::FuncName to this constructor.
				If a user-defined system requires no components, no explicit constructor call is required.
			\tparam ReturnType
				The return type of the value returned by the user-defined member function.
			\tparam ClassType
				The class type of the user-defined member function.
			\param memberFunc
				The user-defined member function that will be called per entity to be processed.
			*//******************************************************************/
			template <typename ReturnType = void, typename ClassType = System_Internal_Base>
			System_Internal(ReturnType(ClassType::* memberFunc)(Args&...) = &System_Internal_Base::DummyFunc);

			explicit System_Internal(int _cpp_par_);

			/*****************************************************************//*!
			\brief
				Called by ECS to executes the system on each entity whose components meet the system's requirements.
				This implementation gathers required components from each entity that meets the requirements and calls the user function to process those components.
				User implementations should only override this if they have a special implementation for getting required entities for processing.
			*//******************************************************************/
			virtual void Run() override;

		protected:
			/*****************************************************************//*!
			\brief
				Executes this system on each entity that has a component in the provided array.
			\param compArr
				The component array to execute on.
			*//******************************************************************/
			template <typename Predicate = std::nullptr_t>
				requires std::is_same_v<Predicate, std::nullptr_t> || std::predicate<Predicate, internal::InternalEntityHandle>
			void RunOnCompArr(CompArr& compArr, Predicate pred = nullptr);

			/*****************************************************************//*!
			\brief
				This function exists for optimization, where it only does a map lookup within the specified entity for
				the requested component type, if the specified component that is already obtained does not match the component type.
			\param entity
				The entity to search the requested component in if the obtained component does not match the requested type.
			\param obtainedCompHash
				The component type hash of the comonent that is already obtained.
			\param obtainedComp
				The handle to the component that is already obtained.
			\param desiredCompHash
				The requested component's type hash.
			\return
				A handle to the requested component.
			*//******************************************************************/
			static InternalCompHandle<RawData> GetComponent(InternalEntityHandle entity, CompHash obtainedCompHash, InternalCompHandle<RawData> obtainedComp, CompHash desiredCompHash);

		protected:
			//! The user-defined system's function that processes 1 component set.
			const std::function<void(System_Internal*, Args&...)> callProcessEntity;

		};

		/*****************************************************************//*!
		\class SystemsManager
		\brief
			This class stores various systems defined by the user and separates them into layers, for simplier management of systems.

			// TODO: It might make sense to have systems be stored all in one place,
			// then each SystemsManager in each ECSPool points to the systems,
			// so we have the option to share systems between ECSPools.
		*//******************************************************************/
		class SystemsManager
		{
		public:
			/*****************************************************************//*!
			\brief
				Destructor. Cleans up and deletes leftover systems.
			*//******************************************************************/
			~SystemsManager();

			/*****************************************************************//*!
			\brief
				Adds a user-defined system into a layer within this SystemManager.
			\tparam SysType
				The type of the user-defined system.
			\param layer
				The layer to add the user-defined system into.
			\param system
				The system object to be added into this SystemManager.
				As this object will be moved into this SystemManager, it is not safe to use the system object after this function call.
			\return
				A handle to the added system object.
			*//******************************************************************/
			template <typename SysType>
			InternalGenericSysHandle<SysType> AddSystem(int layer, SysType&& system);

			/*****************************************************************//*!
			\brief
				Gets a handle to the requested system that is stored within this SystemManager.
				This operation is slightly expensive, so try not to call this unless required.
			\tparam SysType
				The type of the requested system.
			\return
				A handle to the requested system object.
				nullptr if the system does not exist within this SystemManager.
			*//******************************************************************/
			template <typename SysType>
			InternalGenericSysHandle<SysType> GetSystem();

			/*****************************************************************//*!
			\brief
				Gets all systems stored within this SystemManager.
				This operation is quite expensive, so try not to call this unless required.
			\return
				A container of pointers to all systems.
			*//******************************************************************/
			std::vector<InternalGenericSysHandle<System_Internal_Base>> GetAllSystems();

			/*****************************************************************//*!
			\brief
				Removes and deletes a system from this SystemManager.
			\tparam SysType
				The type of the system to remove.
			\return
				True if the system was successfully removed. False otherwise.
			*//******************************************************************/
			template <typename SysType>
			bool RemoveSystem();

			/*****************************************************************//*!
			\brief
				Removes and deletes all systems within a layer in this SystemManager.
			*//******************************************************************/
			void RemoveSystemsInLayer(int layer);

			/*****************************************************************//*!
			\brief
				Removes and deletes all systems stored in this SystemManager.
			*//******************************************************************/
			void RemoveAllSystems();

			/*****************************************************************//*!
			\brief
				Run all systems stored within a layer in this SystemManager. The order of systems
				within a layer is unspecified.
			\param layer
				Systems within this layer will be run.
			*//******************************************************************/
			void RunSystems(int layer);

		private:
			/*****************************************************************//*!
			\brief
				Gets the map container that stores all system objects in the specified layer.
			\param layer
				The layer that the map to be retrieved is in.
			\return
				The map container that stores all system objects in the specified layer.
			*//******************************************************************/
			SysMapType& GetSystemsMap(int layer);

		private:
			//! The map container that maps layer to maps, each of which maps system type hash to system objects.
			SysLayerMapType layerToSystemsMap;
			//! The map container that maps system type hash to the layer that the system is in.
			//! This exists to speed up GetSystem() queries, so we don't have to linearly search all layers for the requested system.
			SysHashToLayerMapType hashToLayerMap;
		};

#pragma endregion // Systems

#pragma region Type Meta

		/*****************************************************************//*!
		\class CompTypeMeta
		\brief
			This class stores information about a component type and some useful things about it.
		*//******************************************************************/
		struct CompTypeMeta
		{
		public:
			//! The component type hash of the component that this metadata describes.
			CompHash hash;
			//! The component's type as a string.
			std::string name;
		};

		/*****************************************************************//*!
		\class SysTypeMeta
		\brief
			This class stores information about a system type and some useful things about it.
		*//******************************************************************/
		struct SysTypeMeta
		{
		public:
			//! The system type hash of the system that this metadata describes.
			SysHash hash;
			//! The system's type as a string.
			std::string name;
		};

		/*****************************************************************//*!
		\class TypeMetaManager
		\brief
			This class stores the metadata of all component and system types that have ever been added into ecs.
		*//******************************************************************/
		class TypeMetaManager
		{
		public:
			/*****************************************************************//*!
			\brief
				Creates the metadata object that describes the specified type and stores it in a map container.
			\tparam CompType
				The component type.
			*//******************************************************************/
			template <typename CompType>
			void RegisterCompType();

			/*****************************************************************//*!
			\brief
				Gets the metadata of a component type, based on the component type's hash.
			\param hash
				The component type's hash.
			\return
				A const pointer to the metadata of the requested component type.
			*//******************************************************************/
			const CompTypeMeta* GetCompTypeMeta(CompHash hash) const;

			/*****************************************************************//*!
			\brief
				Creates the metadata object that describes the specified type and stores it in a map container.
			\tparam SysType
				The system type.
			*//******************************************************************/
			template <typename SysType>
			void RegisterSysType();

			/*****************************************************************//*!
			\brief
				Gets the metadata of a system type, based on the system type's hash.
			\param hash
				The system type's hash.
			\return
				A const pointer to the metadata of the requested system type.
			*//******************************************************************/
			const SysTypeMeta* GetSysTypeMeta(SysHash hash) const;

			/*****************************************************************//*!
			\brief
				Gets the metadata of all systems.
			\return
				The map containing all system metadata.
			*//******************************************************************/
			const SysTypeMetaMap& GetAllSysTypeMeta() const;

			/*****************************************************************//*!
			\brief
				For shutdown purposes: Clears all metadata saved.
			*//******************************************************************/
			void Clear();

		private:
			//! The map container that maps component types' hashes to their metadata object describing their types.
			CompTypeMetaMap compTypeMetaMap;
			//! The map container that maps system types' hashes to their metadata object describing their types.
			SysTypeMetaMap sysTypeMetaMap;
		};

#pragma endregion // Type Meta

#pragma region Pools

		/*****************************************************************//*!
		\class EntityMapWrapper
		\brief
			Wraps the map of entities so that validEntityHandles will also be
			updated alongside changes to the entities map.
		*//******************************************************************/
		class EntityMapWrapper
		{
		public:
			/*****************************************************************//*!
			\brief
				Constructor
			\param entities
				The map of entities in the ECS pool.
			\param validEntityHandles
				The set that contains valid handles to entities within the map of entities.
			*//******************************************************************/
			EntityMapWrapper(EntMapType& entities, ValidEntHandleSetType& validEntityHandles);

			/*****************************************************************//*!
			\brief
				Creates an entity parented to the parent if specified.
			\param parent
				The parent to parent the newly created entity to.
			\return
				A handle to the newly created entity.
			*//******************************************************************/
			InternalEntityHandle CreateEntity(InternalEntityHandle parent);

			/*****************************************************************//*!
			\brief
				Creates an entity with the specified transform.
			\param transformCopy
				The transform to copy.
			\return
				A handle to the newly created entity.
			*//******************************************************************/
			InternalEntityHandle CreateEntity(Transform& transformCopy);

			/*****************************************************************//*!
			\brief
				Gets an entity with the specified hash.
			\param hash
				The hash of the entity.
			\return
				A handle to the entity.
			*//******************************************************************/
			InternalEntityHandle GetEntity(EntityHash hash);

			/*****************************************************************//*!
			\brief
				Deletes an entity with the specified hash.
			\param hash
				The hash of the entity.
			*//******************************************************************/
			void EraseEntity(EntityHash hash);

			/*****************************************************************//*!
			\brief
				Removes all entities.
			*//******************************************************************/
			void ClearAll();

			/*****************************************************************//*!
			\brief
				Checks if a given entity handle is valid within this ECS pool.
			\param handle
				The handle to the supposed entity.
			\return
				True if the handle is valid. False otherwise.
			*//******************************************************************/
			bool CheckValidHandle(InternalEntityHandle handle);

		private:
			//! The map of entity hashes to entities.
			EntMapType& entities;
			//! The set of valid entity handles.
			ValidEntHandleSetType& validEntityHandles;

		};

		/*****************************************************************//*!
		\class ECSPool
		\brief
			This class contains a set of all active entities, components, systems and component type metadata that are stored within ecs.
			This is to allow different sets/pools of entities, components and systems.
		*//******************************************************************/
		class ECSPool
		{
		public:
			/*****************************************************************//*!
			\brief
				Constructor
			\param id
				The id of this ECS pool.
			\param compCallbacksEnabled
				Whether component callbacks are enabled within this pool.
			*//******************************************************************/
			ECSPool(int id, bool compCallbacksEnabled);

			/*****************************************************************//*!
			\brief
				Gets whether component callbacks are enabled within this pool.
			\return
				True if component callbacks are enabled. False otherwise.
			*//******************************************************************/
			bool GetIsCompCallbacksEnabled() const;

			// It was kind of a mistake to make these variables public... but well that's
			// too late now. Best we can do is hide stuff that shouldn't be public anymore and
			// leave the rest as public for now.
		private:
			//! Map of entity UID to Entity_Internal class objects, stores all entities
			EntMapType entities;
			//! Set storing valid entity handles to entities within this pool.
			ValidEntHandleSetType validEntityHandles;

		public:
			//! Map of component hash to compArr, stores all components
			CompArrMapType comps;
			//! A buffer that stores component and entity changes until it is safe to flush them.
			CompChangesBuffer changesBuffer;
			//! SystemsManager that stores all systems
			SystemsManager systems;

			//! Wrapper for this pools' entities
			EntityMapWrapper entitiesWrapper;

			//! The id of this pool
			int id;
		private:
			//! Whether this pool has component callbacks enabled.
			bool compCallbacksEnabled;

			/* ITERATOR SUPPORT */
		public:
			/*****************************************************************//*!
			\struct Entity_InteratorBlueprint
			\brief
				This is the implementation for an iterator that iterates all active entities in ecs.
			\tparam EntityHandleType
				The type of the handle to entities that is used in this iterator. This is exposed to optionally allow const implementations.
			\tparam EntContIterType
				The type of the iterator to the container that holds entities. This is exposed to support either map or normal container types, and optionally allow const implementations.
			\tparam ValueType
				The type of the data that this iterator iterates.
			*//******************************************************************/
			template <typename EntityHandleType, typename EntContIterType, typename ValueType = std::remove_pointer_t<EntityHandleType>>
			struct Entity_IteratorBlueprint
			{
				// note: these types may not be correct... further research needed if we want to support <algorithm> library
				using iterator_category = std::bidirectional_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = ValueType;
				using pointer = ValueType*;
				using reference = ValueType&;

				/*****************************************************************//*!
				\brief
					Constructs an iterator to the entity map with the provided iterators to the map.
				\param iterToEntity
					The entity that this iterator will point to. If the entity is marked for deletion,
					the iterator will increment until it encounters an alive entity or reaches the end of the map.
				\param endIter
					An iterator to 1 past the end of the entity map. This is required for this iterator to
					know when it can't increment anymore while searching for an alive entity.
				*//******************************************************************/
				Entity_IteratorBlueprint(const EntContIterType& iterToEntity, const EntContIterType& endIter);

				/*****************************************************************//*!
				\brief
					Dereferences the iterator, accessing the entity pointed to by this iterator.
				\return
					A handle to the entity.
				*//******************************************************************/
				EntityHandleType operator*() const;

				/*****************************************************************//*!
				\brief
					Dereferences the iterator, accessing the entity pointed to by this iterator.
				\return
					A handle to the entity.
				*//******************************************************************/
				EntityHandleType operator->() const;

				/*****************************************************************//*!
				\brief
					Get a handle to the entity pointed to by this iterator.
				\return
					A handle to the entity.
				*//******************************************************************/
				EntityHandleType GetEntity();

				/*****************************************************************//*!
				\brief
					Prefix increments this iterator to the next active entity within the map.
				\return
					A reference to this iterator.
				*//******************************************************************/
				Entity_IteratorBlueprint& operator++();

				/*****************************************************************//*!
				\brief
					Postfix increments this iterator to the next active entity within the map.
				\return
					A temporary of the original state of this iterator.
				*//******************************************************************/
				Entity_IteratorBlueprint operator++(int);

				/*****************************************************************//*!
				\brief
					Prefix decrements this iterator to the previous active entity within the map.
				\return
					A reference to this iterator.
				*//******************************************************************/
				Entity_IteratorBlueprint& operator--();

				/*****************************************************************//*!
				\brief
					Postfix decrements this iterator to the previous active entity within the map.
				\return
					A temporary of the original state of this iterator.
				*//******************************************************************/
				Entity_IteratorBlueprint operator--(int);

				/*****************************************************************//*!
				\brief
					Increments or decrements an iterator by the specified amount.
				\tparam EntityHandleType_T
					The iterator's EntityHandleType template parameter type.
				\tparam EntContIterType_T
					The iterator's EntContIter template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param iter
					The iterator to increment/decrement.
				\param offset
					The number of times to increment/decrement the iterator by.
				\return
					A temporary of this iterator incremented/decremented by the specified number of times.
				*//******************************************************************/
				template <typename EntityHandleType_T, typename EntContIterType_T, typename ValueType_T>
				friend Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T> operator+(
					const Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& iter, int offset);

				/*****************************************************************//*!
				\brief
					Tests if 2 iterators point to the same entity
				\tparam EntityHandleType_T
					The iterator's EntityHandleType template parameter type.
				\tparam EntContIterType_T
					The iterator's EntContIter template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param a
					Left-hand side iterator.
				\param b
					Right-hand side iterator.
				\return
					True if both iterators point to the same entity. False otherwise.
				*//******************************************************************/
				template <typename EntityHandleType_T, typename EntContIterType_T, typename ValueType_T>
				friend bool operator==(
					const Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& a,
					const Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& b);

				/*****************************************************************//*!
				\brief
					Tests if 2 iterators do not point to the same entity
				\tparam EntityHandleType_T
					The iterator's EntityHandleType template parameter type.
				\tparam EntContIterType_T
					The iterator's EntContIter template parameter type.
				\tparam ValueType_T
					The iterator's ValueType template parameter type.
				\param a
					Left-hand side iterator.
				\param b
					Right-hand side iterator.
				\return
					True if both iterators do not point to the same entity. False otherwise.
				*//******************************************************************/
				template <typename EntityHandleType_T, typename EntContIterType_T, typename ValueType_T>
				friend bool operator!=(
					const Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& a,
					const Entity_IteratorBlueprint<EntityHandleType_T, EntContIterType_T, ValueType_T>& b);

			private:
				/*****************************************************************//*!
				\brief
					Checks whether this iterator is currently pointing to a deleted entity.
				\return
					True if this iterator is currently pointing to a deleted entity. False otherwise.
				*//******************************************************************/
				bool IsPointingToDeletedEntity() const;

				/*****************************************************************//*!
				\brief
					Continually increments or decrements this iterator until an alive entity is found,
					or the end of the map is reached.
				\param isIncrement
					If true, increments this iterator. Otherwise, decrements this iterator.
				*//******************************************************************/
				void TravelNext(bool isIncrement);

				//! The entity map iterator pointing to the current entity.
				EntContIterType iter;
				//! The entity map iterator pointing to 1 past the end.
				EntContIterType endIter;
			};

		public:
			/*****************************************************************//*!
			\brief
				Creates an entity iterator to the beginning of ECSPool::entity with a simplified template parameter options list.
			\tparam EntityHandleType
				The iterator's EntityHandleType template parameter type.
			\return
				An entity iterator to the first entity in ECSPool::entity.
			*//******************************************************************/
			template <typename EntityHandleType>
			Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> Entity_User_Begin();

			/*****************************************************************//*!
			\brief
				Creates an entity iterator to 1 past the end of ECSPool::entity with a simplified template parameter options list.
			\tparam EntityHandleType
				The iterator's EntityHandleType template parameter type.
			\return
				An entity iterator to 1 past the last entity in ECSPool::entity.
			*//******************************************************************/
			template <typename EntityHandleType>
			Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> Entity_User_End();
		};

		/*****************************************************************//*!
		\class CurrentPool
		\brief
			This class stores all created ECSPools and provides interfaces to manage and
			access the active ECSPool.
		*//******************************************************************/
		class CurrentPool
		{
		public:
			CurrentPool() = delete;

			/*****************************************************************//*!
			\brief
				Initializes ECS.
			\param defaultPoolHasCompCallbacksEnabled
				Whether the default pool (0) has component callbacks enabled.
			*//******************************************************************/
			static void Init(bool defaultPoolHasCompCallbacksEnabled);

			/*****************************************************************//*!
			\brief
				Shuts down ECS.
			*//******************************************************************/
			static void Shutdown();

			/*****************************************************************//*!
			\brief
				Gets the pool id of the active ECSPool.
			\return
				The id of the active ECSPool.
			*//******************************************************************/
			static int GetId();

			/*****************************************************************//*!
			\brief
				Gets the CompArr map of the active ECSPool.
			\return
				The CompArr map of the active ECSPool.
			*//******************************************************************/
			static CompArrMapType& Comps();

			/*****************************************************************//*!
			\brief
				Gets the Entities map of the active ECSPool.
			\return
				The Entities map of the active ECSPool.
			*//******************************************************************/
			static EntityMapWrapper& Entities();

			/*****************************************************************//*!
			\brief
				Gets the changes buffer of the active ECSPool.
			\return
				The changes buffer of the active ECSPool.
			*//******************************************************************/
			static CompChangesBuffer& ChangesBuffer();

			/*****************************************************************//*!
			\brief
				Gets the Systems map of the active ECSPool.
			\return
				The Systems map of the active ECSPool.
			*//******************************************************************/
			static SystemsManager& Systems();

			/*****************************************************************//*!
			\brief
				Gets the Typemeta manager of the active ECSPool.
			\return
				The Typemeta manager of the active ECSPool.
			*//******************************************************************/
			static TypeMetaManager& TypeMeta();

			/*****************************************************************//*!
			\brief
				Gets the CompArr map of the specified ECSPool.
			\return
				The CompArr map of the specified ECSPool.
			*//******************************************************************/
			static CompArrMapType& Comps(int id);

			/*****************************************************************//*!
			\brief
				Gets the Entities map of the specified ECSPool.
			\return
				The Entities map of the specified ECSPool.
			*//******************************************************************/
			static EntityMapWrapper& Entities(int id);

			/*****************************************************************//*!
			\brief
				Gets the changes buffer of the specified ECSPool.
			\return
				The changes buffer of the specified ECSPool.
			*//******************************************************************/
			static CompChangesBuffer& ChangesBuffer(int id);

			/*****************************************************************//*!
			\brief
				Gets the Systems map of the specified ECSPool.
			\return
				The Systems map of the specified ECSPool.
			*//******************************************************************/
			static SystemsManager& Systems(int id);

			/*****************************************************************//*!
			\brief
				Switches the active ECSPool to the specified ECSPool.
				Does nothing if the specified pool doesn't exist.
			\param id
				The id of the ECSPool.
			*//******************************************************************/
			static void SwitchToPool(int id);

			/*****************************************************************//*!
			\brief
				Switches the active ECSPool to the specified ECSPool.
				Creates the specified pool if it doesn't exist.
			\param id
				The id of the ECSPool.
			\param compCallbacksEnabled
				Whether component callbacks are enabled in the pool.
			*//******************************************************************/
			static void SwitchToPool_CreateIfNotExist(int id, bool compCallbacksEnabled);

			/*****************************************************************//*!
			\brief
				Gets whether the current ECSPool has component callbacks enabled.
			\return
				True if component callbacks are enabled for the current pool. False
				otherwise.
			*//******************************************************************/
			static bool HasCompCallbacksEnabled();

			/*****************************************************************//*!
			\brief
				Deletes the specified ECSPool.
			\param id
				The id of the ECSPool.
			*//******************************************************************/
			static void DeletePool(int id);

			/*****************************************************************//*!
			\brief
				Gets an iterator to the first entity.
			\return
				An iterator to the first entity.
			*//******************************************************************/
			template <typename EntityHandleType>
			static ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> Entity_User_Begin();

			/*****************************************************************//*!
			\brief
				Gets an iterator to 1 past the last entity.
			\return
				An iterator to the 1 past the last entity.
			*//******************************************************************/
			template <typename EntityHandleType>
			static ECSPool::Entity_IteratorBlueprint<EntityHandleType, EntMapType::iterator> Entity_User_End();

		private:
			/*****************************************************************//*!
			\brief
				Deletes the ECSPool pointed to by the specified pool iterator.
			\param poolIter
				The iterator to the ECSPool to be deleted.
			\return
				An iterator to the next ECSPool in the container after the deletion.
			*//******************************************************************/
			static PoolsMapType::iterator DeletePool(const PoolsMapType::iterator& poolIter);

			//! The active ECSPool that all operations will affect.
			static ECSPool* activePool;
			//! A map of all ECSPools.
			static std::unique_ptr<PoolsMapType> pools;
		};
		
#pragma endregion // Pools

}


	/* INTERNAL GLOBAL FUNCTIONS */

	namespace internal {

		/*****************************************************************//*!
		\brief
			Gets the hash of a component type.
		\tparam T
			The component type.
		\return
			The hash of the component type.
		*//******************************************************************/
		template <typename T>
		constexpr CompHash GetCompHash();

		/*****************************************************************//*!
		\brief
			Gets the hash of a system type.
		\tparam T
			The system type.
		\return
			The hash of the system type.
		*//******************************************************************/
		template <typename T>
		constexpr SysHash GetSysHash();


		/*****************************************************************//*!
		\brief
			Calls the copy constructor of the specified component type to copy a component to a specified memory location.
			This is used by CompArr to manage stored component data.
		\tparam T
			The component type whose copy constructor is to be called.
		\param comp
			The source data.
		\param dest
			The destination where the copied component will be constructed at.
		*//******************************************************************/
		template <typename T>
		void ComponentCopyMethod(RawData* comp, RawData* dest);

		/*****************************************************************//*!
		\brief
			Calls the move constructor of the specified component type to move a component to a specified memory location.
			This is used by CompArr to manage stored component data.
		\tparam T
			The component type whose move constructor is to be called.
		\param comp
			The source data.
		\param dest
			The destination where the source component will be moved to.
		*//******************************************************************/
		template <typename T>
		void ComponentMoveMethod(RawData* comp, RawData* dest);

		/*****************************************************************//*!
		\brief
			Calls the destructor of the specified component type to destroy a component at a specified memory location.
			This is used by CompArr to manage stored component data.
		\tparam T
			The component type whose destructor is to be called.
		\param comp
			The component that will be destroyed.
		*//******************************************************************/
		template <typename T>
		void ComponentDestructorMethod(RawData* comp);

		/*****************************************************************//*!
		\brief
			If the specified component type implements IComponentCallbacks, calls the OnAttached() function.
			This is used by CompArr to inform components of events.
		\tparam T
			The component type.
		\param entity
			The entity holding the component.
		*//******************************************************************/
		template <typename T>
		void ComponentInformAttachedMethod(InternalEntityHandle entity);

		/*****************************************************************//*!
		\brief
			If the specified component type implements IComponentCallbacks, calls the OnDetached() function.
			This is used by CompArr to inform components of events.
		\tparam T
			The component type.
		\param entity
			The entity holding the component.
		*//******************************************************************/
		template <typename T>
		void ComponentInformDetachedMethod(InternalEntityHandle entity);


		/*****************************************************************//*!
		\brief
			Gets a CompArr from the default pool via specifying the component type.
			Creates a CompArr that stores the component type if it does not exist in the default pool.
		\tparam T
			The component type that the CompArr stores.
		\return
			A reference to the CompArr that stores the specified component type.
		*//******************************************************************/
		template <typename T>
		CompArr& GetCompArr();

		/*****************************************************************//*!
		\brief
			Gets a CompArr from the specified pool via specifying the component type.
			Creates a CompArr that stores the component type if it does not exist in the specified pool.
		\tparam T
			The component type that the CompArr stores.
		\tparam DoComponentCallbacks
			Whether this component array should inform components of events.
		\param compArrPool
			The pool of CompArr to search in.
		\return
			A reference to the CompArr that stores the specified component type.
		*//******************************************************************/
		template <typename T, bool DoComponentCallbacks = true>
		CompArr& GetCompArr(CompArrMapType& compArrPool);

		/*****************************************************************//*!
		\brief
			Gets a CompArr from the default pool via specifying the component type's hash.
		\param compType
			The hash of the component type that the requested CompArr contains.
		\return
			A reference to the CompArr that stores the requested component type.
		\throws CompArrNotFoundException
			Thrown if the requested CompArr does not exist in the default pool.
		*//******************************************************************/
		CompArr& GetCompArr(CompHash compType);

		/*****************************************************************//*!
		\brief
			Gets a CompArr from the specified pool via specifying the component type's hash.
		\param compArrPool
			The pool of CompArr to search in.
		\param compType
			The hash of the component type that the requested CompArr contains.
		\return
			A reference to the CompArr that stores the requested component type.
		\throws CompArrNotFoundException
			Thrown if the requested CompArr does not exist in the specified pool.
		*//******************************************************************/
		CompArr& GetCompArr(CompArrMapType& compArrPool, CompHash compType);

		/*****************************************************************//*!
		\brief
			Gets a CompArr from the specified pool that stores the same type of components as
			the provided CompArr.
		\param compArrPool
			The pool of CompArr to search in.
		\param refCompArr
			An existing CompArr that stores the same component type. This will be used to create
			a CompArr copy within the pool if it does not exist.
		\return
			A reference to the CompArr that stores the requested component type.
		*//******************************************************************/
		CompArr& GetCompArr(CompArrMapType& compArrPool, const CompArr& refCompArr);


		/*****************************************************************//*!
		\brief
			Out of the specified component types, gets the CompArr with the least number of components.
		\tparam Args
			CompArrs that store the specified component types will be searched.
		\return
			A pointer to the CompArr that currently stores the least number of components, out of the specified component types.
			If no component types are specified, returns nullptr.
		*//******************************************************************/
		template <typename ...Args>
		CompArr* GetCompArrWithLeastComps();

		/*****************************************************************//*!
		\brief
			Helper function for the above function, to replace a CompArr pointer if the requested CompArr has fewer components.
		\tparam CompType
			The CompArr with this specified component type will be tested.
		\param compsCount
			A pointer to the integer that tracks the number of components that arrToReplace contains.
			If the CompArr that stores CompType has fewer components than arrToReplace, this integer will be overriden.
		\param arrToReplace
			A pointer to the CompArr that is the candidate for the least stored components.
			If the CompArr that stores CompType has fewer components than arrToReplace, this CompArr will be overriden.
		*//******************************************************************/
		template <typename CompType>
		void ReplaceIfFewerComps(uint32_t* compsCount, CompArr** arrToReplace);

		/*****************************************************************//*!
		\brief
			Out of the specified CompArrs, gets the CompArr with the least number of components.
		\tparam NumCompArrs
			The number of CompArrs in the array of CompArrs to search through.
		\param compArrs
			The CompArrs to search through.
		\return
			A pointer to the CompArr that currently stores the least number of components, out of the specified CompArrs.
			If no CompArrs are specified, returns nullptr.
		*//******************************************************************/
		template <size_t NumCompArrs>
		CompArr* GetCompArrWithLeastComps(const std::array<CompArr*, NumCompArrs>& compArrs);


		/*****************************************************************//*!
		\brief
			Gets the entity from the component address.
		\param compAddr
			The address of the component.
		\return
			The entity that the component is attached to.
		*//******************************************************************/
		InternalEntityHandle GetEntityFromCompAddr(void* compAddr);

		/*****************************************************************//*!
		\brief
			Gets the entity from the component address.
		\param compAddr
			The address of the component.
		\return
			The entity that the component is attached to.
		*//******************************************************************/
		ConstInternalEntityHandle GetEntityFromCompAddr(const void* compAddr);

		/*****************************************************************//*!
		\brief
			Gets the CompArr containing a component from the component address.
		\param compAddr
			The address of the component.
		\return
			The CompArr that the component is stored within.
		*//******************************************************************/
		CompArr* GetCompArrFromCompAddr(void* compAddr);

		/*****************************************************************//*!
		\brief
			Gets the CompArr containing a component from the component address.
		\param compAddr
			The address of the component.
		\return
			The CompArr that the component is stored within.
		*//******************************************************************/
		const CompArr* GetCompArrFromCompAddr(const void* compAddr);


		/*****************************************************************//*!
		\brief
			Gets an unused EntityHash in an EntMapType pool.
		\param entityPool
			The pool of entities that are currently instanced.
		\return
			An unused EntityHash.
		*//******************************************************************/
		EntityHash GetUnusedEntityHash(const EntMapType& entityPool);

	}
}

#include "ECSInternal.ipp"
