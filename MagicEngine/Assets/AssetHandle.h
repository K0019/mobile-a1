/******************************************************************************/
/*!
\file   ResourceTypes.h
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
#include "Assets/AssetManager.h"
#include "Utilities/Serializer.h"

template <typename ResourceType>
class AssetHandle : public ISerializeable
{
public:
    AssetHandle(size_t hash = 1);
    ~AssetHandle();
    AssetHandle(const AssetHandle& other);
    AssetHandle(AssetHandle&& other) noexcept;
    AssetHandle<ResourceType>& operator=(const AssetHandle& other);
    AssetHandle<ResourceType>& operator=(AssetHandle&& other) noexcept;

    size_t GetHash() const;
    const ResourceType* GetResource() const;

    AssetHandle<ResourceType>& operator=(size_t hash);

public:
    void Serialize(Serializer& writer) const override;
    void Deserialize(Deserializer& reader) override;

private:
    mutable size_t hashOrPtr;

    // Reference counting helpers
    static void AddRef(size_t hash);
    static void ReleaseRef(size_t hash);

};

#include "AssetHandle.ipp"
