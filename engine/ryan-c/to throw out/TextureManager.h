#pragma once
/******************************************************************************/
/*!
\file   TextureManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Texture manager for managing textures and font atlases.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "VulkanHelper.h"
#include "Buffer.h"
#include "DescriptorSetManager.h"
#include "FontAtlas.h"
#include "VulkanManager.h"

// Forward declarations
typedef unsigned char stbi_uc;


struct AllocatedImage {
    VkImage _image;
    VmaAllocation _allocation; 
};

struct Texture {
    AllocatedImage image;
    VkImageView imageView;
    VkSampler sampler;
    VkFormat format;
    VkExtent3D extent;
    uint32_t index;
    #ifdef IMGUI_ENABLED
    VkDescriptorSet ImGui_handle;
#endif
};

class TextureManager {
public:
    explicit TextureManager();
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    TextureManager(TextureManager&&) noexcept;
    TextureManager& operator=(TextureManager&&) noexcept;

    
    // Resource management

    /*****************************************************************//*!
    \brief
        Loads a texture from an array.
        Each pixel within this array must be in a format of 4 unsigned chars in RGBA order.
    \param data
        Pointer to the start of pixel data.
    \param width
        The number of pixels horizontally.
    \param height
        The number of pixels vertically.
    \param name
        The name of the texture.
    \return
        The index of the texture within the manager.
    *//******************************************************************/
    uint32_t LoadTextureFromMemory(const unsigned char* data, int width, int height, const std::string& name);

    uint32_t loadTextureFromFile(const std::string& filename, const std::string& name, bool isFont = false);
    std::string loadFontAtlasFromFile(const std::string& filename);
    const Texture& getTexture(const std::string& name);
    const Texture& getTexture(uint32_t index) const;
    uint32_t getTextureIndex(const std::string& name);
    const FontAtlas& getFontAtlas(const std::string& name);
    DescriptorSetManager::DescriptorSetHandle getBindlessSet() const;
    void SetBindlessLayout(VkDescriptorSetLayout layout);

    bool TextureExists(const std::string& name) const;
    bool FontAtlasExists(const std::string& name);

    const std::unordered_map<std::string, FontAtlas>& getFontAtlases() const;

private:
    uint32_t m_textureIndex = 0;
    std::unordered_map<std::string, Texture>  m_textures;
    std::unordered_map<std::string, FontAtlas> m_fontAtlases;
    VkDescriptorSetLayout m_bindless_layout;
    DescriptorSetManager::DescriptorSetHandle m_bindless_set;
    VkImageView createImageView(VkFormat vk_format, VkImage image, VkImageAspectFlags flags);
    VkSampler createSampler();
    VkSampler createFontSampler();
    // Private method to get image reference, throws if not found
    Texture& getTextureInternal(const std::string& name);
    std::string removeDirectoryAndExtension(const std::string& filename);
    FontAtlas loadJsonData(const std::string& jsonPath);

    /*****************************************************************//*!
    \brief
        Adds a STBI loaded texture to the manager.
    \param pixels
        Pointer to the pixels of the texture, loaded by STBI.
        Expected format: STBI_rgb_alpha
    \param width
        The number of pixels horizontally.
    \param height
        The number of pixels vertically.
    \param name
        The name of the texture.
    \param isFont
        Whether the texture is a font.
    \return
        The index of the texture within the manager.
    *//******************************************************************/
    uint32_t LoadTexture(const stbi_uc* pixels, int width, int height, const std::string& name, bool isFont);
};
