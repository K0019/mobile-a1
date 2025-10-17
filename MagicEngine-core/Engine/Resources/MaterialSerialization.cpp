#include "Engine/Resources/MaterialSerialization.h"
#include "Engine/Resources/ResourceManager.h"
#include "GameSettings.h"

#include <variant>

using namespace Resource;

namespace
{
    std::string getPathFromHandle(size_t handle)
    {
        auto& filepathManager = ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager();
        if (handle == 0)
        {
            return ""; // return an empty string for invalid handles
        }

        if (auto* fileEntry = filepathManager.GetFileEntry(handle))
        {
            return fileEntry->path.string();
        }

        return "";
    }

    TextureDataSource constructDataSource(const std::filesystem::path& assetsRootPath, const std::filesystem::path& relativePath)
    {
        if (relativePath.empty())
        {
            return std::monostate{};
        }
        else
        {
            return FilePathSource{ assetsRootPath / relativePath };
        }
    }
}

void MaterialSerialization::Serialize(Serializer& writer, const ProcessedMaterial& material, std::array<size_t, 5> textureHandles)
{
    writer.Serialize("name", material.name);

    switch (material.alphaMode)
    {
    case AlphaMode::Opaque: writer.Serialize("alphaMode", std::string{ "Opaque" }); break;
    case AlphaMode::Mask:   writer.Serialize("alphaMode", std::string{ "Mask" });   break;
    case AlphaMode::Blend:  writer.Serialize("alphaMode", std::string{ "Blend" });  break;
    }
    writer.Serialize("alphaCutoff", material.alphaCutoff);
    writer.Serialize("metallicFactor", material.metallicFactor);
    writer.Serialize("roughnessFactor", material.roughnessFactor);
    writer.Serialize("normalScale", material.normalScale);
    writer.Serialize("occlusionStrength", material.occlusionStrength);

    std::array<float, 4> baseColor = { material.baseColorFactor.x, material.baseColorFactor.y, material.baseColorFactor.z, material.baseColorFactor.w };
    writer.Serialize("baseColorFactor", baseColor);

    std::array<float, 3> emissive = { material.emissiveFactor.x, material.emissiveFactor.y, material.emissiveFactor.z };
    writer.Serialize("emissiveFactor", emissive);

    std::unordered_map<std::string, std::string> textures;
    textures["baseColor"]         = getPathFromHandle(textureHandles[0]);
    textures["metallicRoughness"] = getPathFromHandle(textureHandles[1]);
    textures["normal"]            = getPathFromHandle(textureHandles[2]);
    textures["emissive"]          = getPathFromHandle(textureHandles[3]);
    textures["occlusion"]         = getPathFromHandle(textureHandles[4]);
    writer.Serialize("textures", textures);

    writer.Serialize("flags", material.flags);
    //writer.Serialize("materialTypeFlags", material.materialTypeFlags);
}


void MaterialSerialization::Deserialize(Deserializer& reader, ProcessedMaterial& outMaterial)
{
    const auto& assetsRootPath = ST<Filepaths>::Get()->assets + "/";

    reader.DeserializeVar("name", &outMaterial.name);

    std::string alphaModeStr;
    if (reader.DeserializeVar("alphaMode", &alphaModeStr))
    {
        if (alphaModeStr == "Mask")      outMaterial.alphaMode = AlphaMode::Mask;
        else if (alphaModeStr == "Blend") outMaterial.alphaMode = AlphaMode::Blend;
        else                              outMaterial.alphaMode = AlphaMode::Opaque;
    }

    reader.DeserializeVar("alphaCutoff", &outMaterial.alphaCutoff);
    reader.DeserializeVar("metallicFactor", &outMaterial.metallicFactor);
    reader.DeserializeVar("roughnessFactor", &outMaterial.roughnessFactor);
    reader.DeserializeVar("normalScale", &outMaterial.normalScale);
    reader.DeserializeVar("occlusionStrength", &outMaterial.occlusionStrength);

    std::array<float, 4> baseColor;
    if (reader.DeserializeVar("baseColorFactor", &baseColor))
    {
        outMaterial.baseColorFactor = { baseColor[0], baseColor[1], baseColor[2], baseColor[3] };
    }
    std::array<float, 3> emissive;
    if (reader.DeserializeVar("emissiveFactor", &emissive))
    {
        outMaterial.emissiveFactor = { emissive[0], emissive[1], emissive[2] };
    }

    std::unordered_map<std::string, std::string> textures;
    if (reader.DeserializeVar("textures", &textures))
    {
        outMaterial.baseColorTexture.source         = constructDataSource(assetsRootPath, textures["baseColor"]);
        outMaterial.metallicRoughnessTexture.source = constructDataSource(assetsRootPath, textures["metallicRoughness"]);
        outMaterial.normalTexture.source            = constructDataSource(assetsRootPath, textures["normal"]);
        outMaterial.emissiveTexture.source          = constructDataSource(assetsRootPath, textures["emissive"]);
        outMaterial.occlusionTexture.source         = constructDataSource(assetsRootPath, textures["occlusion"]);
    }

    reader.DeserializeVar("flags", &outMaterial.flags);
    //reader.DeserializeVar("materialTypeFlags", &outMaterial.materialTypeFlags);


    //Default flags
    outMaterial.materialTypeFlags = MaterialType::METALLIC_ROUGHNESS;
    outMaterial.flags |= MaterialFlags::DEFAULT_FLAGS;
    //outMaterial.flags = MaterialFlags::DEFAULT_FLAGS;

    // CRITICAL FIX: Set alpha mode in flags
    uint32_t alphaModeFlags = static_cast<uint32_t>(outMaterial.alphaMode) & MaterialFlags::ALPHA_MODE_MASK;
    outMaterial.flags = (outMaterial.flags & ~MaterialFlags::ALPHA_MODE_MASK) | alphaModeFlags;

}