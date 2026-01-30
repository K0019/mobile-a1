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
#include "Engine/Resources/ResourcesHeader.h"

template<typename ResourceType>
UserResourceHandle<ResourceType>::UserResourceHandle(size_t hash)
	: hashOrPtr{ hash }
{
}

template<typename ResourceType>
size_t UserResourceHandle<ResourceType>::GetHash() const
{
	if (hashOrPtr & 1)
		return hashOrPtr;
	else
		return reinterpret_cast<const ResourceType*>(hashOrPtr)->hash;
}

template<typename ResourceType>
const ResourceType* UserResourceHandle<ResourceType>::GetResource() const
{
	if (hashOrPtr & 1)
	{
		if (auto ptr{ ST<MagicResourceManager>::Get()->GetContainer<ResourceType>().GetResource(hashOrPtr) })
			hashOrPtr = reinterpret_cast<size_t>(ptr);
		if (hashOrPtr & 1)
			return nullptr;
	}
	return reinterpret_cast<const ResourceType*>(hashOrPtr);
}

template<typename ResourceType>
UserResourceHandle<ResourceType>& UserResourceHandle<ResourceType>::operator=(size_t hash)
{
	hashOrPtr = hash;
	return *this;
}

template<typename ResourceType>
void UserResourceHandle<ResourceType>::Serialize(Serializer& writer) const
{
	if (hashOrPtr & 1)
		writer.Serialize("hash", hashOrPtr);
	else
		writer.Serialize("hash", reinterpret_cast<ResourceType*>(hashOrPtr)->hash);
}

template<typename ResourceType>
void UserResourceHandle<ResourceType>::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("hash", &hashOrPtr);
	hashOrPtr |= 1; // Legacy fix for non-odd hashes
}
