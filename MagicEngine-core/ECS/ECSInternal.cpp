/******************************************************************************/
/*!
\file   ECSInternal.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements non-template classes and functions declared
  in the internal interface header file for the Entity Component System (ECS).

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ECSInternal.h"

namespace ecs {
	namespace internal {
		ECSPool* CurrentPool::activePool;
		std::unique_ptr<PoolsMapType> CurrentPool::pools;

#pragma region ECSPool

		ECSPool::ECSPool(int id, bool compCallbacksEnabled)
			: id{ id }
			, compCallbacksEnabled{ compCallbacksEnabled }
			, entitiesWrapper{ entities, validEntityHandles }
		{
		}

		bool ECSPool::GetIsCompCallbacksEnabled() const
		{
			return compCallbacksEnabled;
		}

#pragma endregion // ECSPool

#pragma region CurrentPool

		void CurrentPool::Init(bool defaultPoolHasCompCallbacksEnabled)
		{
			pools = std::make_unique<PoolsMapType>();

			// Create a default pool with id 0 and set it as active.
			SwitchToPool_CreateIfNotExist(0, defaultPoolHasCompCallbacksEnabled);
		}

		void CurrentPool::Shutdown()
		{
			// Delete all pools
			PoolsMapType::iterator poolIter{ pools->begin() };
			while (poolIter != pools->end())
				poolIter = DeletePool(poolIter);
			// activePool is set to nullptr for us by DeletePool().

			// Clear metadata
			ST<TypeMetaManager>::Destroy();
		}

		int CurrentPool::GetId()
		{
			return activePool->id;
		}

		CompArrMapType& CurrentPool::Comps()
		{
			return activePool->comps;
		}
		EntityMapWrapper& CurrentPool::Entities()
		{
			return activePool->entitiesWrapper;
		}
		CompChangesBuffer& CurrentPool::ChangesBuffer()
		{
			return activePool->changesBuffer;
		}
		SystemsManager& CurrentPool::Systems()
		{
			return activePool->systems;
		}
		TypeMetaManager& CurrentPool::TypeMeta()
		{
			return *ST<TypeMetaManager>::Get();
		}

		CompArrMapType& CurrentPool::Comps(int id)
		{
			return pools->at(id).comps;
		}
		EntityMapWrapper& CurrentPool::Entities(int id)
		{
			return pools->at(id).entitiesWrapper;
		}
		CompChangesBuffer& CurrentPool::ChangesBuffer(int id)
		{
			return pools->at(id).changesBuffer;
		}
		SystemsManager& CurrentPool::Systems(int id)
		{
			return pools->at(id).systems;
		}

		void CurrentPool::SwitchToPool(int id)
		{
			if (activePool && activePool->id == id)
				return;

			PoolsMapType::iterator poolIter{ pools->find(id) };
			if (poolIter != pools->end())
				activePool = &poolIter->second;
		}

		void CurrentPool::SwitchToPool_CreateIfNotExist(int id, bool compCallbacksEnabled)
		{
			if (activePool && activePool->id == id)
				return;

			PoolsMapType::iterator poolIter{ pools->find(id) };
			if (poolIter == pools->end())
				poolIter = pools->try_emplace(id, id, compCallbacksEnabled).first;

			activePool = &poolIter->second;
		}

		bool CurrentPool::HasCompCallbacksEnabled()
		{
			return activePool->GetIsCompCallbacksEnabled();
		}

		void CurrentPool::DeletePool(int id)
		{
			// Get the pool
			PoolsMapType::iterator poolIter{ pools->find(id) };
			if (poolIter == pools->end())
				return;

			DeletePool(poolIter);
		}

		PoolsMapType::iterator CurrentPool::DeletePool(const PoolsMapType::iterator& poolIter)
		{
			// Switch active pool to the pool to be deleted so the state is as expected for the remove operations.
			int currentPoolId{ activePool ? activePool->id : -1 };
			SwitchToPool(poolIter->first);

			ECSPool& poolToDelete{ poolIter->second };

			// Clear systems
			// (since systems generally expect the whole ecs system to still be in a good state, this needs to happen first)
			poolToDelete.systems.RemoveAllSystems();

			// Empty the buffer
			poolToDelete.changesBuffer.ClearAndReset();

			// Clear the components
			// (this needs to happen before entities clear in case component destructors try calling GetEntity())
			for (auto& [_, compArr] : poolToDelete.comps)
				compArr.RemoveAllComps();
			poolToDelete.comps.clear();
			// note: this sequence breaks down if components' destructors add components. hopefully we'll never encounter such a situation.

			// Clear the entities
			poolToDelete.entitiesWrapper.ClearAll();

			// Switch back to initial pool
			if (currentPoolId >= 0)
				SwitchToPool(currentPoolId);
			// If the requested pool to be deleted is the active pool, set the active pool to the default pool (0).
			if (activePool == &poolIter->second)
			{
				// The default pool might not exist when we're shutting down.
				activePool = (pools->find(0) != pools->end() ? &pools->at(0) : nullptr);

				// Below is the previous implementation that chooses the next pool after the active pool.

				//PoolsMapType::iterator newActivePoolIter{ pools->begin() };

				//// If the first pool we find is again the pool to be deleted, try the next pool in line.
				//if (newActivePoolIter == poolIter)
				//	++newActivePoolIter;

				//// If the next pool is the end of the map, then there will be no pools left after the deletion. Set activePool to nullptr in that case.
				//if (newActivePoolIter == pools->end())
				//	activePool = nullptr;
				//else
				//	activePool = &newActivePoolIter->second;
			}

			// Remove the pool from the pools container.
			return pools->erase(poolIter);
		}

#pragma endregion // CurrentPool

#pragma region Entity

		Entity_Internal::Entity_Internal(EntityHash mapKey)
			: mapKey{ mapKey }
			, transform{}
			, isMarkedForDeletion{ false }
		{
		}

		Entity_Internal::Entity_Internal(EntityHash mapKey, InternalEntityHandle parentEntity)
			: Entity_Internal{ mapKey }
		{
			if (parentEntity)
				transform.SetParent(parentEntity->transform);
		}

		Entity_Internal::Entity_Internal(EntityHash mapKey, Transform& transformCopy)
			: mapKey{ mapKey }
			, transform{ transformCopy }
			, isMarkedForDeletion{ false }
		{
		}

		void Entity_Internal::INTERNAL_CloneCompsToEntity(InternalEntityHandle entity) const
		{
			if (components.empty())
				return;

			for (EntCompMapType::const_iterator compIter{ components.begin() }, compEnd{ components.end() }; compIter != compEnd; ++compIter)
			{
				// Only clone active components attached to this entity
				if (compIter->second & COMP_STATUS_ANY)
					continue;

				// Clone a copy of this component into the component addition buffer
				uint32_t indexInBuffer{ CurrentPool::ChangesBuffer().CloneComp(GetCompArr(compIter->first), compIter->second, entity) };

				// Register the component to the entity
				entity->components.emplace(compIter->first, indexInBuffer | COMP_STATUS_TO_ADD);
			}
		}

		void Entity_Internal::INTERNAL_CloneCompsToEntityNow(InternalEntityHandle entity) const
		{
			INTERNAL_CloneCompsToEntityNow(entity, CurrentPool::Comps(), CurrentPool::Comps());
		}
		void Entity_Internal::INTERNAL_CloneCompsToEntityNow(InternalEntityHandle entity, CompArrMapType& srcCompArrMap, CompArrMapType& destCompArrMap) const
		{
			if (components.empty())
				return;

			bool srcAndDestCompArrIsSame{ &srcCompArrMap == &destCompArrMap };
			for (EntCompMapType::const_iterator compIter{ components.begin() }, compEnd{ components.end() }; compIter != compEnd; ++compIter)
			{
				// Only clone active components attached to this entity
				if (compIter->second & COMP_STATUS_ANY)
					continue;

				// Request CompArr to create a copy of this component, and add it to the provided pool immediately
				CompArr& srcCompArr{ GetCompArr(srcCompArrMap, compIter->first) };
				CompArr& destCompArr{ (srcAndDestCompArrIsSame ? srcCompArr : GetCompArr(destCompArrMap, srcCompArr)) };
				uint32_t compIndex{ srcCompArr.CloneComp(compIter->second, entity, destCompArr) };

				// Register the component to the entity
				entity->components.emplace(compIter->first, compIndex);
			}
		}

		void Entity_Internal::INTERNAL_ChangeCompIndex(CompHash compHash, uint32_t newIndex, bool overrideFlags)
		{
			// Copy flags into the value to be set
			EntCompMapType::iterator compIter{ components.find(compHash) };
			if (!overrideFlags)
				newIndex = (compIter->second & COMP_STATUS_ANY) + (newIndex & COMP_STATUS_UNUSED_BITS);

			// Set the value of the new index
			compIter->second = newIndex;
		}

		void Entity_Internal::INTERNAL_RemoveComp(CompHash compHash)
		{
			components.erase(compHash);
		}

		EntCompMapType::const_iterator Entity_Internal::INTERNAL_CompsBegin() const
		{
			return components.cbegin();
		}

		EntCompMapType::const_iterator Entity_Internal::INTERNAL_CompsEnd() const
		{
			return components.cend();
		}

		void Entity_Internal::INTERNAL_MarkForDeletion()
		{
			isMarkedForDeletion = true;
		}

		bool Entity_Internal::INTERNAL_GetIsMarkedForDeletion() const
		{
			return isMarkedForDeletion;
		}

		void Entity_Internal::INTERNAL_MarkAllCompsRemoved()
		{
			for (auto& [_, index] : components)
				index |= COMP_STATUS_TO_REMOVE;
		}

		EntityHash Entity_Internal::INTERNAL_GetMapKey() const
		{
			return mapKey;
		}

		bool Entity_Internal::INTERNAL_GetHasComp(CompHash compHash) const
		{
			return components.find(compHash) != components.end();
		}

		uint32_t Entity_Internal::INTERNAL_GetCompIndex(CompHash compHash) const
		{
			internal::EntCompMapType::const_iterator compIndexIter{ components.find(compHash) };
			if (compIndexIter == components.end())
				// This component type does not exist on this entity
				return std::numeric_limits<uint32_t>::max();

			// Ensure the component is attached to us
			if (compIndexIter->second & COMP_STATUS_TO_ADD)
				return std::numeric_limits<uint32_t>::max();

			return compIndexIter->second & COMP_STATUS_UNUSED_BITS;
		}

		RawData* Entity_Internal::INTERNAL_GetCompRaw(CompHash compHash) const
		{
			uint32_t compIndex{ INTERNAL_GetCompIndex(compHash) };
			if (compIndex == std::numeric_limits<uint32_t>::max())
				return nullptr;

			try {
				internal::CompArr& compArr{ internal::GetCompArr(compHash) };
				return compArr.GetComp(compIndex);
			}
			// If this component type has never been added to ecs, we'll get this exception.
			catch (const CompArrNotFoundException&) {
				return nullptr;
			}
		}

		bool Entity_Internal::CheckCanAddComp(CompHash compHash)
		{
			// Disallow adding components if this entity is going to be deleted
			if (isMarkedForDeletion)
				return false;

			// Disallow adding multiple components of the same type to 1 entity
			// note: this will disallow attaching components in the same frame that they have been removed.
			//       will consider to fix later if this becomes an issue.
			if (components.find(compHash) != components.end())
				return false;

			return true;
		}

		bool Entity_Internal::CheckCanRemoveComp(CompHash compHash, EntCompMapType::iterator& outCompIndexIter)
		{
			// Disallow removing components if this entity is going to be deleted
			if (isMarkedForDeletion)
				return false;

			// Check whether there exists a component of this type on this entity
			outCompIndexIter = components.find(compHash);
			if (outCompIndexIter == components.end())
				return false;

			// Check whether this component is already marked for deletion
			if (outCompIndexIter->second & COMP_STATUS_TO_REMOVE)
				return false;

			return true;
		}

#pragma endregion // Entity

#pragma region CompChangesBuffer

		CompModifyTask::CompModifyTask(InternalEntityHandle entity, TYPE type)
			: entity{ entity }
			, type{ type }
		{
		}

		uint32_t CompModifyTask::GetCompIndex(CompHash compHash) const
		{
			return entity->INTERNAL_GetCompIndex(compHash);
		}

		CompModifyTask::TYPE CompModifyTask::GetType() const
		{
			return type;
		}

		void CompModifyTask::SetType(TYPE newType)
		{
			type = newType;
		}

		bool CompModifyTask::operator<(const CompModifyTask& other) const
		{
			return entity < other.entity;
		}

		void CompChangesBuffer::RemoveComp(InternalEntityHandle entity, CompHash compType)
		{
			auto emplaceResult{ GetModifySet(compType).emplace(entity, CompModifyTask::TYPE::REMOVE) };
			if (!emplaceResult.second)
			{
				// A task pointing to the same component already exists. Overwrite its type.
				// note: for some reason, the iterator is const. gonna need to hack it a bit...
				emplaceResult.first._Ptr->_Myval.SetType(CompModifyTask::TYPE::REMOVE);
			}
		}

		void CompChangesBuffer::RemoveCompBufferedForAddition(CompHash compType, uint32_t index)
		{
			GetCompArr(compsToAdd, compType).RemoveComp(index, false);
		}

		void CompChangesBuffer::ChangeCompActiveness(InternalEntityHandle entity, CompHash compType, bool isInactive)
		{
			// Check if this component is already queued for modification. If so, we can delete the task.
			CompModifyTask task{ entity, (isInactive ? CompModifyTask::TYPE::SET_INACTIVE : CompModifyTask::TYPE::SET_ACTIVE) };
			auto& modifySet{ GetModifySet(compType) };
			auto existingTaskIter{ modifySet.find(task) };
			if (existingTaskIter != modifySet.end())
				switch (existingTaskIter->GetType())
				{
				case CompModifyTask::TYPE::REMOVE:
					// Setting comp activeness on a comp that's about to be removed has no effect.
					return;
				case CompModifyTask::TYPE::SET_ACTIVE:
					if (isInactive)
						modifySet.erase(existingTaskIter);
					return;
				case CompModifyTask::TYPE::SET_INACTIVE:
					if (!isInactive)
						modifySet.erase(existingTaskIter);
					return;
				default:
					assert(false);
				}

			// Check for no-op
			if (GetCompArr(compType).GetIsCompActive(entity->INTERNAL_GetCompIndex(compType)) == !isInactive)
				return;

			// Add task
			GetModifySet(compType).insert(std::move(task));
		}

		uint32_t CompChangesBuffer::CloneComp(CompArr& srcArr, uint32_t index, InternalEntityHandle entityOwner)
		{
			// Get the destination CompArr
			CompArr* destArr{};
			try {
				destArr = &GetCompArr(compsToAdd, srcArr.GetCompHash());
			}
			// Catch instances where the CompArr for this component type does not exist in our compsToAdd pool yet
			catch (const CompArrNotFoundException&)
			{
				// Clone our version of this CompArr which should be identical to as if it was constructed via type information,
				// and insert into our compsToAdd pool
				std::pair<CompArrMapType::iterator, bool> insertResult{ srcArr.CloneWithoutCompDataIntoPool(compsToAdd) };
				destArr = &insertResult.first->second;
			}

			// Request srcArr to create a copy of this component, and add it to the destArr
			return srcArr.CloneComp(index, entityOwner, *destArr);
		}

		void CompChangesBuffer::DeleteEntity(InternalEntityHandle entity)
		{
			entity->INTERNAL_MarkForDeletion();
			entitiesToRemove.push_back(entity);

			// Remove all components pending to be added from the buffer, and buffer all components on this entity for removal
			for (EntCompMapType::const_iterator compIter{ entity->INTERNAL_CompsBegin() }, compEnd{ entity->INTERNAL_CompsEnd() }; compIter != compEnd; ++compIter)
			{
				// Remove components pending addition
				if (compIter->second & Entity_Internal::COMP_STATUS_TO_ADD)
					RemoveCompBufferedForAddition(compIter->first, compIter->second & Entity_Internal::COMP_STATUS_UNUSED_BITS);
				// Buffer removal of components already attached to entity
				else if (!(compIter->second & Entity_Internal::COMP_STATUS_TO_REMOVE))
				{
					auto& modifySet{ GetModifySet(compIter->first) };
					auto emplaceResult{ modifySet.emplace(entity, CompModifyTask::TYPE::REMOVE) };
					// The emplacement may have failed if there is already a task to act on the component. In this case just change the task to removal.
					if (!emplaceResult.second)
					{
						// std::set doesn't allow modification of existing items...
						CompModifyTask task{ *emplaceResult.first };
						task.SetType(CompModifyTask::TYPE::REMOVE);
						modifySet.erase(emplaceResult.first);
						modifySet.insert(std::move(task));
					}
				}
			}
			// In case components' destructors call GetComp() looking for another component
			entity->INTERNAL_MarkAllCompsRemoved();
		}

		void CompChangesBuffer::AddComponentCallback(CompInformAttachedSig callback, InternalEntityHandle entity)
		{
			componentCallbacksQueue.emplace_back(callback, entity);
		}

		void CompChangesBuffer::FlushChanges()
		{
			// Modify/Remove components
			if (!compsToModify.empty())
			{
				for (ModifyCompSetType::iterator compSetIter{ compsToModify.begin() }, compSetEnd{ compsToModify.end() }; compSetIter != compSetEnd; ++compSetIter)
				{
					CompArr& compArr{ GetCompArr(compSetIter->first) };
					for (CompIndexSetType<CompModifyTask>::iterator taskIter{ compSetIter->second.begin() }, taskEnd{ compSetIter->second.end() };
						taskIter != taskEnd; ++taskIter)
					{
						uint32_t compIndex{ taskIter->GetCompIndex(compSetIter->first) };
						switch (taskIter->GetType())
						{
						case CompModifyTask::TYPE::REMOVE:
							// note: we can optimize the number of moves if the components to be removed are sequential.
							//       this can be considered if performance is an issue here
							compArr.RemoveComp(compIndex);
							break;
						case CompModifyTask::TYPE::SET_ACTIVE:
							compArr.SetCompActiveness(compIndex, false);
							break;
						case CompModifyTask::TYPE::SET_INACTIVE:
							compArr.SetCompActiveness(compIndex, true);
							break;
						default:
							assert(false);
						}
					}
				}

				// TODO: this may be very expensive if we're making changes in a majority of frames.
				//		 would be better to keep the memory of all CompArr loaded and keep a list of component types that are pending addition
				compsToModify.clear();
			}

			// Delete entities
			if (!entitiesToRemove.empty())
			{
				for (const InternalEntityHandle& entity : entitiesToRemove)
					CurrentPool::Entities().EraseEntity(entity->INTERNAL_GetMapKey());
				entitiesToRemove.clear();
			}

			// Add components
			if (!compsToAdd.empty())
			{
				for (CompArrMapType::iterator compArrIter{ compsToAdd.begin() }, compArrEnd{ compsToAdd.end() }; compArrIter != compArrEnd; ++compArrIter)
				{
					try {
						GetCompArr(compArrIter->first).TransferCompsFrom(compArrIter->second);
					}
					// Catch instances where the CompArr for this component type does not exist in the default pool yet
					catch (const CompArrNotFoundException&)
					{
						// Clone our version of this CompArr which should be identical to as if it was constructed via type information,
						// and insert into the default pool
						std::pair<CompArrMapType::iterator, bool> insertResult{ compArrIter->second.CloneWithoutCompDataIntoPool(CurrentPool::Comps()) };
						// Now we can transfer comps as usual. This should not throw.
						insertResult.first->second.TransferCompsFrom(compArrIter->second);
					}
				}

				// TODO: this may be very expensive if we're making changes in a majority of frames.
				//		 would be better to keep the memory of all CompArr loaded and keep a list of component types that are pending addition
				compsToAdd.clear();
			}

			FlushComponentCallbacks();
		}

		void CompChangesBuffer::FlushComponentCallbacks()
		{
			if (componentCallbacksQueue.empty())
				return;

			for (auto& callbackPair : componentCallbacksQueue)
				callbackPair.first(callbackPair.second);
			componentCallbacksQueue.clear();
		}

		void CompChangesBuffer::ClearAndReset()
		{
			// Clear add components
			if (!compsToAdd.empty())
			{
				// Need to call the destructor on these components.
				for (CompArrMapType::iterator compSetIter{ compsToAdd.begin() }, compSetEnd{ compsToAdd.end() }; compSetIter != compSetEnd; ++compSetIter)
					compSetIter->second.RemoveAllComps();
				compsToAdd.clear();
			}
			// Clear remove components
			compsToModify.clear();
			// Clear remove entities
			entitiesToRemove.clear();
		}

		CompIndexSetType<CompModifyTask>& CompChangesBuffer::GetModifySet(CompHash compType)
		{
			ModifyCompSetType::iterator removeSetIter{ compsToModify.find(compType) };
			if (removeSetIter != compsToModify.end())
				return removeSetIter->second;
			else
			{
				std::pair<ModifyCompSetType::iterator, bool> insertIter{ compsToModify.try_emplace(compType) };
				return insertIter.first->second;
			}
		}

#pragma endregion // CompChangesBuffer

#pragma region CompArr

		CompArr::CompArr(CompHash compHash, uint32_t compSize, CompCopySig copyFunc, CompMoveSig moveFunc, CompDestroySig destroyFunc,
			CompInformAttachedSig informAttachedFunc, CompInformDetachedSig informDetachedFunc,
			CompInformAttachedSig trueInformAttachedFunc, CompInformDetachedSig trueInformDetachedFunc)
			: compHash{ compHash }
			, compSize{ compSize }
			, compStepSize{ ReservedBytes + compSize }
			, numInactive{}
			, firstActiveIndex{ numInactive }
			, callCopyFunc{ copyFunc }
			, callMoveFunc{ moveFunc }
			, callDestructorFunc{ destroyFunc }
			, callInformAttachedFunc{ informAttachedFunc }
			, callInformDetachedFunc{ informDetachedFunc }
			, trueInformAttachedFunc{ trueInformAttachedFunc }
			, trueInformDetachedFunc{ trueInformDetachedFunc }
			, tempCompSpace{ new RawData[compSize] }
		{
			// Reserve space for 32 components of this type
			arrRaw.reserve(32 * static_cast<size_t>(compStepSize));
		}

		CompArr::~CompArr()
		{
			RemoveAllComps();
		}

		CompArr& GetCompArr(CompHash compType)
		{
			return GetCompArr(CurrentPool::Comps(), compType);
		}
		CompArr& GetCompArr(CompArrMapType& compArrPool, CompHash compType)
		{
			auto iter = compArrPool.find(compType);
#ifdef _DEBUG
			if (iter == compArrPool.end())
				// We can't construct it here since we don't have the type information. Our only option is throwing
				throw CompArrNotFoundException{};
#endif // _DEBUG
			return iter->second;
		}

		CompArr& GetCompArr(CompArrMapType& compArrPool, const CompArr& refCompArr)
		{
			auto iter = compArrPool.find(refCompArr.GetCompHash());
			if (iter == compArrPool.end())
				return refCompArr.CloneWithoutCompDataIntoPool(compArrPool).first->second;
			else
				return iter->second;
		}

		uint32_t CompArr::AddComp(InternalEntityHandle entPtr, RawData* comp, bool isInactive)
		{
			// Expand array
			uint32_t compIndex{ ExpandArrayForComp(isInactive) };

			// Set entity ptr
			SetEntityPtr(compIndex, entPtr);

			// Move the component into the expanded memory
			size_t insertIndex{ compIndex * compStepSize + ReservedBytes };
			callMoveFunc(comp, arrRaw.data() + insertIndex);
			// Destructor is not called as we assume this component came from outside,
			// where the original object will be destroyed by going out of scope.

			// Inform component of attach event
			CurrentPool::ChangesBuffer().AddComponentCallback(callInformAttachedFunc, entPtr);

			return compIndex;
		}

		void CompArr::RemoveComp(uint32_t index, bool informOwnerEntity)
		{
			// Inform component of detach event
			callInformDetachedFunc(GetEntity(index));

			// Destroy the component
			if (informOwnerEntity)
				GetEntity(index)->INTERNAL_RemoveComp(compHash);
			callDestructorFunc(GetComp(index));

			// If this component was hidden and was not the last hidden component, we need to move the last hidden component first.
			if (index + 1 < firstActiveIndex)
			{
				MoveComp(firstActiveIndex - 1, index);
				index = firstActiveIndex - 1;
			}
			// If this component was hidden, we need to shift the first active index.
			if (index < numInactive)
				--numInactive;

			// Now if there are active components, we need to move the last active component to fill its place.
			uint32_t lastIndex{ GetNumComps() - 1 };
			if (index != lastIndex)
				MoveComp(lastIndex, index);

			// Remove component and the entity key
			SetArraySize(lastIndex);
		}

		uint32_t CompArr::CloneComp(uint32_t index, InternalEntityHandle entityOwner, CompArr& destArr)
		{
			// TODO: Throw something if the other CompArr doesn't store the same type of components as us

			// Expand array
			uint32_t compIndex{ destArr.ExpandArrayForComp(index < numInactive) };

			// Set entity ptr
			destArr.SetEntityPtr(compIndex, entityOwner);

			// Copy the component into the expanded memory
			callCopyFunc(GetComp(index), destArr.GetComp(compIndex));

			// Inform component of attach event
			CurrentPool::ChangesBuffer().AddComponentCallback(destArr.callInformAttachedFunc, entityOwner);

			return compIndex;
		}

		void CompArr::SetCompActiveness(void* compAddr, bool setInactive)
		{
			auto byteOffset{ reinterpret_cast<RawData*>(compAddr) - arrRaw.data() };
			SetCompActiveness(static_cast<uint32_t>(byteOffset / compStepSize), setInactive);
		}


		void CompArr::SetCompActiveness(uint32_t index, bool setInactive)
		{
#ifdef _DEBUG
			assert(!( // We're doing a useless operation moving inactive -> inactive, or active -> active...
				(setInactive && index < firstActiveIndex) ||
				(!setInactive && index >= firstActiveIndex))
			);
#endif
			uint32_t destIndex{ (setInactive ? firstActiveIndex++ : firstActiveIndex-- - 1) };
			// If the component we're changing activeness on is at the border, no swapping is needed.
			if (index == destIndex)
				return;

			// We're gonna do a manual swap
			// Move the component to be moved to temp space
			RawData* srcCompLocation{ GetComp(index) }, *destCompLocation{ GetComp(destIndex) };
			InternalEntityHandle entity{ GetEntity(index) }, otherEntity{ GetEntity(destIndex) };
			callMoveFunc(srcCompLocation, tempCompSpace.get());
			callDestructorFunc(srcCompLocation);
			// Move the component in the destination to the original component's location
			callMoveFunc(destCompLocation, srcCompLocation);
			callDestructorFunc(destCompLocation);
			SetEntityPtr(index, otherEntity);
			otherEntity->INTERNAL_ChangeCompIndex(compHash, index);
			// Move the component from the temp space to the destination
			callMoveFunc(tempCompSpace.get(), destCompLocation);
			callDestructorFunc(tempCompSpace.get());
			SetEntityPtr(destIndex, entity);
			entity->INTERNAL_ChangeCompIndex(compHash, destIndex);
		}

		RawData* CompArr::GetComp(uint32_t index)
		{
			return arrRaw.data() + index * compStepSize + ReservedBytes;
		}
		const RawData* CompArr::GetComp(uint32_t index) const
		{
			return arrRaw.data() + index * compStepSize + ReservedBytes;
		}

		void CompArr::TransferCompsFrom(CompArr& other)
		{
			// Similar logic to AddComp(), except we only expand the array once

			// TODO: Throw something if the other CompArr doesn't store the same type of components as us
			
			if (other.arrRaw.empty())
				return;

			// Expand to fit all new components
			auto insertIndexes{ ExpandArrayForComp(other.GetNumComps() - other.numInactive, other.numInactive) };

			// Transfer one at a time from left to right
			uint32_t srcIndex{};
			for (iterator compIter{ other.begin() }, endIter{ other.end() }; compIter != endIter; ++compIter, ++srcIndex)
			{
				// Which index to modify will depend on whether the component we're moving is inactive or active.
				uint32_t destIndex{ (srcIndex < other.numInactive ? insertIndexes.second++ : insertIndexes.first++) };

				// Inform component of detach
				other.callInformDetachedFunc(compIter.GetEntity());

				InternalEntityHandle entity{ compIter.GetEntity() };
				SetEntityPtr(destIndex, entity);
				callMoveFunc(compIter.GetComp(), GetComp(destIndex));
				callDestructorFunc(compIter.GetComp());
				// note: this is assuming that we're transfering components from a buffer (this) to a pool (other).
				entity->INTERNAL_ChangeCompIndex(compHash, destIndex, true);

				// Inform component of attach, immediately since the vector storing components could reallocate
				// TODO: The above reason is now invalid since we've already expanded the vector. This could potentially be deferred? But also need to be careful of behavior...
				callInformAttachedFunc(entity);
			}

			other.arrRaw.clear();
		}

		std::pair<CompArrMapType::iterator, bool> CompArr::CloneWithoutCompDataIntoPool(CompArrMapType& compArrPool) const
		{
			// Ensure the destination comp arr map is in the current pool, otherwise comp callbacks could be incorrect.
			assert(&CurrentPool::Comps() == &compArrPool);

			bool compCallbacksEnabled{ CurrentPool::HasCompCallbacksEnabled() };
			return compArrPool.emplace(
				std::piecewise_construct, // Need to construct in place due to deleted copy/move constructor
				std::make_tuple(compHash),
				std::make_tuple(compHash, compSize, callCopyFunc, callMoveFunc, callDestructorFunc,
				compCallbacksEnabled ? trueInformAttachedFunc : [](InternalEntityHandle) -> void {},
				compCallbacksEnabled ? trueInformDetachedFunc : [](InternalEntityHandle) -> void {},
				trueInformAttachedFunc,
				trueInformDetachedFunc)
			);
		}

		void CompArr::RemoveAllComps()
		{
			// Call destructor on all active components
			for (uint32_t index{}, end{ GetNumComps() }; index < end; ++index)
			{
				InternalEntityHandle entity{ GetEntity(index) };
				callInformDetachedFunc(entity);
				entity->INTERNAL_RemoveComp(compHash);
				callDestructorFunc(GetComp(index));
			}

			arrRaw.clear();
			numInactive = 0;
		}

		CompHash CompArr::GetCompHash() const
		{
			return compHash;
		}

		void CompArr::SetArraySize(uint32_t numComps)
		{
			uint32_t originalNumComps{ GetNumComps() };
			size_t newVecSize{ static_cast<size_t>(compStepSize) * numComps };
			// If no reallocation needs to happen, we can simply resize.
			if (newVecSize <= arrRaw.capacity())
			{
				arrRaw.resize(newVecSize);
				InsertCompArrPtr(originalNumComps, numComps);
				return;
			}

			// Make a new vector so we can have valid source and destination memory for the manual move
			decltype(arrRaw) newVec{};
			newVec.reserve(newVecSize * 2); // Expand to at least twice the requested amount so we don't have to reallocate so often
			newVec.resize(newVecSize); // Make the vector know that we're using this amount of memory

			// Move memory contents from arrRaw to the new vector.
			for (uint32_t index{}, end{ GetNumComps() }; index < end; ++index)
			{
				// Set entity pointer
				SetEntityPtr(index, GetEntity(index), &newVec);
				// Move the component
				RawData* srcComp{ GetComp(index) }, *destComp{ newVec.data() + index * compStepSize + ReservedBytes };
				callMoveFunc(srcComp, destComp);
				callDestructorFunc(srcComp);
			}

			// Set arrRaw to point to the new memory in the new vector
			arrRaw = std::move(newVec);

			// Insert the comp arr pointer for each component
			InsertCompArrPtr(0, numComps);
		}

		void CompArr::InsertCompArrPtr(uint32_t startIndex, uint32_t endIndex)
		{
			const CompArr* thisPtr{ this };
			for (; startIndex < endIndex; ++startIndex)
				std::memcpy(arrRaw.data() + startIndex * compStepSize, &thisPtr, CompArrPtrSize);
		}

		void CompArr::SetEntityPtr(uint32_t index, InternalEntityHandle entPtr)
		{
			SetEntityPtr(index, entPtr, &arrRaw);
		}

		void CompArr::SetEntityPtr(size_t index, InternalEntityHandle entPtr, CompContType* arr)
		{
			std::memcpy(arr->data() + index * compStepSize + CompArrPtrSize, &entPtr, EntPtrSize);
		}

		uint32_t CompArr::ExpandArrayForComp(bool isInactive)
		{
			// Expand array
			uint32_t compIndex{ GetNumComps() };
			SetArraySize(compIndex + 1);

			// If inactive, move the first active component so that this component can take its place.
			if (isInactive)
			{
				// Only move if there exists an active component - if there are no active components, no moving needs to take place
				if (compIndex != firstActiveIndex)
				{
					MoveComp(firstActiveIndex, compIndex);
					compIndex = firstActiveIndex;
				}

				++numInactive;
			}

			return compIndex;
		}

		std::pair<uint32_t, uint32_t> CompArr::ExpandArrayForComp(uint32_t numActiveToAdd, uint32_t numInactiveToAdd)
		{
			// Pack initial indexes
			std::pair<uint32_t, uint32_t> indexes{ GetNumComps(), firstActiveIndex };
			auto& [activeIndex, inactiveIndex] { indexes };
			
			// Expand array
			SetArraySize(activeIndex + numActiveToAdd + numInactiveToAdd);

			// If there are active components, we need to move them to make space for inactive components
			if (firstActiveIndex != activeIndex)
				for (; numInactiveToAdd; --numInactiveToAdd)
					MoveComp(firstActiveIndex++, activeIndex++);

			return indexes;
		}

		void CompArr::MoveComp(uint32_t srcIndex, uint32_t destIndex)
		{
			RawData* srcComp{ GetComp(srcIndex) }, *destComp{ GetComp(destIndex) };
			// Move the component
			callMoveFunc(srcComp, destComp);
			// Destroy the component that is left in the source location.
			callDestructorFunc(srcComp);
			// Move the entity key
			std::memcpy(destComp - EntPtrSize, srcComp - EntPtrSize, EntPtrSize);
			// Inform the entity of the new index
			GetEntity(destIndex)->INTERNAL_ChangeCompIndex(compHash, destIndex);
		}

		InternalEntityHandle CompArr::GetEntity(uint32_t index)
		{
			return *reinterpret_cast<InternalEntityHandle*>(arrRaw.data() + index * compStepSize + CompArrPtrSize);
		}

		uint32_t CompArr::GetNumComps() const
		{
			return static_cast<uint32_t>(arrRaw.size() / compStepSize);
		}

		bool CompArr::GetIsCompActive(const void* compAddr) const
		{
			return GetComp(firstActiveIndex) <= compAddr;
		}

		bool CompArr::GetIsCompActive(uint32_t index) const
		{
			return firstActiveIndex <= index;
		}
		
		CompArr::iterator CompArr::begin()
		{
			// Iterators have no use of the CompArrPtr.
			return iterator{ compStepSize, arrRaw.data() + CompArrPtrSize };
		}
		CompArr::const_iterator CompArr::begin() const
		{
			return const_iterator{ compStepSize, arrRaw.data() + CompArrPtrSize };
		}
		CompArr::iterator CompArr::begin_active()
		{
			return iterator{ compStepSize, arrRaw.data() + firstActiveIndex * compStepSize + CompArrPtrSize };
		}
		CompArr::const_iterator CompArr::begin_active() const
		{
			return const_iterator{ compStepSize, arrRaw.data() + firstActiveIndex * compStepSize + CompArrPtrSize };
		}
		CompArr::iterator CompArr::end()
		{
			return iterator{ compStepSize, arrRaw.data() + arrRaw.size() + CompArrPtrSize };
		}
		CompArr::const_iterator CompArr::end() const
		{
			return const_iterator{ compStepSize, arrRaw.data() + arrRaw.size() + CompArrPtrSize };
		}

#pragma endregion // CompArr

#pragma region SystemsManager

		SystemsManager::~SystemsManager()
		{
			RemoveAllSystems();
		}

		std::vector<InternalGenericSysHandle<System_Internal_Base>> SystemsManager::GetAllSystems()
		{
			std::vector<InternalGenericSysHandle<System_Internal_Base>> allSystems{};
			std::for_each(layerToSystemsMap.begin(), layerToSystemsMap.end(), [&allSystems](auto& layerToSystems) -> void {
				std::transform(layerToSystems.second.begin(), layerToSystems.second.end(), std::back_inserter(allSystems), [](auto& system) -> InternalGenericSysHandle<System_Internal_Base> {
					return system.second;
				});
			});
			return allSystems;
		}

		void SystemsManager::RemoveSystemsInLayer(int layer)
		{
			SysMapType& sysMap{ GetSystemsMap(layer) };
			for (auto& [sysHash, sysPtr] : sysMap)
			{
				sysPtr->OnRemoved();
				delete sysPtr;
				hashToLayerMap.erase(sysHash);
			}
			sysMap.clear();
		}

		void SystemsManager::RemoveAllSystems()
		{
			for (auto& [_, sysMap] : layerToSystemsMap)
				for (auto& [__, sysPtr] : sysMap)
				{
					sysPtr->OnRemoved();
					delete sysPtr;
				}
			hashToLayerMap.clear();
			layerToSystemsMap.clear();
		}

		void SystemsManager::RunSystems(int layer)
		{
			for (auto& [_, sysPtr] : GetSystemsMap(layer))
			{
				if (!sysPtr->PreRun())
					continue;
				sysPtr->Run();
				sysPtr->PostRun();
			}
		}

		SysMapType& SystemsManager::GetSystemsMap(int layer)
		{
			// Creates a map if it doesn't exist
			return layerToSystemsMap[layer];
		}

#pragma endregion // SystemsManager

#pragma region Type Meta

		const CompTypeMeta* TypeMetaManager::GetCompTypeMeta(CompHash hash) const
		{
			CompTypeMetaMap::const_iterator metaIter{ compTypeMetaMap.find(hash) };
			if (metaIter == compTypeMetaMap.end())
				return nullptr;
			return &metaIter->second;
		}

		const SysTypeMeta* TypeMetaManager::GetSysTypeMeta(SysHash hash) const
		{
			SysTypeMetaMap::const_iterator metaIter{ sysTypeMetaMap.find(hash) };
			if (metaIter == sysTypeMetaMap.end())
				return nullptr;
			return &metaIter->second;
		}

		const SysTypeMetaMap& TypeMetaManager::GetAllSysTypeMeta() const
		{
			return sysTypeMetaMap;
		}

		void TypeMetaManager::Clear()
		{
			compTypeMetaMap.clear();
		}

#pragma endregion // Type Meta

#pragma region Pools

		EntityMapWrapper::EntityMapWrapper(EntMapType& entities, ValidEntHandleSetType& validEntityHandles)
			: entities{ entities }
			, validEntityHandles{ validEntityHandles }
		{
		}

		InternalEntityHandle EntityMapWrapper::CreateEntity(InternalEntityHandle parent)
		{
			// Get an unused entity hash as key
			EntityHash key{ GetUnusedEntityHash(entities) };
			// Place into container and return a pointer to the entity
			std::pair<EntMapType::iterator, bool> emplaceResult{ (parent ? entities.try_emplace(key, key, parent) : entities.emplace(key, key)) };
			// Update the list of valid entity handles
			validEntityHandles.insert(&emplaceResult.first->second);
			return &emplaceResult.first->second;
		}

		InternalEntityHandle EntityMapWrapper::CreateEntity(Transform& transformCopy)
		{
			// Get an unused entity hash as key
			EntityHash key{ GetUnusedEntityHash(entities) };
			// Place into container and return a pointer to the entity
			std::pair<EntMapType::iterator, bool> emplaceResult{ entities.try_emplace(key, key, transformCopy) };
			// Update the list of valid entity handles
			validEntityHandles.insert(&emplaceResult.first->second);
			return &emplaceResult.first->second;
		}

		InternalEntityHandle EntityMapWrapper::GetEntity(EntityHash hash)
		{
			auto entityIter{ entities.find(hash) };
			if (entityIter == entities.end())
				return nullptr;
			return &entityIter->second;
		}

		void EntityMapWrapper::EraseEntity(EntityHash hash)
		{
			auto entityIter{ entities.find(hash) };
			if (entityIter == entities.end())
				return;
			validEntityHandles.erase(&entityIter->second);
			entities.erase(entityIter);
		}

		void EntityMapWrapper::ClearAll()
		{
			validEntityHandles.clear();
			entities.clear();
		}

		bool EntityMapWrapper::CheckValidHandle(InternalEntityHandle handle)
		{
			return validEntityHandles.find(handle) != validEntityHandles.end();
		}

#pragma endregion // Pools

		/* INTERNAL GLOBAL FUNCTIONS */

		InternalEntityHandle GetEntityFromCompAddr(void* compAddr)
		{
			return *reinterpret_cast<InternalEntityHandle*>(
				reinterpret_cast<RawData*>(compAddr) - CompArr::EntPtrSize
			);
		}
		ConstInternalEntityHandle GetEntityFromCompAddr(const void* compAddr)
		{
			return *reinterpret_cast<ConstInternalEntityHandle const*>(
				reinterpret_cast<const RawData*>(compAddr) - CompArr::EntPtrSize
			);
		}

		CompArr* GetCompArrFromCompAddr(void* compAddr)
		{
			return *reinterpret_cast<CompArr**>(
				reinterpret_cast<RawData*>(compAddr) - internal::CompArr::EntPtrSize - internal::CompArr::CompArrPtrSize
			);
		}
		const CompArr* GetCompArrFromCompAddr(const void* compAddr)
		{
			return *reinterpret_cast<const CompArr* const*>(
				reinterpret_cast<const RawData*>(compAddr) - internal::CompArr::EntPtrSize - internal::CompArr::CompArrPtrSize
			);
		}

		EntityHash GetUnusedEntityHash(const EntMapType& entityPool)
		{
			// Get an unused key
			EntityHash key{ util::Rand_UID() };
			while (entityPool.find(key) != entityPool.end())
				key = util::Rand_UID();
			return key;
		}

}
}
