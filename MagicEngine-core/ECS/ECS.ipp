#include "ECS/ECS.h"
/******************************************************************************/
/*!
\file   ECS.ipp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements template functions declared in the
  interface header file for the Entity Component System (ECS).

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/


namespace ecs {

#pragma region Entities

	template<typename T>
	CompHandle<T> Entity::AddComp(T&& comp, bool isActive)
	{
		if (!CheckCanAddComp(internal::GetCompHash<T>()))
			return nullptr;

		// TODO: See if there's any way to resolve this extra lookup each time a comp is added
		// Initialize the global compArrPool with this component type
		internal::GetCompArr<T>();

		// Add component to add buffer
		uint32_t indexInBuffer{ internal::CurrentPool::ChangesBuffer().AddComp(GetHandle(), std::forward<T>(comp), !isActive) };

		// Register component as pending addition in this entity
		components.emplace(internal::GetCompHash<T>(), indexInBuffer | COMP_STATUS_TO_ADD);

		// Return the component within the addition buffer
		return reinterpret_cast<CompHandle<T>>(internal::CurrentPool::ChangesBuffer().GetCompBufferedForAddition(internal::GetCompHash<T>(), indexInBuffer));
	}

	template<typename T>
	bool Entity::RemoveComp()
	{
		return RemoveComp(ecs::internal::GetCompHash<T>());
	}

	template<typename T>
	bool Entity::RemoveCompNow()
	{
		return RemoveCompByHashNow(ecs::internal::GetCompHash<T>());
	}

	template<typename T>
	bool Entity::HasComp()
	{
		return HasComp(internal::GetCompHash<T>());
	}

	template<typename T>
	CompHandle<T> Entity::GetComp()
	{
		return reinterpret_cast<CompHandle<T>>(INTERNAL_GetCompRaw(internal::GetCompHash<T>()));
	}
	template<typename T>
	ConstCompHandle<T> Entity::GetComp() const
	{
		return reinterpret_cast<ConstCompHandle<T>>(INTERNAL_GetCompRaw(internal::GetCompHash<T>()));
	}

	template<typename T>
	CompHandle<T> Entity::GetCompInParents(int maxIterations)
	{
		CompHandle<T> comp{};
		Transform* parentTransform{ GetTransform().GetParent() };
		while (parentTransform && maxIterations)
		{
			comp = parentTransform->GetEntity()->GetComp<T>();
			if (comp)
				return comp;

			parentTransform = parentTransform->GetParent();
			--maxIterations;
		}
		return nullptr;
	}

	template<typename T>
	CompHandle<T> Entity::GetCompInChildren()
	{
		CompHandle<T> comp{};
		for (Transform* childTransform : GetTransform().GetChildren())
		{
			comp = childTransform->GetEntity()->GetComp<T>();
			if (comp)
				return comp;

			comp = childTransform->GetEntity()->GetCompInChildren<T>();
			if (comp)
				return comp;
		}
		return nullptr;
	}

	template<typename T>
	std::vector<CompHandle<T>> Entity::GetCompInChildrenVec()
	{
		std::vector<CompHandle<T>> comps{};

		for (Transform* childTransform : GetTransform().GetChildren())
		{
			Entity* childEntity = childTransform->GetEntity();
			
			CompHandle<T> comp = childEntity->GetComp<T>();
			if (comp)
				comps.push_back(comp);

			// Recurse
			std::vector<CompHandle<T>> childComps = childEntity->GetCompInChildrenVec<T>();

			// Append
			comps.insert(
				comps.end(), 
				std::make_move_iterator(childComps.begin()), 
				std::make_move_iterator(childComps.end())
			);
		}
		return comps;
	}

#pragma region // Entities

#pragma region // Components

	template<typename CompType>
	CompIterator<CompType> GetCompsBegin()
	{
		return internal::GetCompArr<CompType>()->template User_Begin<CompType, EntityHandle>();
	}
	template<typename CompType>
	ConstCompIterator<CompType> GetCompsBeginConst()
	{
		return internal::GetCompArr<CompType>()->template User_Begin<const CompType, ConstEntityHandle>();
	}

	template<typename CompType>
	CompIterator<CompType> GetCompsActiveBegin()
	{
		return internal::GetCompArr<CompType>()->template User_Begin_Active<CompType, EntityHandle>();
	}

	template<typename CompType>
	ConstCompIterator<CompType> GetCompsActiveBeginConst()
	{
		return internal::GetCompArr<CompType>()->template User_Begin_Active<const CompType, ConstEntityHandle>();
	}

	template<typename CompType>
	CompIterator<CompType> GetCompsEnd()
	{
		return internal::GetCompArr<CompType>()->template User_End<CompType, EntityHandle>();
	}
	template<typename CompType>
	ConstCompIterator<CompType> GetCompsEndConst()
	{
		return internal::GetCompArr<CompType>()->template User_End<const CompType, ConstEntityHandle>();
	}

	template<typename CompType>
	CompIterator<CompType> GetCompsIter(CompHandle<CompType> comp)
	{
		return internal::GetCompArr<CompType>()->template User_Custom<CompType, EntityHandle>(
			reinterpret_cast<internal::InternalEntityHandle>(reinterpret_cast<internal::RawData*>(comp) - internal::CompArr::EntPtrSize));
	}

	template<typename CompType>
	size_t GetCompsCount()
	{
		return internal::GetCompArr<CompType>()->GetNumComps();
	}

	template<typename CompType>
	size_t GetCompsActiveCount()
	{
		return internal::GetCompArr<CompType>()->GetNumActiveComps();
	}

	template<typename CompType>
	constexpr CompHash GetCompHash()
	{
		return internal::GetCompHash<CompType>();
	}

	template<typename CompType>
	const CompTypeMeta* GetCompMeta()
	{
		return GetCompMeta(GetCompHash<CompType>());
	}

#pragma endregion // Components

#pragma region Systems

	template<typename SysType>
	SysHandle<SysType> AddSystem(ECS_LAYER layer, SysType&& system)
	{
		return reinterpret_cast<SysHandle<SysType>>(internal::CurrentPool::Systems().AddSystem(
			static_cast<int>(layer), std::forward<SysType>(system)));
	}

	template<typename SysType>
	SysHandle<SysType> GetSystem()
	{
		return reinterpret_cast<SysHandle<SysType>>(internal::CurrentPool::Systems().GetSystem<SysType>());
	}

	template<typename SysType>
	bool RemoveSystem()
	{
		return internal::CurrentPool::Systems().RemoveSystem<SysType>();
	}

	template<typename SysType>
	constexpr SysHash GetSysHash()
	{
		return internal::GetSysHash<SysType>();
	}

	template<typename SysType>
	const SysTypeMeta* GetSysMeta()
	{
		return GetSysMeta(GetSysHash<SysType>());
	}

#pragma endregion // Systems

}