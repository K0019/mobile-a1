#include "MaterialLoader.h"
#include <assimp/scene.h>
#include <assimp/GltfMaterial.h>
#include <algorithm>
#include <filesystem>

namespace compiler
{
    std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene)
    {
        if (!scene || scene->mNumMaterials == 0)
        {
            return {};
        }

        std::vector<const aiMaterial*> materialPtrs;
        materialPtrs.reserve(scene->mNumMaterials);

        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            if (scene->mMaterials[i])
            {
                materialPtrs.push_back(scene->mMaterials[i]);
            }
        }

        return materialPtrs;
    }
}


/*
Material convertToStoredMaterial(const ProcessedMaterial& processed)
{
    Material material;

    material.name = processed.name;
    material.baseColorFactor = processed.baseColorFactor;
    material.metallicFactor = processed.metallicFactor;
    material.roughnessFactor = processed.roughnessFactor;
    material.emissiveFactor = processed.emissiveFactor;
    material.normalScale = processed.normalScale;
    material.occlusionStrength = processed.occlusionStrength;
    material.alphaCutoff = processed.alphaCutoff;

    material.baseColorTexture = processed.baseColorTexture;
    material.metallicRoughnessTexture = processed.metallicRoughnessTexture;
    material.normalTexture = processed.normalTexture;
    material.emissiveTexture = processed.emissiveTexture;
    material.occlusionTexture = processed.occlusionTexture;

    material.alphaMode = processed.alphaMode;
    material.materialTypeFlags = processed.materialTypeFlags;
    material.flags = processed.flags;
    material.doubleSided = processed.doubleSided;

    return material;
}

bool isValidMaterial(const ProcessedMaterial& material)
{
    return !material.name.empty();
}

std::vector<ProcessedMaterial> deduplicateMaterials(std::vector<ProcessedMaterial> materials, float tolerance)
    {
        if (materials.size() <= 1)
        {
            return materials;
        }

        std::vector<bool> isDuplicate(materials.size(), false);

        for (size_t i = 1; i < materials.size(); ++i)
        {
            if (isDuplicate[i])
                continue;

            for (size_t j = 0; j < i; ++j)
            {
                if (!isDuplicate[j] && Detail::compareMaterials(materials[i], materials[j], tolerance))
                {
                    isDuplicate[i] = true;
                    break;
                }
            }
        }

        // Filter out duplicates
        std::vector<ProcessedMaterial> uniqueMaterials;
        uniqueMaterials.reserve(materials.size());

        for (size_t i = 0; i < materials.size(); ++i)
        {
            if (!isDuplicate[i])
            {
                uniqueMaterials.push_back(std::move(materials[i]));
            }
        }

        if (uniqueMaterials.size() != materials.size())
        {
            //LOG_INFO("Deduplicated {} materials to {}", materials.size(), uniqueMaterials.size());
        }

        return uniqueMaterials;
    }

namespace Detail
{
    MaterialTexture extractTextureWithTransform(const aiMaterial* aiMat, unsigned int type, const std::string& scenePath, const std::string& baseDir, const aiScene* scene)
    {
        MaterialTexture matTex;

        if (aiMat->GetTextureCount(static_cast<aiTextureType>(type)) == 0)
        {
            return matTex;
        }

        aiString path;
        aiTextureMapping mapping;
        unsigned int uvIndex;
        ai_real blend;
        aiTextureOp op;
        aiTextureMapMode mapMode[2];

        if (aiMat->GetTexture(static_cast<aiTextureType>(type), 0, &path, &mapping, &uvIndex, &blend, &op, mapMode) == AI_SUCCESS)
        {
            matTex.source = extractTextureSource(path.C_Str(), scenePath, baseDir, scene);
            matTex.uvSet = uvIndex;

            // Extract texture transforms if available
            aiUVTransform uvTransform;
            if (aiMat->Get(AI_MATKEY_UVTRANSFORM(type, 0), uvTransform) == AI_SUCCESS)
            {
                matTex.uvOffset = vec2(uvTransform.mTranslation.x, uvTransform.mTranslation.y);
                matTex.uvScale = vec2(uvTransform.mScaling.x, uvTransform.mScaling.y);
            }
        }

        return matTex;
    }

    TextureDataSource extractTextureSource(const std::string& texturePath, const std::string& scenePath, const std::string& baseDir, const aiScene* scene)
    {
        if (texturePath.empty())
        {
            return std::monostate{};
        }

        // Check for embedded texture (starts with '*')
        if (texturePath[0] == '*')
        {
            EmbeddedMemorySource embeddedSource;
            embeddedSource.scene = scene;
            embeddedSource.identifier = texturePath;
            embeddedSource.scenePath = scenePath;
            return embeddedSource;
        }
        else
        {
            // File-based texture
            FilePathSource fileSource;

            if (std::filesystem::path(texturePath).is_absolute())
            {
                fileSource.path = texturePath;
            }
            else
            {
                // Resolve relative path against base directory
                std::filesystem::path resolvedPath = std::filesystem::path(baseDir) / texturePath;
                fileSource.path = resolvedPath.lexically_normal().string();
            }

            return fileSource;
        }
    }

    void populateTexturesVector(ProcessedMaterial& material)
    {
        material.textures.clear();

        // Helper to add valid texture sources
        auto addIfValid = [&](const MaterialTexture& matTex)
            {
                if (matTex.hasTexture())
                {
                    material.textures.push_back(matTex.source);
                }
            };

        // Add all texture sources
        addIfValid(material.baseColorTexture);
        addIfValid(material.metallicRoughnessTexture);
        addIfValid(material.normalTexture);
        addIfValid(material.emissiveTexture);
        addIfValid(material.occlusionTexture);
    }

    bool compareMaterials(const ProcessedMaterial& a, const ProcessedMaterial& b, float tolerance)
    {
        return true;
        //auto texturesEqual = [](const MaterialTexture& texA, const MaterialTexture& texB) -> bool
        //    {
        //        std::string keyA = TextureCacheKeyGenerator::generateKey(texA.source);
        //        std::string keyB = TextureCacheKeyGenerator::generateKey(texB.source);
        //        return keyA == keyB && texA.uvSet == texB.uvSet && glm::length(texA.uvScale - texB.uvScale) < 0.001f && glm::length(texA.uvOffset - texB.uvOffset) < 0.001f;
        //    };
        //
        //auto floatEqual = [tolerance](float x, float y) { return std::abs(x - y) <= tolerance; };
        //
        //auto vec3Equal = [&floatEqual](const vec3& x, const vec3& y)
        //    {
        //        return floatEqual(x.x, y.x) && floatEqual(x.y, y.y) && floatEqual(x.z, y.z);
        //    };
        //
        //auto vec4Equal = [&floatEqual](const vec4& x, const vec4& y)
        //    {
        //        return floatEqual(x.x, y.x) && floatEqual(x.y, y.y) && floatEqual(x.z, y.z) && floatEqual(x.w, y.w);
        //    };
        //
        //// Compare material properties
        //return vec4Equal(a.baseColorFactor, b.baseColorFactor) && floatEqual(a.metallicFactor, b.metallicFactor) && floatEqual(a.roughnessFactor, b.roughnessFactor) && vec3Equal(a.emissiveFactor, b.emissiveFactor) && floatEqual(a.normalScale, b.normalScale) && floatEqual(a.occlusionStrength, b.occlusionStrength) && floatEqual(a.alphaCutoff, b.alphaCutoff) && a.alphaMode == b.alphaMode && a.materialTypeFlags == b.materialTypeFlags && a.flags == b.flags && a.doubleSided == b.doubleSided && texturesEqual(a.baseColorTexture, b.baseColorTexture) && texturesEqual(a.metallicRoughnessTexture, b.metallicRoughnessTexture) && texturesEqual(a.normalTexture, b.normalTexture) && texturesEqual(a.emissiveTexture, b.emissiveTexture) && texturesEqual(a.occlusionTexture, b.occlusionTexture);
    }
} // namespace Detail

*/
