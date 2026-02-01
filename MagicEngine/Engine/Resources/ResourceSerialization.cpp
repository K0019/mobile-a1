/******************************************************************************/
/*!
\file   ResourceSerialization.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Serializes the record of imported resources to and from file.
Extended to support asset type and source tracking fields.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/ResourceSerialization.h"
#include "resource/asset_metadata.h"
#include "VFS/VFS.h"
#include <filesystem>

void ResourceSerialization::Serialize(Serializer& writer, const ResourceFilepaths& filepaths, const ResourceNames& names)
{
	writer.StartArray("files");
	for (const auto fileEntry : filepaths.GetFileEntries())
	{
		writer.StartObject();

		writer.Serialize("filepath", fileEntry->path);

		// Extended fields (always written, backwards compatible)
		writer.Serialize("assetType", std::string(AssetFormat::getAssetTypeName(fileEntry->assetType)));
		if (!fileEntry->sourcePath.empty())
			writer.Serialize("sourcePath", fileEntry->sourcePath);
		if (fileEntry->sourceTimestamp > 0)
			writer.Serialize("sourceTimestamp", fileEntry->sourceTimestamp);
		if (fileEntry->compiledTimestamp > 0)
			writer.Serialize("compiledTimestamp", fileEntry->compiledTimestamp);
		if (fileEntry->formatVersion > 0)
			writer.Serialize("formatVersion", fileEntry->formatVersion);

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
				const std::string* name = names.GetName(hash);
				writer.Serialize("name", name ? *name : std::string("unnamed"));
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

		// Read extended fields (optional, for backwards compatibility)
		std::string assetTypeStr{};
		std::string sourcePath{};
		uint64_t sourceTimestamp = 0;
		uint64_t compiledTimestamp = 0;
		uint32_t formatVersion = 0;

		// These will silently fail if field doesn't exist (old format)
		reader.DeserializeVar("assetType", &assetTypeStr);
		reader.DeserializeVar("sourcePath", &sourcePath);
		reader.DeserializeVar("sourceTimestamp", &sourceTimestamp);
		reader.DeserializeVar("compiledTimestamp", &compiledTimestamp);
		reader.DeserializeVar("formatVersion", &formatVersion);

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
				hash |= 1; // Legacy fix for non-odd hashes
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

		// Get the primary resource hash before moving resources
		size_t primaryHash = 0;
		if (!resources.empty() && !resources[0].hashes.empty())
			primaryHash = resources[0].hashes[0];

		// SetFilepath will load from .meta file if extended fields not in JSON
		filepaths->SetFilepath(filepath, std::move(resources));

		// If we read extended fields from JSON, apply them (overrides .meta file data)
		if (!assetTypeStr.empty() || !sourcePath.empty() || sourceTimestamp > 0)
		{
			AssetFormat::AssetType assetType = Resource::AssetMetadata::parseAssetType(assetTypeStr.c_str());
			filepaths->UpdateEntryMetadata(filepath, assetType, sourcePath,
			                               sourceTimestamp, compiledTimestamp, formatVersion);
		}

		// Migration: backfill .meta file with hash from assets.json if needed
		if (primaryHash != 0)
		{
			std::filesystem::path physicalPath = VFS::ConvertVirtualToPhysical(filepath);
			std::filesystem::path metaPath = Resource::AssetMetadata::getMetaPath(physicalPath);

			Resource::AssetMetadata meta;
			meta.loadFromFile(metaPath);  // OK if file doesn't exist - we'll create it

			// Only update if .meta is missing the hash
			if (!meta.hasResourceHash())
			{
				meta.resourceHash = primaryHash;

				// Also populate other fields if we have them from JSON
				if (!assetTypeStr.empty())
					meta.assetType = Resource::AssetMetadata::parseAssetType(assetTypeStr.c_str());
				else if (meta.assetType == AssetFormat::AssetType::Unknown)
					meta.assetType = Resource::AssetMetadata::inferFromPath(filepath);

				if (!sourcePath.empty() && meta.sourcePath.empty())
					meta.sourcePath = sourcePath;
				if (sourceTimestamp > 0 && meta.sourceTimestamp == 0)
					meta.sourceTimestamp = sourceTimestamp;
				if (compiledTimestamp > 0 && meta.compiledTimestamp == 0)
					meta.compiledTimestamp = compiledTimestamp;
				if (formatVersion > 0 && meta.formatVersion == 0)
					meta.formatVersion = formatVersion;

				meta.saveToFile(metaPath);
			}
		}

		reader.PopAccess(); // Element
	}
	reader.PopAccess(); // files
}
