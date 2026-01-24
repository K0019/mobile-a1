/******************************************************************************/
/*!
\file   ResourceFiletypeImporterAudio.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/27/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A resource importer for audio files supported by FMOD.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Importers/ResourceFiletypeImporterGame.h"
#include "Game/Weapon.h"

bool ResourceFiletypeImporterGameWeapon::Import(const std::string& assetRelativeFilepath)
{
	WeaponInfo weaponInfo{};
	Deserializer reader{ assetRelativeFilepath };
	if (!reader.IsValid())
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Unable to read game weapon file: " << assetRelativeFilepath;
		return false;
	}
	reader.Deserialize(&weaponInfo);

	const auto fileEntry{ GenerateFileEntryForResources<WeaponInfo>(assetRelativeFilepath, 1) };
	weaponInfo.hash = fileEntry->associatedResources[0].hashes[0];
	*ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<WeaponInfo>().INTERNAL_GetResource(weaponInfo.hash, true) = std::move(weaponInfo);
	return true;
}
