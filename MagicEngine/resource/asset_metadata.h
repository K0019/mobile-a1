#pragma once
#include "resource/asset_formats/asset_type.h"
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Resource
{
    /**
     * @brief Metadata associated with a compiled asset.
     *
     * This class represents the .meta sidecar files that track:
     * - Asset type
     * - Original source file path
     * - Timestamps for incremental compilation
     * - Format version for compatibility checks
     */
    class AssetMetadata
    {
    public:
        // Data members
        AssetFormat::AssetType assetType = AssetFormat::AssetType::Unknown;
        std::string sourcePath;
        uint64_t sourceTimestamp = 0;
        uint64_t compiledTimestamp = 0;
        uint32_t formatVersion = 0;

        /**
         * @brief Get the .meta file path for a given asset path.
         * @param assetPath Path to the compiled asset
         * @return Path to the corresponding .meta file
         */
        static std::filesystem::path getMetaPath(const std::filesystem::path& assetPath)
        {
            return assetPath.string() + ".meta";
        }

        /**
         * @brief Load metadata from a .meta file.
         * @param metaPath Path to the .meta file
         * @return true if loaded successfully, false otherwise
         */
        bool loadFromFile(const std::filesystem::path& metaPath)
        {
            std::ifstream file(metaPath);
            if (!file.is_open())
                return false;

            std::string line;
            while (std::getline(file, line))
            {
                // Skip empty lines and comments
                if (line.empty() || line[0] == '#')
                    continue;

                size_t colonPos = line.find(':');
                if (colonPos == std::string::npos)
                    continue;

                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);

                // Trim whitespace
                while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
                    key.pop_back();
                while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
                    value.erase(0, 1);

                if (key == "assetType")
                    assetType = parseAssetType(value.c_str());
                else if (key == "sourcePath")
                    sourcePath = value;
                else if (key == "sourceTimestamp")
                    sourceTimestamp = std::stoull(value);
                else if (key == "compiledTimestamp")
                    compiledTimestamp = std::stoull(value);
                else if (key == "formatVersion")
                    formatVersion = static_cast<uint32_t>(std::stoul(value));
            }

            return true;
        }

        /**
         * @brief Save metadata to a .meta file.
         * @param metaPath Path to the .meta file
         * @return true if saved successfully, false otherwise
         */
        bool saveToFile(const std::filesystem::path& metaPath) const
        {
            std::ofstream file(metaPath);
            if (!file.is_open())
                return false;

            file << "# Asset metadata - auto-generated\n";
            file << "assetType: " << AssetFormat::getAssetTypeName(assetType) << "\n";
            if (!sourcePath.empty())
                file << "sourcePath: " << sourcePath << "\n";
            if (sourceTimestamp > 0)
                file << "sourceTimestamp: " << sourceTimestamp << "\n";
            if (compiledTimestamp > 0)
                file << "compiledTimestamp: " << compiledTimestamp << "\n";
            if (formatVersion > 0)
                file << "formatVersion: " << formatVersion << "\n";

            return true;
        }

        /**
         * @brief Parse an asset type from a string name.
         * @param name The string name of the asset type (e.g., "Mesh", "Texture")
         * @return The corresponding AssetType enum value
         */
        static AssetFormat::AssetType parseAssetType(const char* name)
        {
            if (!name || name[0] == '\0')
                return AssetFormat::AssetType::Unknown;
            return AssetFormat::parseAssetType(name);
        }

        /**
         * @brief Get the display name for an asset type.
         * @param type The asset type
         * @return Human-readable name string
         */
        static const char* getTypeName(AssetFormat::AssetType type)
        {
            return AssetFormat::getAssetTypeName(type);
        }

        /**
         * @brief Infer asset type from file path/extension.
         * @param filepath The file path or extension
         * @return Inferred asset type
         */
        static AssetFormat::AssetType inferFromPath(const std::string& filepath)
        {
            size_t dotPos = filepath.rfind('.');
            if (dotPos == std::string::npos)
                return AssetFormat::AssetType::Unknown;

            return AssetFormat::getAssetTypeFromExtension(filepath.substr(dotPos));
        }
    };

} // namespace Resource
