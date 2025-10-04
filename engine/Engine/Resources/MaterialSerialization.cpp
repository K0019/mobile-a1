#include "MaterialSerialization.h"

using namespace AssetLoading;


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

    //writer.StartObject();
    std::unordered_map<std::string, size_t> textures;
    textures["baseColor"] = textureHandles[0];
    textures["normal"] = textureHandles[1];
    textures["metallicRoughness"] = textureHandles[2];
    textures["emissive"] = textureHandles[3];
    textures["occlusion"] = textureHandles[4];
    writer.Serialize("textures", textures);
    //writer.EndObject(); // Textures object
}


void MaterialSerialization::Deserialize(Deserializer& reader, ProcessedMaterial& outMaterial)
{
    reader.DeserializeVar("name", &outMaterial.name);

    std::string alphaModeStr;
    if (reader.DeserializeVar("alphaMode", &alphaModeStr))
    {
        if (alphaModeStr == "Mask") outMaterial.alphaMode = AlphaMode::Mask;
        else if (alphaModeStr == "Blend") outMaterial.alphaMode = AlphaMode::Blend;
        else outMaterial.alphaMode = AlphaMode::Opaque;
    }

    reader.DeserializeVar("alphaCutoff", &outMaterial.alphaCutoff);
    reader.DeserializeVar("metallicFactor", &outMaterial.metallicFactor);
    reader.DeserializeVar("roughnessFactor", &outMaterial.roughnessFactor);
    reader.DeserializeVar("normalScale", &outMaterial.normalScale);
    reader.DeserializeVar("occlusionStrength", &outMaterial.occlusionStrength);


    std::array<float, 4> baseColorArray;
    if (reader.DeserializeVar("baseColorFactor", &baseColorArray))
    {
        outMaterial.baseColorFactor = { baseColorArray[0], baseColorArray[1], baseColorArray[2], baseColorArray[3] };
    }

    std::array<float, 3> emissiveArray;
    if (reader.DeserializeVar("emissiveFactor", &emissiveArray))
    {
        outMaterial.emissiveFactor = { emissiveArray[0], emissiveArray[1], emissiveArray[2] };
    }

    std::unordered_map<std::string, std::string> texturePaths;
    if (reader.DeserializeVar("textures", &texturePaths))
    {
        if (texturePaths.contains("baseColor") && !texturePaths["baseColor"].empty())
            outMaterial.baseColorTexture.source = FilePathSource{ texturePaths["baseColor"] };

        if (texturePaths.contains("normal") && !texturePaths["normal"].empty())
            outMaterial.normalTexture.source = FilePathSource{ texturePaths["normal"] };

        if (texturePaths.contains("metallicRoughness") && !texturePaths["metallicRoughness"].empty())
            outMaterial.metallicRoughnessTexture.source = FilePathSource{ texturePaths["metallicRoughness"] };

        if (texturePaths.contains("emissive") && !texturePaths["emissive"].empty())
            outMaterial.emissiveTexture.source = FilePathSource{ texturePaths["emissive"] };

        if (texturePaths.contains("occlusion") && !texturePaths["occlusion"].empty())
            outMaterial.occlusionTexture.source = FilePathSource{ texturePaths["occlusion"] };
    }
}