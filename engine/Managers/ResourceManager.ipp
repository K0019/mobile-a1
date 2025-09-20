#pragma once
#include "ResourceManager.h"

template<std::derived_from<ResourceBase> ResourceType>
const ResourceType* ResourceContainerBase<ResourceType>::GetResource(size_t hash)
{
	auto resourceIter{ resources.find(hash) };
	if (resourceIter == resources.end())
		return nullptr;

	// If the resource can't load, don't return anything to ensure upstream doesn't use an invalid resource.
	if (!resourceIter->second.Load())
		return nullptr;

	return &resourceIter->second;
}

template<std::derived_from<ResourceBase> ResourceType>
ResourceType* ResourceContainerBase<ResourceType>::CreateResource(const std::string& name, const std::string& filepath)
{
	size_t hash{ util::GenHash(filepath) };
	assert(resources.find(hash) == resources.end());

	bool filepathAddedOk{ ST<ResourceFilepaths>::Get()->AddFilepath(hash, filepath) };
	assert(filepathAddedOk);

	ResourceType* resource{ &resources[hash] };
	resource->hash = hash;
	resource->name = name;
	return resource;
}

template<std::derived_from<ResourceBase> ResourceType>
void ResourceContainerBase<ResourceType>::Serialize(Serializer& writer) const
{
	auto sortedResources{ util::ToSortedVectorOfRefs(resources) };

	writer.StartArray("resources");
	for (const auto& [hash, resource] : sortedResources)
		writer.Serialize("", resource.get());
	writer.EndArray();
}

template<std::derived_from<ResourceBase> ResourceType>
void ResourceContainerBase<ResourceType>::Deserialize(Deserializer& reader)
{
	if (!reader.PushAccess("resources"))
		return;

	for (size_t i{}; reader.PushArrayElementAccess(i); reader.PopAccess(), ++i)
	{
		size_t hash{};
		if (!reader.DeserializeVar("hash", &hash))
			continue;

		auto& resource{ resources.try_emplace(hash).first->second };
		reader.Deserialize("", &resource);
	}

	reader.PopAccess();
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
