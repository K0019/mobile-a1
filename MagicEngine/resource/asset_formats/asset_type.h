#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace AssetFormat
{
    /**
     * @brief Enumeration of supported asset types in the engine.
     */
    enum class AssetType : uint8_t
    {
        Unknown = 0,
        Mesh,
        Animation,
        Material,
        Texture,
        Audio,
        AudioGroup,
        Scene,
        Prefab,
        Script,
        Shader,
        Font,
        GameData,  // Game-specific data files (e.g., .weapon)
        Video,     // Video files (e.g., .mp4, .webm, .mkv)
        Count
    };

    /**
     * @brief Get the string name for an asset type.
     * @param type The asset type
     * @return String representation of the asset type
     */
    inline constexpr const char* getAssetTypeName(AssetType type)
    {
        switch (type)
        {
            case AssetType::Unknown:   return "Unknown";
            case AssetType::Mesh:      return "Mesh";
            case AssetType::Animation: return "Animation";
            case AssetType::Material:  return "Material";
            case AssetType::Texture:   return "Texture";
            case AssetType::Audio:     return "Audio";
            case AssetType::AudioGroup:return "AudioGroup";
            case AssetType::Scene:     return "Scene";
            case AssetType::Prefab:    return "Prefab";
            case AssetType::Script:    return "Script";
            case AssetType::Shader:    return "Shader";
            case AssetType::Font:      return "Font";
            case AssetType::GameData:  return "GameData";
            case AssetType::Video:     return "Video";
            default:                   return "Unknown";
        }
    }

    /**
     * @brief Get asset type from file extension.
     * @param extension File extension (with or without leading dot)
     * @return Corresponding asset type, or Unknown if not recognized
     */
    inline AssetType getAssetTypeFromExtensionImpl(std::string_view extension)
    {
        // Remove leading dot if present
        if (!extension.empty() && extension[0] == '.')
            extension = extension.substr(1);

        // Mesh formats
        if (extension == "mesh" || extension == "fbx" || extension == "gltf" ||
            extension == "glb" || extension == "obj" || extension == "dae")
            return AssetType::Mesh;

        // Animation formats
        if (extension == "anim")
            return AssetType::Animation;

        // Material formats
        if (extension == "material" || extension == "mat")
            return AssetType::Material;

        // Texture formats
        if (extension == "ktx" || extension == "ktx2" || extension == "dds" ||
            extension == "png" || extension == "jpg" || extension == "jpeg" ||
            extension == "tga" || extension == "bmp" || extension == "hdr" ||
            extension == "exr")
            return AssetType::Texture;

        // Audio formats
        if (extension == "mp3" || extension == "wav" || extension == "ogg" ||
            extension == "flac")
            return AssetType::Audio;

        // Audio group
        if (extension == "sg")
            return AssetType::AudioGroup;

        // Scene formats
        if (extension == "scene")
            return AssetType::Scene;

        // Prefab formats
        if (extension == "prefab")
            return AssetType::Prefab;

        // Script formats
        if (extension == "lua")
            return AssetType::Script;

        // Shader formats
        if (extension == "vert" || extension == "frag" || extension == "comp" ||
            extension == "glsl" || extension == "hlsl" || extension == "spv" ||
            extension == "hina_sl")
            return AssetType::Shader;

        // Font formats
        if (extension == "ttf" || extension == "otf")
            return AssetType::Font;

        // Game data formats
        if (extension == "weapon")
            return AssetType::GameData;

        // Video formats
        if (extension == "mp4" || extension == "webm" || extension == "mkv" ||
            extension == "mov" || extension == "avi")
            return AssetType::Video;

        return AssetType::Unknown;
    }

    /**
     * @brief Get asset type from file extension (string_view overload).
     */
    inline AssetType getAssetTypeFromExtension(std::string_view extension)
    {
        return getAssetTypeFromExtensionImpl(extension);
    }

    /**
     * @brief Get asset type from file extension (const string& overload for compatibility).
     */
    inline AssetType getAssetTypeFromExtension(const std::string& extension)
    {
        return getAssetTypeFromExtensionImpl(extension);
    }

    /**
     * @brief Parse asset type from string name.
     * @param name String name of the asset type
     * @return Corresponding asset type, or Unknown if not recognized
     */
    inline AssetType parseAssetType(std::string_view name)
    {
        if (name == "Mesh")      return AssetType::Mesh;
        if (name == "Animation") return AssetType::Animation;
        if (name == "Material")  return AssetType::Material;
        if (name == "Texture")   return AssetType::Texture;
        if (name == "Audio")     return AssetType::Audio;
        if (name == "AudioGroup")return AssetType::AudioGroup;
        if (name == "Scene")     return AssetType::Scene;
        if (name == "Prefab")    return AssetType::Prefab;
        if (name == "Script")    return AssetType::Script;
        if (name == "Shader")    return AssetType::Shader;
        if (name == "Font")      return AssetType::Font;
        if (name == "GameData")  return AssetType::GameData;
        if (name == "Video")     return AssetType::Video;
        return AssetType::Unknown;
    }

} // namespace AssetFormat
