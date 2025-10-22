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
#ifndef GLFW
#include "tools/assets/io/texture_loader.h"
#else
#include "ktx.h"
#include <graphics/vulkan/vk_util.h>
#endif

bool ResourceFiletypeImporterKTX::Import(const std::filesystem::path& assetRelativeFilepath)
{
#ifndef GLFW
	// Load the file
	FilePathSource filepathSource{ GetExeRelativeFilepath(assetRelativeFilepath) };
	Resource::LoadingConfig config{ Resource::LoadingConfig::createBalanced() };
	Resource::ProcessedTexture processedTexture{ Resource::TextureLoading::extractTexture(filepathSource, config) };
#else
	Resource::ProcessedTexture processedTexture{ ManualLoadTexture(GetExeRelativeFilepath(assetRelativeFilepath)) };
#endif
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

#ifdef GLFW
Resource::ProcessedTexture ResourceFiletypeImporterKTX::ManualLoadTexture(const std::filesystem::path& filepath)
{
	// This is hardcoded af to make android work for now. For this, we're only supporting ktx2.
	assert(filepath.extension() == "ktx2" || filepath.extension() == "KTX2");

	Resource::ProcessedTexture texture{};
    ktxTexture2* ktxTex = nullptr;

    KTX_error_code result = ktxTexture2_CreateFromNamedFile(filepath.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

    if (result != KTX_SUCCESS || !ktxTex)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to load KTX2 file " << filepath.string() << ".KTX Error : " << ktxErrorString(result);
        if (ktxTex) ktxTexture_Destroy(ktxTexture(ktxTex));
        return texture;
    }

    if (ktxTex->classId != class_id::ktxTexture2_c)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "File " << filepath.string() << "is not a KTX2 file. Found KTX1 instead.";
        ktxTexture_Destroy(ktxTexture(ktxTex));
        return texture;
    }

    SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

    // Extract texture properties
    texture.originalFileSize = std::filesystem::file_size(filepath);
    texture.width = ktxTex->baseWidth;
    texture.height = ktxTex->baseHeight;
    texture.channels = 4; // Most KTX textures are 4-channel

    // Copy texture data
    const size_t dataSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
    texture.data.resize(dataSize);
    std::memcpy(texture.data.data(), ktxTex->pData, dataSize);

    // Create descriptor
    texture.textureDesc = vk::TextureDesc{
        .type = vk::TextureType::Tex2D,
        .format = vk::vkFormatToFormat(static_cast<VkFormat>(ktxTex->vkFormat)),
        .dimensions = {texture.width, texture.height, 1},
        .usage = vk::TextureUsageBits_Sampled,
        .numMipLevels = ktxTex->numLevels,
        .debugName = filepath.string().c_str() };

    CONSOLE_LOG(LEVEL_DEBUG) << "Loaded KTX2 texture " << filepath.string() << " - " << texture.width << "x" << texture.height << ", " << ktxTex->numLevels << " mips";
    return texture;
}
#endif
