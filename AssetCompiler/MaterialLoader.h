#pragma once
#include <vector>
#include <string>
#include <map>
#include "MeshCompilerData.h"

struct aiScene;
//struct aiTextureType;
struct aiMaterial;

namespace compiler
{

    struct ProcessedMaterialSlot
    {
        std::string name;
        uint32_t originalIndex;

        // Key: Texture type (e.g., "albedo", "normal", "metallic")
        // Value: The SOURCE filepath of the texture (e.g., "textures/car_paint_d.png")
        std::map<std::string, std::filesystem::path> texturePaths;

        // Key: Parameter name (e.g., "baseColorFactor", "roughnessFactor")
        // Value: The parameter value
        std::map<std::string, vec4> vectorParams;
        std::map<std::string, float> scalarParams;
    };

    //ProcessedMaterialSlot extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath);

    // Collection utilities
    std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene);

    //void extractTexturePath(const aiMaterial* aiMat, aiTextureType type, const std::string& key, ProcessedMaterialSlot& outSlot, const std::filesystem::path& modelBasePath);
}


