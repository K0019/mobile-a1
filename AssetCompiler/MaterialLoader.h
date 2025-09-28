#pragma once
#include <vector>
#include "DataStructs.h"

struct aiScene;
struct aiMaterial;

namespace compiler
{

    struct ProcessedMaterialSlot
    {
        std::string name;
        uint32_t originalIndex;
        uint32_t materialNameHash;
    };

    ProcessedMaterialSlot extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex);

    // Collection utilities
    std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene, const LoadingConfig& config);
}


