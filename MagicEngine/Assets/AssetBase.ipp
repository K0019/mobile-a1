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
#include "Assets/AssetBase.h"
#include <cassert>

template<std::derived_from<ResourceBase> ResourceType>
const void* ResourceContainerBase<ResourceType>::GetResource(size_t hash)
{
	auto resourceIter{ resources.find(hash) };
	if (resourceIter == resources.end() || !resourceIter->second)
		return nullptr;

	// If the resource isn't loaded, queue it for deferred loading.
	// Return nullptr so callers use their fallback/placeholder path.
	// The resource will be imported by AssetManager::ProcessPendingLoads()
	// on the main thread between frames, avoiding ANR on Android.
	if (!resourceIter->second->IsLoaded())
	{
		// Only broadcast once per hash to avoid spamming hot loops
		if (m_deferredRequested.insert(hash).second)
			Messaging::BroadcastAll("NeedResourceLoadDeferred", hash);
		return nullptr;
	}

	// Resource is loaded — clear deferred flag so future unloads can re-request
	m_deferredRequested.erase(hash);
	return resourceIter->second.get();
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
	refCounts.erase(hash);
	m_deferredRequested.erase(hash);
}

template<std::derived_from<ResourceBase> ResourceType>
void ResourceContainerBase<ResourceType>::INTERNAL_CreateResource(size_t hash)
{
	assert((hash & 1) && "Asset hash must have bit 0 set (odd) for pointer-tagging to work");
	auto& ptr = resources[hash];
	if (!ptr)
		ptr = std::make_unique<ResourceType>();
	ptr->hash = hash;
}

template<std::derived_from<ResourceBase> ResourceType>
ResourceType* ResourceContainerBase<ResourceType>::INTERNAL_GetResource(size_t hash, bool createIfMissing)
{
	if (createIfMissing)
	{
		assert((hash & 1) && "Asset hash must have bit 0 set (odd) for pointer-tagging to work");
		auto& ptr = resources[hash];
		if (!ptr)
			ptr = std::make_unique<ResourceType>();
		ptr->hash = hash;
		return ptr.get();
	}

	auto resourceIter{ resources.find(hash) };
	return (resourceIter != resources.end() ? resourceIter->second.get() : nullptr);
}

template<std::derived_from<ResourceBase> ResourceType>
auto ResourceContainerBase<ResourceType>::Editor_GetAllResources() const
{
	std::vector<std::pair<std::reference_wrapper<const size_t>, std::reference_wrapper<const ResourceType>>> vec;
	vec.reserve(resources.size());
	for (const auto& [key, ptr] : resources)
		if (ptr)
			vec.emplace_back(std::cref(key), std::cref(*ptr));
	std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) { return a.first.get() < b.first.get(); });
	return vec;
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
