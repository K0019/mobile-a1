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
#include "Assets/AssetHandle.h"

template<typename ResourceType>
void AssetHandle<ResourceType>::AddRef(size_t hash)
{
	if (hash != 1 && ST<AssetManager>::IsInitialized())
		ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceType>().IncrementRef(hash);
}

template<typename ResourceType>
void AssetHandle<ResourceType>::ReleaseRef(size_t hash)
{
	if (hash != 1 && ST<AssetManager>::IsInitialized())
		ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceType>().DecrementRef(hash);
}

template<typename ResourceType>
AssetHandle<ResourceType>::AssetHandle(size_t hash)
	: hashOrPtr{ hash | 1 }  // Ensure hash form (bit 0 set)
{
	AddRef(GetHash());
}

template<typename ResourceType>
AssetHandle<ResourceType>::~AssetHandle()
{
	ReleaseRef(GetHash());
}

template<typename ResourceType>
AssetHandle<ResourceType>::AssetHandle(const AssetHandle& other)
	: hashOrPtr{ other.GetHash() | 1 }  // Reset to hash form (force re-resolve)
{
	AddRef(GetHash());
}

template<typename ResourceType>
AssetHandle<ResourceType>::AssetHandle(AssetHandle&& other) noexcept
	: hashOrPtr{ other.hashOrPtr }
{
	other.hashOrPtr = 1;  // Null out source (no ref change needed)
}

template<typename ResourceType>
AssetHandle<ResourceType>& AssetHandle<ResourceType>::operator=(const AssetHandle& other)
{
	if (this != &other) {
		size_t oldHash = GetHash();
		hashOrPtr = other.GetHash() | 1;  // Reset to hash form
		size_t newHash = GetHash();
		if (oldHash != newHash) {
			ReleaseRef(oldHash);
			AddRef(newHash);
		}
	}
	return *this;
}

template<typename ResourceType>
AssetHandle<ResourceType>& AssetHandle<ResourceType>::operator=(AssetHandle&& other) noexcept
{
	if (this != &other) {
		ReleaseRef(GetHash());
		hashOrPtr = other.hashOrPtr;
		other.hashOrPtr = 1;  // Null out source
	}
	return *this;
}

template<typename ResourceType>
size_t AssetHandle<ResourceType>::GetHash() const
{
	if (hashOrPtr & 1)
		return hashOrPtr;
	// Pointer form — only safe to dereference if the manager (and thus containers) is still alive
	if (!ST<AssetManager>::IsInitialized())
		return 1;  // Sentinel: manager torn down, treat as null handle
	return reinterpret_cast<const ResourceType*>(hashOrPtr)->hash;
}

template<typename ResourceType>
const ResourceType* AssetHandle<ResourceType>::GetResource() const
{
	if (hashOrPtr & 1)
	{
		if (auto ptr{ ST<AssetManager>::Get()->GetContainer<ResourceType>().GetResource(hashOrPtr) })
			hashOrPtr = reinterpret_cast<size_t>(ptr);
		if (hashOrPtr & 1)
			return nullptr;
	}
	return reinterpret_cast<const ResourceType*>(hashOrPtr);
}

template<typename ResourceType>
AssetHandle<ResourceType>& AssetHandle<ResourceType>::operator=(size_t hash)
{
	size_t oldHash = GetHash();
	hashOrPtr = hash | 1;  // Ensure hash form (bit 0 set)
	size_t newHash = GetHash();
	if (oldHash != newHash) {
		ReleaseRef(oldHash);
		AddRef(newHash);
	}
	return *this;
}

template<typename ResourceType>
void AssetHandle<ResourceType>::Serialize(Serializer& writer) const
{
	writer.Serialize("hash", GetHash());
}

template<typename ResourceType>
void AssetHandle<ResourceType>::Deserialize(Deserializer& reader)
{
	size_t oldHash = GetHash();
	reader.DeserializeVar("hash", &hashOrPtr);
	hashOrPtr |= 1; // Ensure hash form (bit 0 set)
	size_t newHash = GetHash();
	if (oldHash != newHash) {
		ReleaseRef(oldHash);
		AddRef(newHash);
	}
}
