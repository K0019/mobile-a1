#include "material_loader.h"
#include "logging/log.h"
#include <assimp/scene.h>
#include <assimp/GltfMaterial.h>
#include <algorithm>
#include <filesystem>

namespace Resource::MaterialLoading
{
  ProcessedMaterial extractMaterial(const aiMaterial* aiMat, uint32_t materialIndex, const std::string& scenePath,
                                    const std::string& baseDir, const aiScene* scene)
  {
    ProcessedMaterial material;
    material.flags = MaterialFlags::DEFAULT_FLAGS;
    // Extract material name
    aiString name;
    if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && strlen(name.C_Str()) > 0)
    {
      material.name = name.C_Str();
    }
    else
    {
      material.name = "Material_" + std::to_string(materialIndex);
    }
    // Extract PBR properties with comprehensive fallbacks
    aiColor4D color;
    float factor;
    // Base color (albedo) with multiple fallback paths
    if (aiMat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
    {
      material.baseColorFactor = vec4(color.r, color.g, color.b, color.a);
    }
    else if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
    {
      material.baseColorFactor = vec4(color.r, color.g, color.b, 1.0f);
    }
    else if (aiMat->Get("$clr.diffuse", 0, 0, color) == AI_SUCCESS)
    {
      material.baseColorFactor = vec4(color.r, color.g, color.b, 1.0f);
    }
    // Metallic factor with multiple property keys
    if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, factor) == AI_SUCCESS)
    {
      material.metallicFactor = std::clamp(factor, 0.0f, 1.0f);
    }
    else if (aiMat->Get("$mat.metallicFactor", 0, 0, factor) == AI_SUCCESS)
    {
      material.metallicFactor = std::clamp(factor, 0.0f, 1.0f);
    }
    else if (aiMat->Get(AI_MATKEY_REFLECTIVITY, factor) == AI_SUCCESS)
    {
      material.metallicFactor = std::clamp(factor, 0.0f, 1.0f);
    }
    // Roughness factor with multiple fallback strategies
    if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, factor) == AI_SUCCESS)
    {
      material.roughnessFactor = std::clamp(factor, 0.0f, 1.0f);
    }
    else if (aiMat->Get("$mat.roughnessFactor", 0, 0, factor) == AI_SUCCESS)
    {
      material.roughnessFactor = std::clamp(factor, 0.0f, 1.0f);
    }
    else if (aiMat->Get(AI_MATKEY_SHININESS, factor) == AI_SUCCESS)
    {
      material.roughnessFactor = std::clamp(1.0f - (factor / 256.0f), 0.0f, 1.0f);
    }
    else if (aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, factor) == AI_SUCCESS)
    {
      material.roughnessFactor = std::clamp(1.0f - factor, 0.0f, 1.0f);
    }
    // Emissive factor
    if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS)
    {
      material.emissiveFactor = vec3(color.r, color.g, color.b);
    }
    else if (aiMat->Get("$clr.emissive", 0, 0, color) == AI_SUCCESS)
    {
      material.emissiveFactor = vec3(color.r, color.g, color.b);
    }
    // Normal scale
    if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), factor) == AI_SUCCESS)
    {
      material.normalScale = std::max(0.0f, factor);
    }
    // Occlusion strength
    if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_AMBIENT_OCCLUSION, 0), factor) == AI_SUCCESS)
    {
      material.occlusionStrength = std::clamp(factor, 0.0f, 1.0f);
    }
    // Alpha properties
    if (aiMat->Get(AI_MATKEY_OPACITY, factor) == AI_SUCCESS)
    {
      material.baseColorFactor.a = std::clamp(factor, 0.0f, 1.0f);
    }
    if (aiMat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, factor) == AI_SUCCESS)
    {
      material.alphaCutoff = std::clamp(factor, 0.0f, 1.0f);
    }
    aiString alphaModeStr;
    if (aiMat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaModeStr) == AI_SUCCESS)
    {
      std::string mode = alphaModeStr.C_Str();
      if (mode == "OPAQUE")
      {
        material.alphaMode = AlphaMode::Opaque;
      }
      else if (mode == "MASK")
      {
        material.alphaMode = AlphaMode::Mask;
      }
      else if (mode == "BLEND")
      {
        material.alphaMode = AlphaMode::Blend;
      }
    }
    else
    {
      // Fallback: detect transparency from alpha value
      if (material.baseColorFactor.a < 0.99f)
      {
        material.alphaMode = AlphaMode::Blend;
      }
      else
      {
        material.alphaMode = AlphaMode::Opaque;
      }
    }
    // Alpha cutoff
    if (aiMat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, factor) == AI_SUCCESS)
    {
      material.alphaCutoff = std::clamp(factor, 0.0f, 1.0f);
    }
    // Unlit detection
    int shadingModel;
    if (aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == AI_SUCCESS)
    {
      if (shadingModel == aiShadingMode_Unlit || shadingModel == aiShadingMode_NoShading)
      {
        material.flags |= MaterialFlags::UNLIT;
      }
    }
    // Alternative unlit detection for various formats
    aiString unlitStr;
    if (aiMat->Get("unlit", 0, 0, unlitStr) == AI_SUCCESS)
    {
      std::string unlit = unlitStr.C_Str();
      if (unlit == "true" || unlit == "1")
      {
        material.flags |= MaterialFlags::UNLIT;
      }
    }
    // Two-sided detection
    int twosided;
    if (aiMat->Get(AI_MATKEY_TWOSIDED, twosided) == AI_SUCCESS && twosided)
    {
      material.flags |= MaterialFlags::DOUBLE_SIDED;
    }
    // Set material type
    material.materialTypeFlags = MaterialType::METALLIC_ROUGHNESS;
    // CRITICAL FIX: Set alpha mode in flags
    uint32_t alphaModeFlags = static_cast<uint32_t>(material.alphaMode) & MaterialFlags::ALPHA_MODE_MASK;
    material.flags = (material.flags & ~MaterialFlags::ALPHA_MODE_MASK) | alphaModeFlags;
    // Extract textures with robust fallback system
    // Color textures (baseColor, emissive) use sRGB, data textures (normal, metallicRoughness, occlusion) use linear
    material.baseColorTexture = Detail::extractTextureWithTransform(aiMat, aiTextureType_BASE_COLOR, scenePath, baseDir,
                                                                    scene);
    material.baseColorTexture.isSRGB = true;  // Color texture

    material.metallicRoughnessTexture = Detail::extractTextureWithTransform(
      aiMat, aiTextureType_METALNESS, scenePath, baseDir, scene);
    material.metallicRoughnessTexture.isSRGB = false;  // Data texture (packed values)

    material.normalTexture = Detail::extractTextureWithTransform(aiMat, aiTextureType_NORMALS, scenePath, baseDir,
                                                                 scene);
    material.normalTexture.isSRGB = false;  // Data texture (vectors, not colors)

    material.emissiveTexture = Detail::extractTextureWithTransform(aiMat, aiTextureType_EMISSIVE, scenePath, baseDir,
                                                                   scene);
    material.emissiveTexture.isSRGB = true;  // Color texture

    material.occlusionTexture = Detail::extractTextureWithTransform(aiMat, aiTextureType_AMBIENT_OCCLUSION, scenePath,
                                                                    baseDir, scene);
    material.occlusionTexture.isSRGB = false;  // Data texture (grayscale values)

    // Fallback for base color texture
    if (!material.baseColorTexture.hasTexture())
    {
      material.baseColorTexture = Detail::extractTextureWithTransform(aiMat, aiTextureType_DIFFUSE, scenePath, baseDir,
                                                                      scene);
      material.baseColorTexture.isSRGB = true;  // Color texture
    }
    // Fallback for normal texture
    if (!material.normalTexture.hasTexture())
    {
      material.normalTexture = Detail::extractTextureWithTransform(aiMat, aiTextureType_HEIGHT, scenePath, baseDir,
                                                                   scene);
      material.normalTexture.isSRGB = false;  // Data texture (vectors)
    }
    // Populate textures vector for texture loader
    Detail::populateTexturesVector(material);
    return material;
  }

  std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene, const LoadingConfig& config)
  {
    if (!scene || scene->mNumMaterials == 0)
    {
      return {};
    }
    std::vector<const aiMaterial*> materialPtrs;
    materialPtrs.reserve(std::min(scene->mNumMaterials, config.maxMaterials));
    for (uint32_t i = 0; i < scene->mNumMaterials && i < config.maxMaterials; ++i)
    {
      if (scene->mMaterials[i])
      {
        materialPtrs.push_back(scene->mMaterials[i]);
      }
    }
    return materialPtrs;
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
      if (isDuplicate[i]) continue;
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
      LOG_INFO("Deduplicated {} materials to {}", materials.size(), uniqueMaterials.size());
    }
    return uniqueMaterials;
  }

  namespace Detail
  {
    MaterialTexture extractTextureWithTransform(const aiMaterial* aiMat, unsigned int type,
                                                const std::string& scenePath, const std::string& baseDir,
                                                const aiScene* scene)
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
      if (aiMat->GetTexture(static_cast<aiTextureType>(type), 0, &path, &mapping, &uvIndex, &blend, &op, mapMode) ==
        AI_SUCCESS)
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

    TextureDataSource extractTextureSource(const std::string& texturePath, const std::string& scenePath,
                                           const std::string& baseDir, const aiScene* scene)
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
      addIfValid(material.baseColorTexture);
      addIfValid(material.metallicRoughnessTexture);
      addIfValid(material.normalTexture);
      addIfValid(material.emissiveTexture);
      addIfValid(material.occlusionTexture);
    }

    bool compareMaterials(const ProcessedMaterial& a, const ProcessedMaterial& b, float tolerance)
    {
      auto texturesEqual = [](const MaterialTexture& texA, const MaterialTexture& texB) -> bool
      {
        std::string keyA = TextureCacheKeyGenerator::generateKey(texA.source);
        std::string keyB = TextureCacheKeyGenerator::generateKey(texB.source);
        return keyA == keyB && texA.uvSet == texB.uvSet && glm::length(texA.uvScale - texB.uvScale) < 0.001f &&
          glm::length(texA.uvOffset - texB.uvOffset) < 0.001f;
      };
      auto floatEqual = [tolerance](float x, float y) { return std::abs(x - y) <= tolerance; };
      auto vec3Equal = [&floatEqual](const vec3& x, const vec3& y)
      {
        return floatEqual(x.x, y.x) && floatEqual(x.y, y.y) && floatEqual(x.z, y.z);
      };
      auto vec4Equal = [&floatEqual](const vec4& x, const vec4& y)
      {
        return floatEqual(x.x, y.x) && floatEqual(x.y, y.y) && floatEqual(x.z, y.z) && floatEqual(x.w, y.w);
      };
      // Compare material properties
      return vec4Equal(a.baseColorFactor, b.baseColorFactor) && floatEqual(a.metallicFactor, b.metallicFactor) &&
        floatEqual(a.roughnessFactor, b.roughnessFactor) && vec3Equal(a.emissiveFactor, b.emissiveFactor) &&
        floatEqual(a.normalScale, b.normalScale) && floatEqual(a.occlusionStrength, b.occlusionStrength) &&
        floatEqual(a.alphaCutoff, b.alphaCutoff) && a.alphaMode == b.alphaMode && a.materialTypeFlags == b.
        materialTypeFlags && a.flags == b.flags && texturesEqual(a.baseColorTexture, b.baseColorTexture) &&
        texturesEqual(a.metallicRoughnessTexture, b.metallicRoughnessTexture) &&
        texturesEqual(a.normalTexture, b.normalTexture) && texturesEqual(a.emissiveTexture, b.emissiveTexture) &&
        texturesEqual(a.occlusionTexture, b.occlusionTexture);
    }
  } // namespace Detail
} // namespace AssetLoading::MaterialLoading
