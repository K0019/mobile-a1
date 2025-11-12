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
#include "Engine/Resources/ResourceManager.h"
#include "Utilities/Serializer.h"

template <typename ResourceType>
class UserResourceHandle : public ISerializeable
{
public:
    UserResourceHandle(size_t hash = 1);

    size_t GetHash() const;
    const ResourceType* GetResource() const;

    UserResourceHandle<ResourceType>& operator=(size_t hash);

public:
    void Serialize(Serializer& writer) const override;
    void Deserialize(Deserializer& reader) override;

private:
    mutable size_t hashOrPtr;

};

#include "ResourcesHeader.ipp"
