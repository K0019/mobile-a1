#pragma once
#include "ResourceTypes.h"

template<std::derived_from<ResourceBase> ResourceType>
const ResourceType* ResourceContainerBase<ResourceType>::GetResource(size_t hash)
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

	Messaging::BroadcastAll("ResourceDeleted", hash, typeid(ResourceType).hash_code());
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
	return container->GetResource(hash);
}
