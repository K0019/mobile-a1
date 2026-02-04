/******************************************************************************/
/*!
\file   AssetSerialization.h
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
#include "Assets/AssetFilepaths.h"
#include "Assets/AssetNames.h"
#include "Utilities/Serializer.h"

class AssetSerialization
{
public:
	static void Serialize(Serializer& writer, const AssetFilepaths& filepaths, const AssetNames& names);

	using LoadedResourceCallbackType = void(*)(size_t resourceTypeHash, size_t resourceHash);
	static void Deserialize(Deserializer& reader, AssetFilepaths* filepaths, AssetNames* names, LoadedResourceCallbackType resourceLoadedCallback);
};

