#include "ResourceSerialization.h"

void ResourceSerialization::Serialize(Serializer& writer, const ResourceFilepaths& filepaths, const ResourceNames& names)
{
	writer.StartArray("files");
	for (const auto fileEntry : filepaths.GetFileEntries())
	{
		writer.StartObject();

		writer.Serialize("filepath", fileEntry->path.string());
		writer.StartArray("resources");

		for (const auto& resource : fileEntry->associatedResources)
		{
			writer.StartObject();
			writer.Serialize("resourceType", resource.resourceTypeHash);
			writer.StartArray("resourceEntries");

			for (size_t hash : resource.hashes)
			{
				writer.StartObject();
				writer.Serialize("hash", hash);
				writer.Serialize("name", *names.GetName(hash));
				writer.EndObject();
			}

			writer.EndArray();
			writer.EndObject();
		}

		writer.EndArray();
		writer.EndObject();
	}
	writer.EndArray();
}

void ResourceSerialization::Deserialize(Deserializer& reader, ResourceFilepaths* filepaths, ResourceNames* names, LoadedResourceCallbackType resourceLoadedCallback)
{
	reader.PushAccess("files");
	for (size_t fileIndex{}; reader.PushArrayElementAccess(fileIndex); ++fileIndex)
	{
		std::string filepath{};
		reader.DeserializeVar("filepath", &filepath);

		std::vector<AssociatedResourceHashes> resources;
		reader.PushAccess("resources");
		for (size_t resourceTypeIndex{}; reader.PushArrayElementAccess(resourceTypeIndex); ++resourceTypeIndex)
		{
			auto& newResource{ resources.emplace_back() };
			reader.DeserializeVar("resourceType", &newResource.resourceTypeHash);

			reader.PushAccess("resourceEntries");
			for (size_t hashIndex{}; reader.PushArrayElementAccess(hashIndex); ++hashIndex)
			{
				size_t hash{};
				reader.DeserializeVar("hash", &hash);
				newResource.hashes.push_back(hash);
				resourceLoadedCallback(newResource.resourceTypeHash, hash);

				std::string name{};
				reader.DeserializeVar("name", &name);
				names->SetName(hash, name);

				reader.PopAccess(); // Element
			}
			reader.PopAccess(); // resourceEntries

			reader.PopAccess(); // Element
		}
		reader.PopAccess(); // resources

		filepaths->SetFilepath(filepath, std::move(resources));
		reader.PopAccess(); // Element
	}
	reader.PopAccess(); // files
}
