/******************************************************************************/
/*!
\file   ResourceSerialization.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Serializes the record of imported resources to and from file.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

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

