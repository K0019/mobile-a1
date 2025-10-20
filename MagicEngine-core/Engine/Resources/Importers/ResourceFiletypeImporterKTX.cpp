/******************************************************************************/
/*!
\file   ResourceFiletypeImporterKTX.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/27/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A resource importer for KTX files.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Importers/ResourceFiletypeImporterKTX.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "resource/resource_types.h"
#include "tools/assets/io/texture_loader.h"

bool ResourceFiletypeImporterKTX::Import(const std::string& assetRelativeFilepath)
{
	// Load the file
	//FilePathSource filepathSource{ GetExeRelativeFilepath(assetRelativeFilepath) };
	FilePathSource filepathSource{ assetRelativeFilepath };

	Resource::LoadingConfig config{ Resource::LoadingConfig::createBalanced() };
	Resource::ProcessedTexture processedTexture{ Resource::TextureLoading::extractTexture(filepathSource, config) };
	if (processedTexture.data.empty())
		return false;

	// Create texture handle
	auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
	TextureHandle textureHandle{ graphicsAssetSystem.createTexture(processedTexture) };

	graphicsAssetSystem.FlushUploads();

	// Create resource entry
	const auto* fileEntry{ GenerateFileEntryForResources<ResourceTexture>(assetRelativeFilepath, 1) };
	
	// Assign resource to texture handle
	ST<MagicResourceManager>::Get()->INTERNAL_GetTextures().INTERNAL_GetResource(fileEntry->associatedResources[0].hashes[0], true)->handle = textureHandle;

	return true;
}
