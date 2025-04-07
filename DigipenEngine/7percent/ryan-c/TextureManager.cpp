/******************************************************************************/
/*!
\file   TextureManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Only file to include stb_image.h. Definition of texture manager is here too i guess.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TextureManager.h"
#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma warning(pop)
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
namespace fs = std::filesystem;
#include "Device.h"
#include "Engine.h"
#include "VkInit.h"

TextureManager::TextureManager() {}

TextureManager::~TextureManager()
{
	auto m_device = VulkanManager::Get().VkDevice().handle();
	auto _allocator = VulkanManager::Get().VkAllocator();

	for(auto& image : m_textures)
	{
		vkDestroySampler(m_device, image.second.sampler, pAllocator);
		vmaDestroyImage(_allocator, image.second.image._image, image.second.image._allocation);
		vkDestroyImageView(m_device, image.second.imageView, pAllocator);
	}
}

TextureManager::TextureManager(TextureManager&&) noexcept = default;

TextureManager& TextureManager::operator=(TextureManager&&) noexcept = default;

uint32_t TextureManager::loadTextureFromFile(const std::string& filename, const std::string& name, bool isFont)
{
	int texWidth, texHeight, texChannels;
	if (stbi_uc* pixels{ stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha) })
	{
		uint32_t index{ LoadTexture(pixels, texWidth, texHeight, (name.empty() ? removeDirectoryAndExtension(filename) : name), isFont) };
		stbi_image_free(pixels);
		return index;
	}
	else
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to load texture file " << filename;
		return ResourceManager::INVALID_TEXTURE_ID;
	}
}

uint32_t TextureManager::LoadTextureFromMemory(const unsigned char* data, int width, int height, const std::string& name)
{
	return LoadTexture(data, width, height, name, false);
}

std::string TextureManager::loadFontAtlasFromFile(const std::string& filename)
{
	auto fontName = removeDirectoryAndExtension(filename);

	size_t lastDot = filename.find_last_of('.');

	// Extract the name without extension
	std::string nameWithoutExt = (lastDot == std::string::npos) ?
		filename :
		filename.substr(0, lastDot);

	std::string jsonPath = nameWithoutExt + "-atlas.json";
	std::string pngPath = nameWithoutExt + "-atlas.png";

	if(!fs::exists(jsonPath) || !fs::exists(pngPath)) {
		CONSOLE_LOG(LEVEL_ERROR) << "Font atlas files not found for: " << fontName;
	}

	FontAtlas atlas = loadJsonData(jsonPath);
	loadTextureFromFile(pngPath, fontName, true);
	atlas.textureName = fontName;
	atlas.textureID = getTextureIndex(fontName);
	m_fontAtlases[fontName] = atlas;
	return fontName;
}

const Texture& TextureManager::getTexture(const std::string& name)
{
	return getTextureInternal(name);
}
namespace
{
	Texture invalidTexture{.index = ResourceManager::INVALID_TEXTURE_ID};
}

const Texture& TextureManager::getTexture(uint32_t index) const
{
	for(auto& texture : m_textures)
	{
		if(texture.second.index == index)
		{
			return texture.second;
		}
	}
	CONSOLE_LOG(LEVEL_ERROR) << "Texture not found with index: " << index;
	return invalidTexture;
}

uint32_t TextureManager::getTextureIndex(const std::string& name)
{
	auto it = m_textures.find(name);
	if(it == m_textures.end()) {
		CONSOLE_LOG(LEVEL_ERROR) << "Texture not found: " << name;
		return ResourceManager::INVALID_TEXTURE_ID;
	}
	return it->second.index;
}

const FontAtlas& TextureManager::getFontAtlas(const std::string& name)
{
	if(!m_fontAtlases.contains(name))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Font atlas not found: " << name;
		return m_fontAtlases.begin()->second;
	}
	return { m_fontAtlases.at(name) };
}

DescriptorSetManager::DescriptorSetHandle TextureManager::getBindlessSet() const
{
	return m_bindless_set;
}

void TextureManager::SetBindlessLayout(VkDescriptorSetLayout layout)
{
	m_bindless_layout = layout;
	m_bindless_set = VulkanManager::Get().VkDescriptorSetManager().createDescriptorSet(layout);
}

bool TextureManager::TextureExists(const std::string& name) const
{
	return m_textures.contains(name);
}

bool TextureManager::FontAtlasExists(const std::string& name)
{
	return m_fontAtlases.contains(name);
}

const std::unordered_map<std::string, FontAtlas>& TextureManager::getFontAtlases() const
{
	return m_fontAtlases;
}

VkImageView TextureManager::createImageView(VkFormat vk_format, VkImage image, VkImageAspectFlags flags)
{
	VkImageViewCreateInfo viewInfo = VkInit::ImageViewCreateInfo(vk_format, image, flags);
	VkImageView imageView;
	vkCreateImageView(VulkanManager::Get().VkDevice().handle(), &viewInfo, pAllocator, &imageView);
	return imageView;
}

VkSampler TextureManager::createSampler()
{
	VkSamplerCreateInfo samplerInfo = VkInit::SamplerCreateInfo(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkSampler sampler;
	vkCreateSampler(VulkanManager::Get().VkDevice().handle(), &samplerInfo, pAllocator, &sampler);
	return sampler;
}

VkSampler TextureManager::createFontSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkSampler sampler;
	if(vkCreateSampler(VulkanManager::Get().VkDevice().handle(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create font texture sampler!");
	}
	return sampler;
}

Texture& TextureManager::getTextureInternal(const std::string& name)
{
	if(!m_textures.contains(name))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Texture not found: " << name;
		return m_textures.begin()->second;
	}
	return m_textures.at(name);
}

std::string TextureManager::removeDirectoryAndExtension(const std::string& filename) {
	// Find the last directory separator
	size_t lastSeparator = filename.find_last_of("/\\");

	// Extract the filename without directory
	std::string nameWithoutDir = (lastSeparator == std::string::npos) ?
		filename :
		filename.substr(lastSeparator + 1);

	// Find the last dot (for the extension)
	size_t lastDot = nameWithoutDir.find_last_of('.');

	// Extract the name without extension
	std::string nameWithoutExt = (lastDot == std::string::npos) ?
		nameWithoutDir :
		nameWithoutDir.substr(0, lastDot);

	return nameWithoutExt;
}

FontAtlas TextureManager::loadJsonData(const std::string& jsonPath) {
	FontAtlas atlas;
	std::ifstream ifs(jsonPath);
	if(!ifs.is_open()) {
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to open font atlas JSON file: " << jsonPath;
		return atlas;
	}

	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document doc;
	doc.ParseStream(isw);

	if(doc.HasParseError()) {
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to parse font atlas JSON: " << jsonPath;
		return atlas;
	}

	// Read atlas properties
	if(doc.HasMember("atlas") && doc["atlas"].IsObject()) {
		const auto& atlasObj = doc["atlas"];
		atlas.width = atlasObj["width"].GetInt();
		atlas.height = atlasObj["height"].GetInt();
	}

	// Read metrics
	if(doc.HasMember("metrics") && doc["metrics"].IsObject()) {
		const auto& metricsObj = doc["metrics"];
		atlas.ascender = metricsObj["ascender"].GetFloat();
		atlas.descender = metricsObj["descender"].GetFloat();
	}

	// Read glyphs
	if(doc.HasMember("glyphs") && doc["glyphs"].IsArray()) {
		const auto& glyphsArray = doc["glyphs"];
		for(const auto& glyphObj : glyphsArray.GetArray()) {
			uint32_t unicode = glyphObj["unicode"].GetUint();
			if(unicode >= FontAtlas::FIRST_CHAR && unicode <= FontAtlas::LAST_CHAR) {
				size_t index = unicode - FontAtlas::FIRST_CHAR;
				Glyph& glyph = atlas.glyphs[index];
				glyph.advance = glyphObj["advance"].GetFloat();

				if(glyphObj.HasMember("planeBounds") && glyphObj["planeBounds"].IsObject()) {
					const auto& planeBounds = glyphObj["planeBounds"];
					glyph.planeBounds[0] = { planeBounds["left"].GetFloat(), planeBounds["bottom"].GetFloat() };
					glyph.planeBounds[1] = { planeBounds["right"].GetFloat(), planeBounds["top"].GetFloat() };
				}

				if(glyphObj.HasMember("atlasBounds") && glyphObj["atlasBounds"].IsObject()) {
					const auto& atlasBounds = glyphObj["atlasBounds"];
					glyph.atlasBounds[0] = { atlasBounds["left"].GetFloat(), atlasBounds["bottom"].GetFloat() };
					glyph.atlasBounds[1] = { atlasBounds["right"].GetFloat(), atlasBounds["top"].GetFloat() };
				}
			}
		}
	}

	// Read kerning information
	if(doc.HasMember("kerning") && doc["kerning"].IsArray()) {
		const auto& kerningArray = doc["kerning"];
		for(const auto& kerningObj : kerningArray.GetArray()) {
			uint32_t unicode1 = kerningObj["unicode1"].GetUint();
			uint32_t unicode2 = kerningObj["unicode2"].GetUint();
			float advance = kerningObj["advance"].GetFloat();
			atlas.kerningMap[{unicode1, unicode2}] = advance;
		}
	}
	return atlas;
}

uint32_t TextureManager::LoadTexture(const stbi_uc* pixels, int width, int height, const std::string& name, bool isFont)
{
	auto m_allocator = VulkanManager::Get().VkAllocator();
	VkDeviceSize imageSize = width * height * 4;

	Texture newTexture{};
	// Use UNORM format for fonts (MTSDF) and SRGB for regular textures
	newTexture.format = isFont ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
	newTexture.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

	// Create image using VMA
	VkImageCreateInfo imageInfo = VkInit::ImageCreateInfo(newTexture.format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, newTexture.extent);

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	vmaCreateImage(m_allocator, &imageInfo, &allocCreateInfo, &newTexture.image._image, &newTexture.image._allocation, nullptr);

	// Use VMA to stage and transfer the image data
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = imageSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocationInfo allocationInfo;
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;

	vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &allocationInfo);

	// Copy image data to staging buffer
	memcpy(allocationInfo.pMappedData, pixels, imageSize);

	// Transition image layout and copy data
	ST<Engine>::Get()->_vulkan->immediateSubmit([&](VkCommandBuffer cmd) {
		VkImageSubresourceRange range = {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier2 barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.srcAccessMask = 0;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = newTexture.image._image;
		barrier.subresourceRange = range;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmd, &dependencyInfo);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageExtent = newTexture.extent;

		vkCmdCopyBufferToImage(cmd, stagingBuffer, newTexture.image._image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		vkCmdPipelineBarrier2(cmd, &dependencyInfo);
		});

	// Cleanup staging buffer
	vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

	newTexture.imageView = createImageView(newTexture.format, newTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
	newTexture.sampler = isFont ? createFontSampler() : createSampler();

	newTexture.index = m_textureIndex++;

	VkDescriptorImageInfo vimageInfo{};
	vimageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vimageInfo.imageView = newTexture.imageView;
	vimageInfo.sampler = newTexture.sampler;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = VulkanManager::Get().VkDescriptorSetManager().getDescriptorSet(m_bindless_set, 0);
	write.dstBinding = 1;
	write.dstArrayElement = newTexture.index;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo = &vimageInfo;

	vkUpdateDescriptorSets(VulkanManager::Get().VkDevice().handle(), 1, &write, 0, nullptr);
#ifdef IMGUI_ENABLED
	newTexture.ImGui_handle = ImGui_ImplVulkan_AddTexture(
		newTexture.sampler,
		newTexture.imageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
#endif

	m_textures[name] = newTexture;
	return newTexture.index;
}
