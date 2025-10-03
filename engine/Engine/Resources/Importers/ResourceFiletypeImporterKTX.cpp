#include "ResourceFiletypeImporterKTX.h"
#include "ResourceTypesGraphics.h"

#include "GraphicsAPI.h"
#include "asset_types.h"
#include "texture_loader.h"

bool ResourceFiletypeImporterKTX::Import(const std::filesystem::path& assetRelativeFilepath)
{
	// Load the file
	FilePathSource filepathSource{ GetExeRelativeFilepath(assetRelativeFilepath) };
	AssetLoading::LoadingConfig config{ AssetLoading::LoadingConfig::createBalanced() };
	AssetLoading::ProcessedTexture processedTexture{ AssetLoading::TextureLoading::extractTexture(filepathSource, config) };
	if (processedTexture.data.empty())
		return false;

	// Create texture handle
	auto graphicsAssetSystem{ ST<GraphicsAssets>::Get()->INTERNAL_GetAssetSystem() };
	TextureHandle textureHandle{ graphicsAssetSystem->createTexture(processedTexture) };

	// Create resource entry
	const auto* fileEntry{ GenerateFileEntryForResources<ResourceTexture>(assetRelativeFilepath, 1) };
	
	// Assign resource to texture handle
	ST<ResourceManager>::Get()->INTERNAL_GetTextures().INTERNAL_GetResource(fileEntry->associatedResources[0].hashes[0], true)->handle = textureHandle;

	return true;
}
