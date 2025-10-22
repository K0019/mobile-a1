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
#ifdef GLFW
#include "tools/assets/io/texture_loader.h"
#else
#include "ktx.h"
#include <graphics/vulkan/vk_util.h>
#endif

bool ResourceFiletypeImporterKTX::Import(const std::string& assetRelativeFilepath)
{
#ifdef GLFW
	// Load the file
	//FilePathSource filepathSource{ GetExeRelativeFilepath(assetRelativeFilepath) };
	FilePathSource filepathSource{ assetRelativeFilepath };

	Resource::LoadingConfig config{ Resource::LoadingConfig::createBalanced() };
	Resource::ProcessedTexture processedTexture{ Resource::TextureLoading::extractTexture(filepathSource, config) };
#else
	//Resource::ProcessedTexture processedTexture{ ManualLoadTexture(GetExeRelativeFilepath(assetRelativeFilepath)) };
	Resource::ProcessedTexture processedTexture{ ManualLoadTexture(assetRelativeFilepath) };
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

#ifndef GLFW
Resource::ProcessedTexture ResourceFiletypeImporterKTX::ManualLoadTexture(const std::string& filepath)
{
	// This is hardcoded af to make android work for now. For this, we're only supporting ktx2.
	assert(VFS::GetExtension(filepath) == ".ktx2");

	Resource::ProcessedTexture texture{};
    ktxTexture2* ktxTex = nullptr;

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(filepath, fileData))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "VFS: Read texture file failed: " << filepath;
        return texture;
    }

    KTX_error_code result = ktxTexture2_CreateFromMemory(fileData.data(), fileData.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

    if (result != KTX_SUCCESS || !ktxTex)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to load KTX2 file " << filepath << ".KTX Error : " << ktxErrorString(result);
        if (ktxTex) ktxTexture_Destroy(ktxTexture(ktxTex));
        return texture;
    }

    if (ktxTex->classId != class_id::ktxTexture2_c)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "File " << filepath << "is not a KTX2 file. Found KTX1 instead.";
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
        .debugName = filepath.c_str() };

    CONSOLE_LOG(LEVEL_DEBUG) << "Loaded KTX2 texture " << filepath << " - " << texture.width << "x" << texture.height << ", " << ktxTex->numLevels << " mips";
    return texture;
}
#endif
