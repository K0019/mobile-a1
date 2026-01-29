/******************************************************************************/
/*!
\file   ResourceTypes.ipp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Provides the base resource type and a container to store resources of user
defined resource types.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Engine/Resources/Types/ResourceTypes.h"

template<std::derived_from<ResourceBase> ResourceType>
const void* ResourceContainerBase<ResourceType>::GetResource(size_t hash)
{
	auto resourceIter{ resources.find(hash) };
	if (resourceIter == resources.end())
		return nullptr;

	// If the resource isn't loaded, try to ask the importer to load it
	if (!resourceIter->second.IsLoaded())
	{
		Messaging::BroadcastAll("NeedResourceLoaded", hash);
		// If it's still not loaded, don't return anything to ensure downstream doesn't use an invalid resource.
		if (!resourceIter->second.IsLoaded())
			return nullptr;
	}

	return &resourceIter->second;
}

template<std::derived_from<ResourceBase> ResourceType>
void ResourceContainerBase<ResourceType>::DeleteResource(size_t hash)
{
	auto resourceIter{ resources.find(hash) };
	if (resourceIter == resources.end())
		return;

	static const size_t typeHash = util::ConsistentHash<ResourceType>();  // Cache hash per type
	Messaging::BroadcastAll("ResourceDeleted", hash, typeHash);
	resources.erase(resourceIter);
}

template<std::derived_from<ResourceBase> ResourceType>
void ResourceContainerBase<ResourceType>::INTERNAL_CreateResource(size_t hash)
{
	auto* resource{ &resources[hash] };
	resource->hash = hash;
}

template<std::derived_from<ResourceBase> ResourceType>
ResourceType* ResourceContainerBase<ResourceType>::INTERNAL_GetResource(size_t hash, bool createIfMissing)
{
	if (createIfMissing)
	{
		auto* resource{ &resources[hash] };
		resource->hash = hash;
		return resource;
	}

	auto resourceIter{ resources.find(hash) };
	return (resourceIter != resources.end() ? &resourceIter->second : nullptr);
}

template<std::derived_from<ResourceBase> ResourceType>
auto ResourceContainerBase<ResourceType>::Editor_GetAllResources() const
{
	return util::ToSortedVectorOfRefs(resources);
}


template<std::derived_from<ResourceBase> ResourceType>
UserResourceGetter<ResourceType>::UserResourceGetter(ResourceContainerBase<ResourceType>* container)
	: container{ container }
{
}

template<std::derived_from<ResourceBase> ResourceType>
const ResourceType* UserResourceGetter<ResourceType>::GetResource(size_t hash) const
{
	return reinterpret_cast<const ResourceType*>(container->GetResource(hash));
}
