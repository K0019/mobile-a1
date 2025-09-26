#pragma once
#include "ResourceFilepaths.h"
#include "ResourceNames.h"
#include "Serializer.h"

class ResourceSerialization
{
public:
	static void Serialize(Serializer& writer, const ResourceFilepaths& filepaths, const ResourceNames& names);

	using LoadedResourceCallbackType = void(*)(size_t resourceTypeHash, size_t resourceHash);
	static void Deserialize(Deserializer& reader, ResourceFilepaths* filepaths, ResourceNames* names, LoadedResourceCallbackType resourceLoadedCallback);
};

